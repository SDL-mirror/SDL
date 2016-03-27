/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_RENDER_AMIGAOS4 && !SDL_RENDER_DISABLED

#include "../SDL_sysrender.h"
#include "SDL_hints.h"

#include "../../video/SDL_sysvideo.h"
#include "../../video/amigaos4/SDL_os4window.h"
#include "../../video/amigaos4/SDL_os4video.h"

#include <proto/graphics.h>
#include <proto/layers.h>
#include <intuition/intuition.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

/* AmigaOS4 compositing renderer implementation */

static SDL_Renderer *OS4_CreateRenderer(SDL_Window * window, Uint32 flags);

static void OS4_WindowEvent(SDL_Renderer * renderer,
                           const SDL_WindowEvent *event);

static int OS4_GetOutputSize(SDL_Renderer * renderer, int *w, int *h);

static int OS4_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);

static int OS4_SetTextureColorMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int OS4_SetTextureAlphaMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int OS4_SetTextureBlendMode(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int OS4_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, const void *pixels,
                            int pitch);

static int OS4_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, void **pixels, int *pitch);
static void OS4_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int OS4_SetRenderTarget(SDL_Renderer * renderer, SDL_Texture * texture);


static int OS4_UpdateViewport(SDL_Renderer * renderer);
static int OS4_UpdateClipRect(SDL_Renderer * renderer);

static int OS4_RenderClear(SDL_Renderer * renderer);

static int OS4_RenderDrawPoints(SDL_Renderer * renderer,
                               const SDL_FPoint * points, int count);
static int OS4_RenderDrawLines(SDL_Renderer * renderer,
                              const SDL_FPoint * points, int count);

static int OS4_RenderFillRects(SDL_Renderer * renderer,
                              const SDL_FRect * rects, int count);
static int OS4_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Rect * srcrect, const SDL_FRect * dstrect);
/*
static int OS4_RenderCopyEx(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * srcrect, const SDL_FRect * dstrect,
                          const double angle, const SDL_FPoint * center, const SDL_RendererFlip flip);
*/
static int OS4_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                               Uint32 format, void * pixels, int pitch);

static void OS4_RenderPresent(SDL_Renderer * renderer);

static void OS4_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture);

static void OS4_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver OS4_RenderDriver = {
	OS4_CreateRenderer,
    {
	 "compositing",
	 SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE,
	 1,
     {
	  //SDL_PIXELFORMAT_RGB555,
	  //SDL_PIXELFORMAT_RGB565,
	  //SDL_PIXELFORMAT_RGB888,
	  //SDL_PIXELFORMAT_BGR888,
      SDL_PIXELFORMAT_ARGB8888,
	  //SDL_PIXELFORMAT_RGBA8888,
	  //SDL_PIXELFORMAT_ABGR8888,
	  //SDL_PIXELFORMAT_BGRA8888
     },
     0,
     0}
};

typedef struct
{
	struct GraphicsIFace *iGraphics;
	struct LayersIFace * iLayers;
	struct BitMap *bitmap;
	struct BitMap *target;
	struct RastPort rastport;
	SDL_Rect cliprect;
} OS4_RenderData;

typedef struct
{
	struct BitMap *bitmap;
	APTR lock;
} OS4_TextureData;

static struct BitMap *
OS4_ActivateRenderer(SDL_Renderer * renderer)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;

	if (!data->bitmap && renderer->window) {

		//SDL_WindowData * windowdata = (SDL_WindowData *)renderer->window->driverdata;

		int width = renderer->window->w;
		int height = renderer->window->h;
		int depth = 32;

		dprintf("Allocating bitmap %d*%d*%d\n", width, height, depth);

		data->bitmap = data->iGraphics->AllocBitMapTags(
			width,
			height,
			depth,
            BMATags_Displayable, TRUE,
			BMATags_PixelFormat, PIXF_A8R8G8B8,
			//BMATags_Friend, windowdata->syswin->RPort->BitMap,
			TAG_DONE);

		if (data->bitmap) {			   
			OS4_UpdateViewport(renderer);
			OS4_UpdateClipRect(renderer);
		} else {
			dprintf("Allocation failed\n");
		}
	}

	return data->bitmap;
}

