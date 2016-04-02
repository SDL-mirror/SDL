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

#include "SDL_render_compositing.h"
#include "SDL_rc_texture.h"

#include <proto/graphics.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static SDL_bool
OS4_IsBlendModeSupported(SDL_BlendMode mode)
{
    switch (mode) {
        case SDL_BLENDMODE_NONE:
        case SDL_BLENDMODE_BLEND:
        case SDL_BLENDMODE_ADD:
            //dprintf("Texture blend mode: %d\n", mode);
            return SDL_TRUE;
        default:
            dprintf("Not supported blend mode %d\n", mode);
            return SDL_FALSE;
    }
}

int
OS4_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;
    OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
    OS4_TextureData *texturedata;

    if (texture->format != SDL_PIXELFORMAT_ARGB8888) {
        return SDL_SetError("Not supported texture format");
    }

    if (!SDL_PixelFormatEnumToMasks
        (texture->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        return SDL_SetError("Unknown texture format");
    }

    //dprintf("Allocation VRAM bitmap %d*%d*%d for texture\n", texture->w, texture->h, bpp);

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
        TAG_DONE);

    if (!texturedata->bitmap) {
        dprintf("Failed to allocate bitmap\n");
        SDL_free(texturedata);
        return -1;
    }

    /* Check texture parameters just for debug */
	//OS4_IsColorModEnabled(texture->r, texture->g, texture->b);
    OS4_IsBlendModeSupported(texture->blendMode);

    texture->driverdata = texturedata;

    return 0;
}

int
OS4_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
	//OS4_IsColorModEnabled(texture->r, texture->g, texture->b);

    return 0;
}

int
OS4_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    dprintf("Texture alpha %d\n", texture->a);
    return 0;
}

int
OS4_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return OS4_IsBlendModeSupported(texture->blendMode) ? 0 : -1;
}

int
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

int
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
                  rect->x * 4);

        *pitch = bytesperrow;

        return 0;
    } else {
        dprintf("Lock failed\n");
        return -1;
    }
}

void
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

int
OS4_SetRenderTarget(SDL_Renderer * renderer, SDL_Texture * texture)
{
    OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;

    if (texture) {
        OS4_TextureData *texturedata = (OS4_TextureData *) texture->driverdata;
        data->target = texturedata->bitmap;

        //dprintf("Render target texture %p (bitmap %p)\n", texture, data->target);
    } else {
        data->target = data->bitmap;
        //dprintf("Render target window\n");
    }
    return 0;
}

void
OS4_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
    OS4_TextureData *texturedata = (OS4_TextureData *) texture->driverdata;

    if (texturedata) {
        if (texturedata->bitmap) {
            //dprintf("Freeing texture bitmap %p\n", texturedata->bitmap);

            data->iGraphics->FreeBitMap(texturedata->bitmap);
            texturedata->bitmap = NULL;
        }

        SDL_free(texturedata);
    }
}

#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */

