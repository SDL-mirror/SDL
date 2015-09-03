/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include "../SDL_sysvideo.h"
#include "../SDL_blit.h"
#include "SDL_os4video.h"
#include "SDL_os4utils.h"
#include "SDL_os4blit.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/Picasso96API.h>
#include <proto/utility.h>
#include <proto/layers.h>

#include <intuition/intuition.h>
#include <libraries/Picasso96.h>

//#define DEBUG

#include "../../main/amigaos4/SDL_os4debug.h"

extern struct GraphicsIFace  *SDL_IGraphics;
extern struct LayersIFace    *SDL_ILayers;
extern struct P96IFace       *SDL_IP96;
extern struct IntuitionIFace *SDL_IIntuition;

#ifdef DEBUG
static char *get_flags_str(Uint32 flags)
{
    static char buffer[256];

    buffer[0] = '\0';

	if (flags & SDL_HWSURFACE)		SDL_strlcat(buffer, "HWSURFACE ", 256);
	if (flags & SDL_SRCCOLORKEY)	SDL_strlcat(buffer, "SRCCOLORKEY ", 256);
	if (flags & SDL_RLEACCELOK)		SDL_strlcat(buffer, "RLEACCELOK ", 256);
	if (flags & SDL_RLEACCEL)		SDL_strlcat(buffer, "RLEACCEL ", 256);
	if (flags & SDL_SRCALPHA)		SDL_strlcat(buffer, "SRCALPHA ", 256);
	if (flags & SDL_PREALLOC)		SDL_strlcat(buffer, "PREALLOC ", 256);

    return buffer;
}
#endif

int os4video_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	int result = -1;

	dprintf("Allocating HW surface %p flags:%s\n", surface, get_flags_str(surface->flags));

	/* Create surface hardware record if not already present */
	if (!surface->hwdata)
		surface->hwdata = SaveAllocPooled(_this->driverdata, sizeof(struct private_hwdata));

	if (surface->hwdata) {
		struct BitMap *friend_bitmap;

		surface->hwdata->type = hwdata_bitmap;
		surface->hwdata->lock = 0;

		/* Determine friend bitmap */
		if (_this->driverdata->scr == NULL)
		{
			/* Windowed mode - use the off-screen buffer's bitmap */
			friend_bitmap = _this->driverdata->offScreenBuffer.bitmap;
		}
		else
		{
			/* Full-screen mode. Use the display's bitmap */
			friend_bitmap = _this->driverdata->win->RPort->BitMap;
		}

		dprintf("Trying to create %dx%dx%d bitmap\n",
				surface->w, surface->h, surface->format->BitsPerPixel);

		surface->hwdata->bm = SDL_IP96->p96AllocBitMap (surface->w,
													surface->h,
													surface->format->BitsPerPixel,
													BMF_MINPLANES | BMF_DISPLAYABLE,
													friend_bitmap,
													0);

		if (surface->hwdata->bm)
		{
			/* Successfully created bitmap */
			dprintf ("Created bitmap %p\n", surface->hwdata->bm);

			surface->flags |= SDL_HWSURFACE | SDL_PREALLOC;
			result = 0;
		}
		else
		{
			/* Failed */
			dprintf ("Failed to create bitmap\n");
			SaveFreePooled(_this->driverdata, surface->hwdata, sizeof(struct private_hwdata));
			surface->hwdata = NULL;
		}
	}
	return result;
}

void os4video_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	dprintf("Freeing HW surface %p\n", surface);

	if (surface)
	{
		/* Check if this is surface allocated by AllocHWSurface */
		if (surface->hwdata && surface->hwdata->type == hwdata_bitmap)
		{
			/* Yes. Free BitMap */
			dprintf("Freeing bitmap %p\n", surface->hwdata->bm);
			SDL_IP96->p96FreeBitMap (surface->hwdata->bm);

			/*  Free surface hardware record */
			SaveFreePooled(_this->driverdata, surface->hwdata, sizeof(struct private_hwdata));
			surface->hwdata = NULL;
		}
	}
	return;
}