SDL_Renderer *
OS4_CreateRenderer(SDL_Window * window, Uint32 flags)
{
	SDL_VideoData *videodata = (SDL_VideoData *)SDL_GetVideoDevice()->driverdata;
	SDL_Renderer *renderer;
	OS4_RenderData *data;

	dprintf("Creating renderer for '%s' (flags 0x%x)\n", window->title, flags);

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

	data = (OS4_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
		OS4_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

	renderer->WindowEvent = OS4_WindowEvent;
	renderer->GetOutputSize = OS4_GetOutputSize;
	renderer->CreateTexture = OS4_CreateTexture;
	renderer->SetTextureColorMod = OS4_SetTextureColorMod;
	renderer->SetTextureAlphaMod = OS4_SetTextureAlphaMod;
	renderer->SetTextureBlendMode = OS4_SetTextureBlendMode;
	renderer->UpdateTexture = OS4_UpdateTexture;
	renderer->LockTexture = OS4_LockTexture;
	renderer->UnlockTexture = OS4_UnlockTexture;
	renderer->SetRenderTarget = OS4_SetRenderTarget;
	renderer->UpdateViewport = OS4_UpdateViewport;
	renderer->UpdateClipRect = OS4_UpdateClipRect;
	renderer->RenderClear = OS4_RenderClear;
	renderer->RenderDrawPoints = OS4_RenderDrawPoints;
	renderer->RenderDrawLines = OS4_RenderDrawLines;
	renderer->RenderFillRects = OS4_RenderFillRects;
	renderer->RenderCopy = OS4_RenderCopy;
	//renderer->RenderCopyEx = OS4_RenderCopyEx;
	renderer->RenderReadPixels = OS4_RenderReadPixels;
	renderer->RenderPresent = OS4_RenderPresent;
	renderer->DestroyTexture = OS4_DestroyTexture;
	renderer->DestroyRenderer = OS4_DestroyRenderer;
	renderer->info = OS4_RenderDriver.info;

    renderer->driverdata = data;

	data->iGraphics = videodata->iGraphics;
	data->iLayers = videodata->iLayers;

	data->iGraphics->InitRastPort(&data->rastport);

	//OS4_ActivateRenderer();

	return renderer;
}

static void
OS4_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;

	dprintf("Called\n");

    if (event->event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if (data->bitmap) {
			data->iGraphics->FreeBitMap(data->bitmap);
			data->bitmap = NULL;
		}
    }
}

static int
OS4_GetOutputSize(SDL_Renderer * renderer, int *w, int *h)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;

	struct BitMap * bitmap = OS4_ActivateRenderer(renderer);

	if (bitmap) {
        if (w) {
			*w = data->iGraphics->GetBitMapAttr(bitmap, BMA_WIDTH);
        }
        if (h) {
			*h = data->iGraphics->GetBitMapAttr(bitmap, BMA_HEIGHT);
        }

		dprintf("w=%d, h=%d\n", *w, *h);

        return 0;
    } else {
		SDL_SetError("OS4 renderer doesn't have an output bitmap");
        return -1;
    }
}

static int
OS4_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	OS4_TextureData *texturedata;

    if (!SDL_PixelFormatEnumToMasks
        (texture->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        return SDL_SetError("Unknown texture format");
    }

	dprintf("Creating texture %d*%d*%d\n", texture->w, texture->h, bpp);

	texturedata = SDL_calloc(1, sizeof(*texturedata));
	if (!texturedata)
	{
		dprintf("Failed to allocate driver data\n");
		SDL_OutOfMemory();
		return -1;
	}

	texturedata->bitmap = data->iGraphics->AllocBitMapTags(
		texture->w,
		texture->h,
		bpp,
		BMATags_Displayable, TRUE,
		BMATags_PixelFormat, PIXF_A8R8G8B8,
		//BMATags_Friend, data->bitmap,
		TAG_DONE);

	if (!texturedata->bitmap) {
		dprintf("Failed to allocate bitmap\n");
		SDL_free(texturedata);
        return -1;
    }

	/* TODO:
	SDL_SetSurfaceColorMod(texture->driverdata, texture->r, texture->g,
                           texture->b);
    SDL_SetSurfaceAlphaMod(texture->driverdata, texture->a);
    SDL_SetSurfaceBlendMode(texture->driverdata, texture->blendMode);
	*/

	texture->driverdata = texturedata;

    return 0;
}

static int
OS4_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
	/*
	SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceColorMod(surface, texture->r, texture->g,
                                  texture->b);
	*/

	dprintf("TODO\n");
	return 0;
}

static int
OS4_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
	/*
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceAlphaMod(surface, texture->a);
	*/

	dprintf("TODO\n");
	return 0;
}


static int
OS4_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
	/*
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceBlendMode(surface, texture->blendMode);
	*/

	dprintf("TODO\n");
	return 0;
}

static int
OS4_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, const void *pixels, int pitch)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	OS4_TextureData *texturedata = (OS4_TextureData *) texture->driverdata;

	int32 ret = data->iGraphics->BltBitMapTags(
		BLITA_Source, pixels,
		BLITA_SrcType, BLITT_ARGB32,
		BLITA_SrcBytesPerRow, pitch,
		BLITA_Dest, texturedata->bitmap,
		BLITA_DestX, rect->x,
		BLITA_DestY, rect->y,
		BLITA_Width, rect->w,
		BLITA_Height, rect->h,
		TAG_DONE);

	if (ret != -1) {
		dprintf("BltBitMapTags(): %d\n", ret);
		return -1;
	}

	return 0;
}

static int
OS4_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * rect, void **pixels, int *pitch)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	OS4_TextureData *texturedata = (OS4_TextureData *) texture->driverdata;

	APTR baseaddress;
	uint32 bytesperrow;

	//dprintf("Called\n");

	texturedata->lock = data->iGraphics->LockBitMapTags(
		texturedata->bitmap,
		LBM_BaseAddress, &baseaddress,
		LBM_BytesPerRow, &bytesperrow,		  
		TAG_DONE);

	if (texturedata->lock) {
	    *pixels =
			 (void *) ((Uint8 *) baseaddress + rect->y * bytesperrow +
				  rect->x * 4); // TODO: supports only 32-bit
		
		*pitch = bytesperrow;
	
	    return 0;
	} else {
		dprintf("Lock failed\n");
		return -1;
	}
}

static void
OS4_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	OS4_TextureData *texturedata = (OS4_TextureData *) texture->driverdata;

	//dprintf("Called\n");

	if (texturedata->lock) {
		data->iGraphics->UnlockBitMap(texturedata->lock);
	    texturedata->lock = NULL;
	}
}

static int
OS4_SetRenderTarget(SDL_Renderer * renderer, SDL_Texture * texture)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;

	dprintf("Called for texture %p\n", texture);

	if (texture) {
		OS4_TextureData *texturedata = (OS4_TextureData *) texture->driverdata;
		data->target = texturedata->bitmap;
    } else {
		//data->surface = data->window;
		data->target = NULL;
    }
    return 0;
}

static int
OS4_UpdateViewport(SDL_Renderer * renderer)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;

	//if (!surface) {
        /* We'll update the viewport after we recreate the surface */
	//	  return 0;
	//}

	if (&renderer->viewport) {
		data->cliprect = renderer->viewport;
	} else {
		data->cliprect.x = 0;
		data->cliprect.y = 0;
		data->cliprect.w = renderer->window->w;
		data->cliprect.h = renderer->window->h;
	}

	dprintf("Cliprect: (%d,%d) - %d*%d\n",
	    data->cliprect.x, data->cliprect.y, data->cliprect.w, data->cliprect.h);

	//SDL_SetClipRect(data->surface, &renderer->viewport);
    return 0;
}

static int
OS4_UpdateClipRect(SDL_Renderer * renderer)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
    const SDL_Rect *rect = &renderer->clip_rect;

	if (rect && !SDL_RectEmpty(rect)) {
		data->cliprect = *rect;
	} else {
		data->cliprect.x = 0;
		data->cliprect.y = 0;
		data->cliprect.w = renderer->window->w;
		data->cliprect.h = renderer->window->h;
	}

	dprintf("Cliprect: (%d,%d) - %d*%d\n",
	    data->cliprect.x, data->cliprect.y, data->cliprect.w, data->cliprect.h);

/*
	//if (surface) {
        if (!SDL_RectEmpty(rect)) {
            SDL_SetClipRect(surface, rect);
        } else {
            SDL_SetClipRect(surface, NULL);
        }
    //}
*/
    return 0;
}