int os4video_LockHWSurface(_THIS, SDL_Surface *surface)
{
	int success = -1;
	struct private_hwdata *hwdata = surface->hwdata;

	surface->pixels = (uint8*)0xcccccccc;

	if (hwdata->bm)
	{
		/* We're locking a surface in video memory (either a full-screen hardware display
		 * surface or a hardware off-screen surface). We need to get P96 to lock that the
		 * corresponding bitmap in memory so that we can access the pixels directly
		 */
		hwdata->lock = SDL_IP96->p96LockBitMap(hwdata->bm, (uint8 *)&hwdata->ri, sizeof(hwdata->ri));

		if (hwdata->lock)
		{
			/* Bitmap was successfully locked */
			surface->pitch  = surface->hwdata->ri.BytesPerRow;
			surface->pixels = surface->hwdata->ri.Memory;

			if (surface->hwdata->type == hwdata_display_hw)
			{
				/* In full-screen mode, set up offset to pixels (SDL offsets the surface
				 * when the surface size doesn't match a real resolution, but the value it
				 * sets is wrong, because it doesn't know the surface pitch).
				 */
				if  (surface->flags & SDL_FULLSCREEN)
					surface->offset = _this->offset_y * surface->pitch
									+ _this->offset_x * surface->format->BytesPerPixel;
			}

			/* Done */
			success = 0;
		}
		else
			dprintf ("Failed to lock bitmap:%p\n", surface->hwdata->bm);
	}

	return success;
}

void os4video_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	struct private_hwdata *hwdata = surface->hwdata;

	if (hwdata->bm)
		SDL_IP96->p96UnlockBitMap(surface->hwdata->bm, surface->hwdata->lock);

	surface->hwdata->lock = 0;
	surface->pixels = (uint8*)0xcccccccc;
}

int os4video_FillHWRect(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color)
{
//	dprintf("x=%d y=%d w=%d h=%d color=%08x\n", rect->x, rect->y, rect->w, rect->h, color);

	int    xmin, ymin;
	int    xmax, ymax;

	struct RastPort *rp = NULL;

	static struct RastPort tmpras;
	static int tmpras_initialized = 0;

	if (dst->hwdata && dst->hwdata->bm)
	{
		xmin = rect->x; ymin = rect->y;
		xmax = xmin + rect->w - 1;
		ymax = ymin + rect->h - 1;

		if (dst->hwdata->type == hwdata_display_hw)
		{
			if (!(dst->flags & SDL_DOUBLEBUF))
			{
				rp = _this->driverdata->win->RPort;
			}
		}

		/* We need to set up a rastport for rendering to off-screen surfaces */
		if (!rp)
		{
			if (!tmpras_initialized) {
				SDL_IGraphics->InitRastPort(&tmpras);
				tmpras_initialized = 1;
			}

			tmpras.BitMap = dst->hwdata->bm;
			rp = &tmpras;
		}

		if (dst->format->BitsPerPixel > 8)
		{
			/* Convert hi/true-colour pixel format to ARGB32 format for P96 fill.
		 	 * Bloody stupid if you ask me, 'cos P96 will have to convert it
		 	 * back again */
			struct SDL_PixelFormat *format = dst->format;
			uint32 argb_colour;

			argb_colour = (((color & format->Amask) >> format->Ashift) << (format->Aloss + 24))
						| (((color & format->Rmask) >> format->Rshift) << (format->Rloss + 16))
						| (((color & format->Gmask) >> format->Gshift) << (format->Gloss + 8))
						| (((color & format->Bmask) >> format->Bshift) <<  format->Bloss);

			SDL_IP96->p96RectFill(rp, xmin, ymin, xmax, ymax, argb_colour);
		}
		else
		{
			/* Fall back on graphics lib for palette-mapped surfaces */
			SDL_IGraphics->SetAPen(rp, color);
			SDL_IGraphics->RectFill(rp, xmin, ymin, xmax, ymax);
		}
		return 0;
	}
	else
		return -1;
}

static int os4video_HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect,
			        SDL_Surface *dst, SDL_Rect *dstrect)
{
	dprintf("called\n");

	struct BitMap   *src_bm = src->hwdata->bm;
	struct RenderInfo src_ri={ 0 };

	static struct RastPort dst_rp;
	static int dst_rp_initialized = 0;

	if (!dst_rp_initialized) {
		SDL_IGraphics->InitRastPort(&dst_rp);
		dst_rp_initialized = 1;
	}

	dst_rp.BitMap = dst->hwdata->bm;
	
	if (src_bm)
	{
		LONG src_lock = SDL_IP96->p96LockBitMap(src_bm, (uint8 *)&src_ri, sizeof(src_ri));

		if (src_lock)
		{
			SDL_IP96->p96WritePixelArray(&src_ri,
										 srcrect->x,
										 srcrect->y,
										 &dst_rp,
										 dstrect->x,
										 dstrect->y,
										 srcrect->w,
										 srcrect->h);
	
			SDL_IP96->p96UnlockBitMap(src_bm, src_lock);
		}
		else
			dprintf("Bitmap lock failed\n");
	}
	return 0;
}