static int
OS4_RenderClear(SDL_Renderer * renderer)
{
	struct BitMap *bitmap = OS4_ActivateRenderer(renderer);
    Uint32 color;

	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	//Sint32 s = SDL_GetTicks();
	//dprintf("Called\n");

	if (!bitmap) {
        return -1;
    }

	color = renderer->a << 24 |
			renderer->r << 16 |
			renderer->g << 8 |
			renderer->b;

	data->rastport.BitMap = data->bitmap;
	
	data->iGraphics->RectFillColor(
		&data->rastport,
		0,
		0,
		renderer->window->w - 1,
		renderer->window->h - 1,
		color); // graphics.lib v54!

    //dprintf("Took %d\n", SDL_GetTicks() - s);

	return 0;
}

static int
OS4_RenderDrawPoints(SDL_Renderer * renderer, const SDL_FPoint * points,
                    int count)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	struct BitMap *bitmap = OS4_ActivateRenderer(renderer);
	SDL_Point *final_points;
	int i, status, ret = 0;

	if (!bitmap) {
        return -1;
	}

    final_points = SDL_stack_alloc(SDL_Point, count);
    if (!final_points) {
        return SDL_OutOfMemory();
    }
    if (renderer->viewport.x || renderer->viewport.y) {
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        for (i = 0; i < count; ++i) {
            final_points[i].x = (int)(x + points[i].x);
            final_points[i].y = (int)(y + points[i].y);
        }
    } else {
        for (i = 0; i < count; ++i) {
            final_points[i].x = (int)points[i].x;
            final_points[i].y = (int)points[i].y;
        }
	}

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {

		Uint32 color =
			renderer->a << 24 |
			renderer->r << 16 |
			renderer->g << 8 |
			renderer->b;

		data->rastport.BitMap = bitmap;

		// TODO: clipping?
		for (i = 0; i < count; ++i) {
			ret |= data->iGraphics->WritePixelColor(
				&data->rastport,
				final_points[i].x,
				final_points[i].y,
				color);
		}
		//status = SDL_DrawPoints(surface, final_points, count, color);

    } else {
		/*
        status = SDL_BlendPoints(surface, final_points, count,
                                renderer->blendMode,
                                renderer->r, renderer->g, renderer->b,
                                renderer->a);
								*/
		dprintf("TODO\n");
	}

    SDL_stack_free(final_points);

	status = ret ? -1 : 0;

    return status;
}

static int
OS4_RenderDrawLines(SDL_Renderer * renderer, const SDL_FPoint * points,
                   int count)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	struct BitMap *bitmap = OS4_ActivateRenderer(renderer);

    SDL_Point *final_points;
    int i, status;

	if (!bitmap) {
        return -1;
	}

    final_points = SDL_stack_alloc(SDL_Point, count);
    if (!final_points) {
        return SDL_OutOfMemory();
    }
    if (renderer->viewport.x || renderer->viewport.y) {
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        for (i = 0; i < count; ++i) {
            final_points[i].x = (int)(x + points[i].x);
            final_points[i].y = (int)(y + points[i].y);
        }
    } else {
        for (i = 0; i < count; ++i) {
            final_points[i].x = (int)points[i].x;
            final_points[i].y = (int)points[i].y;
        }
    }

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {

		Uint32 color =
			renderer->a << 24 |
			renderer->r << 16 |
			renderer->g << 8 |
			renderer->b;

		data->rastport.BitMap = bitmap;
		
		data->iGraphics->SetRPAttrs(&data->rastport, RPTAG_APenColor, color, TAG_DONE);

		// TODO: clipping?
		for (i = 0; i < count - 1; ++i) {
			data->iGraphics->Move(
				&data->rastport,
				final_points[i].x,
				final_points[i].y);

			data->iGraphics->Draw(
				&data->rastport,
				final_points[i + 1].x,
				final_points[i + 1].y);
		}

		status = 0;
    } else {
/*
        status = SDL_BlendLines(surface, final_points, count,
                                renderer->blendMode,
                                renderer->r, renderer->g, renderer->b,
                                renderer->a);
*/
        dprintf("TODO\n");
    }

    SDL_stack_free(final_points);

    return status;
}

static int
OS4_RenderFillRects(SDL_Renderer * renderer, const SDL_FRect * rects, int count)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	struct BitMap *bitmap = OS4_ActivateRenderer(renderer);
    SDL_Rect *final_rects;
    int i, status;

	//dprintf("Called for %d rects\n", count);
	//Sint32 s = SDL_GetTicks();
	
	if (!bitmap) {
        return -1;
    }

    final_rects = SDL_stack_alloc(SDL_Rect, count);
    if (!final_rects) {
        return SDL_OutOfMemory();
    }
    if (renderer->viewport.x || renderer->viewport.y) {
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        for (i = 0; i < count; ++i) {
            final_rects[i].x = (int)(x + rects[i].x);
            final_rects[i].y = (int)(y + rects[i].y);
            final_rects[i].w = SDL_max((int)rects[i].w, 1);
            final_rects[i].h = SDL_max((int)rects[i].h, 1);
        }
    } else {
        for (i = 0; i < count; ++i) {
            final_rects[i].x = (int)rects[i].x;
            final_rects[i].y = (int)rects[i].y;
            final_rects[i].w = SDL_max((int)rects[i].w, 1);
            final_rects[i].h = SDL_max((int)rects[i].h, 1);
        }
    }

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {

		Uint32 color =
		        renderer->a << 24 |
				renderer->r << 16 |
				renderer->g << 8 |
				renderer->b;

		data->rastport.BitMap = data->bitmap;

		// TODO: clipping?
		for (i = 0; i < count; ++i) {
			
			//dprintf("%d, %d - %d, %d\n", final_rects[i].x, final_rects[i].y, final_rects[i].w, final_rects[i].h);

			data->iGraphics->RectFillColor(
				&data->rastport,
				final_rects[i].x,
				final_rects[i].y,
				final_rects[i].x + final_rects[i].w - 1,
				final_rects[i].y + final_rects[i].h - 1,
				color); // graphics.lib v54!
		}

		status = 0;
    } else {
		dprintf("TODO\n");
		/*
        status = SDL_BlendFillRects(surface, final_rects, count,
                                    renderer->blendMode,
                                    renderer->r, renderer->g, renderer->b,
                                    renderer->a);
		*/
		status = 0;
    }
    SDL_stack_free(final_rects);
    //dprintf("Took %d\n", SDL_GetTicks() - s);

    return status;
}

static int
OS4_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
              const SDL_Rect * srcrect, const SDL_FRect * dstrect)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	OS4_TextureData *texturedata = (OS4_TextureData *) texture->driverdata;
	
	struct BitMap *dst = OS4_ActivateRenderer(renderer);
	struct BitMap *src = texturedata->bitmap;

    SDL_Rect final_rect;

	float scalex, scaley;
	uint32 flags, ret_code;

	//dprintf("Called\n");
	//Sint32 s = SDL_GetTicks();
	if (!dst) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        final_rect.x = (int)(renderer->viewport.x + dstrect->x);
        final_rect.y = (int)(renderer->viewport.y + dstrect->y);
    } else {
        final_rect.x = (int)dstrect->x;
        final_rect.y = (int)dstrect->y;
    }
    final_rect.w = (int)dstrect->w;
    final_rect.h = (int)dstrect->h;

/*
    if ( srcrect->w == final_rect.w && srcrect->h == final_rect.h ) {
        return SDL_BlitSurface(src, srcrect, surface, &final_rect);
    } else {
        return SDL_BlitScaled(src, srcrect, surface, &final_rect);
    }
*/
	flags = COMPFLAG_IgnoreDestAlpha | COMPFLAG_HardwareOnly;

	scalex = srcrect->w ? (float)final_rect.w / srcrect->w : 1.0f;
	scaley = srcrect->h ? (float)final_rect.h / srcrect->h : 1.0f;
	
	ret_code = data->iGraphics->CompositeTags(
		COMPOSITE_Src_Over_Dest,
		src,
		dst,
		COMPTAG_SrcAlpha, COMP_FLOAT_TO_FIX(texture->a / 255.0f), // TODO: blend modes
		//COMPTAG_SrcAlphaMask, colorkey_bm,
		COMPTAG_SrcX,		srcrect->x,
		COMPTAG_SrcY,		srcrect->y,
		COMPTAG_SrcWidth,   srcrect->w,
		COMPTAG_SrcHeight,  srcrect->h,
		COMPTAG_OffsetX,    final_rect.x,
		COMPTAG_OffsetY,    final_rect.y,
		COMPTAG_ScaleX,     COMP_FLOAT_TO_FIX(scalex),
		COMPTAG_ScaleY,	    COMP_FLOAT_TO_FIX(scaley),
		//COMPTAG_DestX,      dstrect->x, // TODO: clipping?
		//COMPTAG_DestY,      dstrect->y,
		//COMPTAG_DestWidth,  srcrect->w,
		//COMPTAG_DestHeight, srcrect->h,
		COMPTAG_Flags,      flags,
		TAG_END);

	if (ret_code) {
		dprintf("CompositeTags: %d\n", ret_code);
		return -1;
	}

    //dprintf("Took %d\n", SDL_GetTicks() - s);

	return 0;
}