int os4video_CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst)
{
	dprintf("src flags:%s dst flags:%s\n", get_flags_str(src->flags), get_flags_str(dst->flags));

	if (src->hwdata && dst->hwdata && !(src->flags & (SDL_SRCALPHA|SDL_SRCCOLORKEY)))
	{
		src->flags |= SDL_HWACCEL;
		src->map->hw_blit = os4video_HWAccelBlit;

		dprintf("Hardware blitting supported\n");

		return 1;
	}
	else
	{
		src->flags &= ~SDL_HWACCEL;

		dprintf("Hardware blitting not supported\n");

		return 0;
	}
}

int os4video_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	struct SDL_PrivateVideoData *hidden = _this->driverdata;
	struct private_hwdata       *hwdata = &hidden->screenHWData;

	if (hwdata->type == hwdata_display_hw)
	{
		struct DoubleBufferData *dbData = &hidden->dbData;

		dprintf("Flipping double-buffered surface\n");

		if (!dbData->SafeToFlip)
		{
			/* If the screen buffer has been flipped previously, then wait
			 * for a message from gfx.lib telling us that the new buffer has
			 * been made visible.
			 */
			IExec->Wait(1 << (dbData->SafeToFlip_MsgPort->mp_SigBit));
			while (IExec->GetMsg(dbData->SafeToFlip_MsgPort) != NULL)
				;
			dbData->SafeToFlip = TRUE;
		}
		/*
		 * Now try to flip the screen buffers
		 */
		if (SDL_IIntuition->ChangeScreenBuffer(hidden->scr, dbData->sb[dbData->currentSB]))
		{
			/* The flip was succesful. Update our pointer to the off-screen buffer */
			dbData->currentSB   = 1 - dbData->currentSB;
			hwdata->bm = dbData->sb[dbData->currentSB]->sb_BitMap;

			/* A successful flip means that we need to wait for a signal before writing
			 * to the new off-screen buffer and to wait for a signal before
			 * flipping again.
			 */
			dbData->SafeToWrite = FALSE;
			dbData->SafeToFlip  = FALSE;
		}
		if (!dbData->SafeToWrite)
		{
			/* If the screen has just been flipped, wait until gfx.lib signals us
			 * that it is safe to write to the new off-screen buffer
			 */
			IExec->Wait(1 << (dbData->SafeToWrite_MsgPort->mp_SigBit));
			while (IExec->GetMsg(dbData->SafeToWrite_MsgPort) != NULL)
				;
			dbData->SafeToWrite = TRUE;
		}
	}

	return 0;
}

void os4video_UpdateRectsFullscreenDB(_THIS, int numrects, SDL_Rect *rects)
{
	// dprintf("*** IMPLEMENT ME ***\n");
	//
	// Nah! Don't bother. This should be a no-op.
}

void os4video_UpdateRectsNone(_THIS, int numrects, SDL_Rect *rects)
{
}


static void os4video_OffscreenHook_8bit (struct Hook *hook, struct RastPort *rp, int *message)
{
	SDL_VideoDevice *_this = (SDL_VideoDevice *)hook->h_Data;
	struct SDL_PrivateVideoData *hidden = _this->driverdata;

	struct Rectangle *rect = (struct Rectangle *)(message + 1);
	uint32 offsetX         = message[3];
	uint32 offsetY         = message[4];

	/* Source bitmap (off-screen) is private, so does not need locking */
	UBYTE *srcMem   = hidden->offScreenBuffer.pixels;
	ULONG  srcPitch = hidden->offScreenBuffer.pitch;

	/* Attempt to lock destination bitmap (screen) */
	struct RenderInfo dst_ri;
	LONG   dst_lock = SDL_IP96->p96LockBitMap(rp->BitMap, (uint8 *)&dst_ri, sizeof(dst_ri));

	if (dst_lock)
	{
		uint32 dst;
		uint32 src;
		uint32 BytesPerPixel;
		int32  line = rect->MinY;

		offsetX -= hidden->win->BorderLeft;
		offsetY -= hidden->win->BorderTop;

		switch (dst_ri.RGBFormat)
		{
	    	case RGBFB_R8G8B8:
        	case RGBFB_B8G8R8:
        		BytesPerPixel = 3;
				break;

			case RGBFB_R5G6B5PC:
        	case RGBFB_B5G6R5PC:
        	case RGBFB_B5G5R5PC:
        	case RGBFB_R5G5B5PC:
			case RGBFB_R5G6B5:
        	case RGBFB_R5G5B5:
				BytesPerPixel = 2;
				break;

        	case RGBFB_A8R8G8B8:
        	case RGBFB_A8B8G8R8:
        	case RGBFB_R8G8B8A8:
        	case RGBFB_B8G8R8A8:
				BytesPerPixel = 4;
				break;
			default:
				BytesPerPixel = 0;
				break;
		}

		dst = (uint32)dst_ri.Memory + dst_ri.BytesPerRow * rect->MinY + BytesPerPixel * rect->MinX;
		src = (uint32)srcMem  + srcPitch * offsetY + offsetX;

		switch (BytesPerPixel)
		{
			case 2:
				while (line <= rect->MaxY)
				{
					cpy_8_16((void *)dst, (void *)src, rect->MaxX - rect->MinX + 1, hidden->offScreenBuffer.palette);
					dst += dst_ri.BytesPerRow;
					src += srcPitch;
					line ++;
             	}
				break;
			case 3:
				while (line <= rect->MaxY)
				{
					cpy_8_24((void *)dst, (void *)src, rect->MaxX - rect->MinX + 1, hidden->offScreenBuffer.palette);
					dst += dst_ri.BytesPerRow;
					src += srcPitch;
					line ++;
             	}
				break;
			case 4:
				while (line <= rect->MaxY)
				{
					cpy_8_32((void *)dst, (void *)src, rect->MaxX - rect->MinX + 1, hidden->offScreenBuffer.palette);
					dst += dst_ri.BytesPerRow;
					src += srcPitch;
					line ++;
				}
				break;
		}

		SDL_IP96->p96UnlockBitMap(rp->BitMap, dst_lock);
	}
	else
		dprintf("Bitmap lock failed\n");
}

#ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
#endif

/*
 * Flush rectanges from off-screen buffer to the screen
 * Special case for palettized off-screen buffers
 */
void os4video_UpdateRectsOffscreen_8bit(_THIS, int numrects, SDL_Rect *rects)
{
#ifdef PROFILE_UPDATE_RECTS
	uint32 start;
	uint32 end;
#endif

	struct SDL_PrivateVideoData *hidden = _this->driverdata;
	struct Window 				*w      = hidden->win;

	/* We don't want Intuition dead-locking us while we try to do this... */
	SDL_ILayers->LockLayerInfo(&w->WScreen->LayerInfo);

	{
		/* Current bounds of inner window */
		const struct Rectangle windowRect = {
			w->BorderLeft,
			w->BorderTop,
			w->Width  - w->BorderRight  - 1,
			w->Height - w->BorderBottom - 1
		};

		struct Rectangle clipRect;
		struct Hook      clipHook;

		const SDL_Rect *r = &rects[0];

		for ( ; numrects > 0; r++, numrects--)
		{
			clipHook.h_Entry = (uint32(*)())os4video_OffscreenHook_8bit;
			clipHook.h_Data  = _this;

			clipRect.MinX    = windowRect.MinX + r->x;
			clipRect.MinY    = windowRect.MinY + r->y;
			clipRect.MaxX    = MIN(windowRect.MaxX, clipRect.MinX + r->w - 1);
			clipRect.MaxY    = MIN(windowRect.MaxY, clipRect.MinY + r->h - 1);

#ifdef PROFILE_UPDATE_RECTS
			start = SDL_GetTicks();
#endif

			SDL_ILayers->DoHookClipRects(&clipHook, w->RPort, &clipRect);

#ifdef PROFILE_UPDATE_RECTS
			end = SDL_GetTicks();
			dprintf("DoHookClipRects took %d microseconds\n", end-start);
#endif
		}
	}

	SDL_ILayers->UnlockLayerInfo(&w->WScreen->LayerInfo);
}

/*
 * Flush rectanges from off-screen buffer to the screen
 */
void os4video_UpdateRectsOffscreen(_THIS, int numrects, SDL_Rect *rects)
{
	struct SDL_PrivateVideoData *hidden = _this->driverdata;
	struct Window               *w      = hidden->win;
	
	/* We don't want our window changing size while we're doing this */
	SDL_ILayers->LockLayer(0, w->WLayer);

	{
		struct RenderInfo dst_ri;
		/* Current dimensions of inner window */
		const struct IBox windowBox = {
			w->BorderLeft, w->BorderTop, w->Width, w->Height
		};
		
		const SDL_Rect *r = &rects[0];

		for ( ; numrects > 0; r++, numrects--)
		{
			/* Blit rect to screen, constraing rect to bounds of inner window */
			SDL_IGraphics->BltBitMapRastPort(hidden->offScreenBuffer.bitmap,
								 r->x,
								 r->y,
								 w->RPort,
								 r->x + windowBox.Left,
								 r->y + windowBox.Top,
								 MIN(r->w, windowBox.Width),
								 MIN(r->h, windowBox.Height),
								 0xC0);
		}
	}

	SDL_ILayers->UnlockLayer(w->WLayer);
}