/*
static int
GetScaleQuality(void)
{
    const char *hint = SDL_GetHint(SDL_HINT_RENDER_SCALE_QUALITY);

    if (!hint || *hint == '0' || SDL_strcasecmp(hint, "nearest") == 0) {
        return 0;
    } else {
        return 1;
    }
}
*/

#if 0
static int
SW_RenderCopyEx(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * srcrect, const SDL_FRect * dstrect,
                const double angle, const SDL_FPoint * center, const SDL_RendererFlip flip)
{
    SDL_Surface *surface = SW_ActivateRenderer(renderer);
    SDL_Surface *src = (SDL_Surface *) texture->driverdata;
    SDL_Rect final_rect, tmp_rect;
    SDL_Surface *surface_rotated, *surface_scaled;
    Uint32 colorkey;
    int retval, dstwidth, dstheight, abscenterx, abscentery;
    double cangle, sangle, px, py, p1x, p1y, p2x, p2y, p3x, p3y, p4x, p4y;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        final_rect.x = (int)(renderer->viewport.x + dstrect->x);
        final_rect.y = (int)(renderer->viewport.y + dstrect->y);
    } else {
        final_rect.x = (int)dstrect->x;
        final_rect.y = (int)dstrect->y;
    }
    final_rect.w = (int)dstrect->w;
    final_rect.h = (int)dstrect->h;

    surface_scaled = SDL_CreateRGBSurface(SDL_SWSURFACE, final_rect.w, final_rect.h, src->format->BitsPerPixel,
                                          src->format->Rmask, src->format->Gmask,
                                          src->format->Bmask, src->format->Amask );
    if (surface_scaled) {
        SDL_GetColorKey(src, &colorkey);
        SDL_SetColorKey(surface_scaled, SDL_TRUE, colorkey);
        tmp_rect = final_rect;
        tmp_rect.x = 0;
        tmp_rect.y = 0;

        retval = SDL_BlitScaled(src, srcrect, surface_scaled, &tmp_rect);
        if (!retval) {
            SDLgfx_rotozoomSurfaceSizeTrig(tmp_rect.w, tmp_rect.h, -angle, &dstwidth, &dstheight, &cangle, &sangle);
            surface_rotated = SDLgfx_rotateSurface(surface_scaled, -angle, dstwidth/2, dstheight/2, GetScaleQuality(), flip & SDL_FLIP_HORIZONTAL, flip & SDL_FLIP_VERTICAL, dstwidth, dstheight, cangle, sangle);
            if(surface_rotated) {
                /* Find out where the new origin is by rotating the four final_rect points around the center and then taking the extremes */
                abscenterx = final_rect.x + (int)center->x;
                abscentery = final_rect.y + (int)center->y;
                /* Compensate the angle inversion to match the behaviour of the other backends */
                sangle = -sangle;

                /* Top Left */
                px = final_rect.x - abscenterx;
                py = final_rect.y - abscentery;
                p1x = px * cangle - py * sangle + abscenterx;
                p1y = px * sangle + py * cangle + abscentery;

                /* Top Right */
                px = final_rect.x + final_rect.w - abscenterx;
                py = final_rect.y - abscentery;
                p2x = px * cangle - py * sangle + abscenterx;
                p2y = px * sangle + py * cangle + abscentery;

                /* Bottom Left */
                px = final_rect.x - abscenterx;
                py = final_rect.y + final_rect.h - abscentery;
                p3x = px * cangle - py * sangle + abscenterx;
                p3y = px * sangle + py * cangle + abscentery;

                /* Bottom Right */
                px = final_rect.x + final_rect.w - abscenterx;
                py = final_rect.y + final_rect.h - abscentery;
                p4x = px * cangle - py * sangle + abscenterx;
                p4y = px * sangle + py * cangle + abscentery;

                tmp_rect.x = (int)MIN(MIN(p1x, p2x), MIN(p3x, p4x));
                tmp_rect.y = (int)MIN(MIN(p1y, p2y), MIN(p3y, p4y));
                tmp_rect.w = dstwidth;
                tmp_rect.h = dstheight;

                retval = SDL_BlitSurface(surface_rotated, NULL, surface, &tmp_rect);
                SDL_FreeSurface(surface_scaled);
                SDL_FreeSurface(surface_rotated);
                return retval;
            }
        }
        return retval;
    }

    return -1;
}
#endif

static int
OS4_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                    Uint32 format, void * pixels, int pitch)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	SDL_Rect final_rect;
	
	struct BitMap *bitmap = OS4_ActivateRenderer(renderer);

	dprintf("Called\n");

	if (!bitmap) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        final_rect.x = renderer->viewport.x + rect->x;
        final_rect.y = renderer->viewport.y + rect->y;
        final_rect.w = rect->w;
        final_rect.h = rect->h;
        rect = &final_rect;
    }

	if (rect->x < 0 || rect->x+rect->w > renderer->window->w ||
		rect->y < 0 || rect->y+rect->h > renderer->window->h) {
		return SDL_SetError("Tried to read outside of surface bounds");
    }

	if (format != SDL_PIXELFORMAT_ARGB8888) {
		return SDL_SetError("Unsupported pixel format");
	}

	data->rastport.BitMap = bitmap;

	data->iGraphics->ReadPixelArray(
		&data->rastport,
		rect->x,
		rect->y,
		pixels,
		0,
		0,
		pitch,
		PIXF_A8R8G8B8,
		rect->w,
		rect->h);

	return 0;
}

static void
OS4_RenderPresent(SDL_Renderer * renderer)
{
    SDL_Window *window = renderer->window;

	//dprintf("Called\n");
	//Uint32 s = SDL_GetTicks();
	
	if (window) {
		//SDL_UpdateWindowSurface(window);
		OS4_RenderData *data = (OS4_RenderData *)renderer->driverdata;
		SDL_WindowData *windowdata = (SDL_WindowData *)window->driverdata;

		struct Window *syswin = windowdata->syswin;

		// TODO: should we take viewport into account?
		// TODO: VSYNC
		if (syswin) {
			data->iLayers->LockLayer(0, syswin->WLayer);
			
			int32 ret = data->iGraphics->BltBitMapTags(
				BLITA_Source, data->bitmap,
				BLITA_DestType, data->target ? BLITT_BITMAP : BLITT_RASTPORT,
				BLITA_Dest, data->target ? (APTR) data->target : (APTR) syswin->RPort, // ?,
				BLITA_DestX, syswin->BorderLeft,
				BLITA_DestY, syswin->BorderTop,
				BLITA_Width, window->w,
				BLITA_Height, window->h,
				TAG_DONE);
			
			data->iLayers->UnlockLayer(syswin->WLayer);

			if (ret != -1) {
				dprintf("BltBitMapTags(): %d\n", ret);
			}
		}
    }
    //dprintf("Took %d\n", SDL_GetTicks() - s);
}

static void
OS4_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
	OS4_TextureData *texturedata = (OS4_TextureData *) texture->driverdata;

	dprintf("Called %p %p\n", renderer, texture);

	if (texturedata) {
		if (texturedata->bitmap) {
			data->iGraphics->FreeBitMap(texturedata->bitmap);
			texturedata->bitmap = NULL;
		}

		SDL_free(texturedata);
	}
}

static void
OS4_DestroyRenderer(SDL_Renderer * renderer)
{
	OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;

	dprintf("Called\n");

	if (data->bitmap) {
		data->iGraphics->FreeBitMap(data->bitmap);
		data->bitmap = NULL;
	}

    SDL_free(data);
    SDL_free(renderer);
}

#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
