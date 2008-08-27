/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include <stdio.h>
#include <stdlib.h>
#include <nds.h>
#include <nds/arm9/video.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/trig_lut.h>

#include "SDL_config.h"

#include "SDL_video.h"
#include "../SDL_sysvideo.h"
#include "../SDL_yuv_sw_c.h"
#include "../SDL_renderer_sw.h"

/* NDS sprite-related functions */
#define SPRITE_DMA_CHANNEL 3
#define SPRITE_ANGLE_MASK 0x01FF

void
NDS_OAM_Update(tOAM * oam, int sub)
{
    DC_FlushAll();
    dmaCopyHalfWords(SPRITE_DMA_CHANNEL, oam->spriteBuffer,
                     sub ? OAM_SUB : OAM, SPRITE_COUNT * sizeof(SpriteEntry));
}

void
NDS_OAM_RotateSprite(SpriteRotation * spriteRotation, u16 angle)
{
    s16 s = SIN[angle & SPRITE_ANGLE_MASK] >> 4;
    s16 c = COS[angle & SPRITE_ANGLE_MASK] >> 4;

    spriteRotation->hdx = c;
    spriteRotation->hdy = s;
    spriteRotation->vdx = -s;
    spriteRotation->vdy = c;
}

void
NDS_OAM_Init(tOAM * oam, int sub)
{
    int i;
    for (i = 0; i < SPRITE_COUNT; i++) {
        oam->spriteBuffer[i].attribute[0] = ATTR0_DISABLED;
        oam->spriteBuffer[i].attribute[1] = 0;
        oam->spriteBuffer[i].attribute[2] = 0;
    }
    for (i = 0; i < MATRIX_COUNT; i++) {
        NDS_OAM_RotateSprite(&(oam->matrixBuffer[i]), 0);
    }
    swiWaitForVBlank();
    NDS_OAM_Update(oam, sub);
}

void
NDS_OAM_HideSprite(SpriteEntry * spriteEntry)
{
    spriteEntry->isRotoscale = 0;
    spriteEntry->isHidden = 1;
}

void
NDS_OAM_ShowSprite(SpriteEntry * spriteEntry, int affine, int double_bound)
{
    if (affine) {
        spriteEntry->isRotoscale = 1;
        spriteEntry->rsDouble = double_bound;
    } else {
        spriteEntry->isHidden = 0;
    }
}


/* SDL NDS renderer implementation */

static SDL_Renderer *NDS_CreateRenderer(SDL_Window * window, Uint32 flags);
static int NDS_ActivateRenderer(SDL_Renderer * renderer);
static int NDS_DisplayModeChanged(SDL_Renderer * renderer);
static int NDS_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int NDS_QueryTexturePixels(SDL_Renderer * renderer,
                                  SDL_Texture * texture, void **pixels,
                                  int *pitch);
static int NDS_SetTexturePalette(SDL_Renderer * renderer,
                                 SDL_Texture * texture,
                                 const SDL_Color * colors, int firstcolor,
                                 int ncolors);
static int NDS_GetTexturePalette(SDL_Renderer * renderer,
                                 SDL_Texture * texture, SDL_Color * colors,
                                 int firstcolor, int ncolors);
static int NDS_SetTextureColorMod(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int NDS_SetTextureAlphaMod(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int NDS_SetTextureBlendMode(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int NDS_SetTextureScaleMode(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int NDS_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * rect, const void *pixels,
                             int pitch);
static int NDS_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * rect, int markDirty,
                           void **pixels, int *pitch);
static void NDS_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void NDS_DirtyTexture(SDL_Renderer * renderer,
                             SDL_Texture * texture, int numrects,
                             const SDL_Rect * rects);
static int NDS_RenderFill(SDL_Renderer * renderer, Uint8 r, Uint8 g,
                          Uint8 b, Uint8 a, const SDL_Rect * rect);
static int NDS_RenderCopy(SDL_Renderer * renderer,
                          SDL_Texture * texture,
                          const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static void NDS_RenderPresent(SDL_Renderer * renderer);
static void NDS_DestroyTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static void NDS_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver NDS_RenderDriver = {
    NDS_CreateRenderer,
    {"nds",                     /* char* name */
     (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTDISCARD | SDL_RENDERER_PRESENTVSYNC),  /* u32 flags */
     (SDL_TEXTUREMODULATE_NONE),        /* u32 mod_modes */
     (SDL_TEXTUREBLENDMODE_MASK),       /* u32 blend_modes */
     (SDL_TEXTURESCALEMODE_FAST),       /* u32 scale_modes */
     3,                         /* u32 num_texture_formats */
     {
      SDL_PIXELFORMAT_INDEX8,
      SDL_PIXELFORMAT_ABGR1555,
      SDL_PIXELFORMAT_BGR555,
      },                        /* u32 texture_formats[20] */
     (256),                     /* int max_texture_width */
     (256),                     /* int max_texture_height */
     }
};

typedef struct
{
    bg_attribute *bg;           /* backgrounds */
    tOAM oam_copy;              /* sprites */
    u8 bg_taken[4];
    int sub;
} NDS_RenderData;

typedef struct
{
    enum
    { NDSTX_BG, NDSTX_SPR } type;       /* represented in a bg or sprite. */
    int hw_index;               /* sprite: index in the OAM. /  bg: 2 or 3. */
    struct
    {
        int hdx, hdy, vdx, vdy; /* affine transformation, used for scaling. */
        int pitch, bpp;         /* some useful info */
    } dim;
    u16 *vram_pixels;           /* where the pixel data is stored (a pointer into VRAM) */
    u16 *vram_palette;          /* where the palette data is stored if it's indexed. */
    /*int size; */
} NDS_TextureData;



SDL_Renderer *
NDS_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_DisplayMode *displayMode = &display->current_mode;
    SDL_Renderer *renderer;
    NDS_RenderData *data;
    int i, n;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks(displayMode->format, &bpp,
                                    &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown display format");
        return NULL;
    }
    switch (displayMode->format) {
    case SDL_PIXELFORMAT_INDEX8:
    case SDL_PIXELFORMAT_ABGR1555:
    case SDL_PIXELFORMAT_BGR555:
        /* okay */
        break;
    case SDL_PIXELFORMAT_RGB555:
    case SDL_PIXELFORMAT_RGB565:
    case SDL_PIXELFORMAT_ARGB1555:
        /* we'll take these too for now */
        break;
    default:
        SDL_SetError("Warning: wrong display format for NDS!\n");
        break;
    }

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (NDS_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        NDS_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);

    renderer->RenderFill = NDS_RenderFill;
    renderer->RenderCopy = NDS_RenderCopy;
    renderer->RenderPresent = NDS_RenderPresent;
    renderer->DestroyRenderer = NDS_DestroyRenderer;
    renderer->info.name = NDS_RenderDriver.info.name;
    renderer->info.flags = 0;
    renderer->window = window->id;
    renderer->driverdata = data;
    renderer->CreateTexture = NDS_CreateTexture;
    renderer->QueryTexturePixels = NDS_QueryTexturePixels;
    renderer->SetTexturePalette = NDS_SetTexturePalette;
    renderer->GetTexturePalette = NDS_GetTexturePalette;
    renderer->SetTextureColorMod = NDS_SetTextureColorMod;
    renderer->SetTextureAlphaMod = NDS_SetTextureAlphaMod;
    renderer->SetTextureBlendMode = NDS_SetTextureBlendMode;
    renderer->SetTextureScaleMode = NDS_SetTextureScaleMode;
    renderer->UpdateTexture = NDS_UpdateTexture;
    renderer->LockTexture = NDS_LockTexture;
    renderer->UnlockTexture = NDS_UnlockTexture;
    renderer->DirtyTexture = NDS_DirtyTexture;
    renderer->DestroyTexture = NDS_DestroyTexture;

    renderer->info.mod_modes = NDS_RenderDriver.info.mod_modes;
    renderer->info.blend_modes = NDS_RenderDriver.info.blend_modes;
    renderer->info.scale_modes = NDS_RenderDriver.info.scale_modes;
    renderer->info.num_texture_formats =
        NDS_RenderDriver.info.num_texture_formats;
    SDL_memcpy(renderer->info.texture_formats,
               NDS_RenderDriver.info.texture_formats,
               sizeof(renderer->info.texture_formats));
    renderer->info.max_texture_width =
        NDS_RenderDriver.info.max_texture_width;
    renderer->info.max_texture_height =
        NDS_RenderDriver.info.max_texture_height;

    data->sub = 0;              /* TODO: this is hard-coded to the "main" screen.
                                   figure out how to detect whether to set it to
                                   "sub" screen.  window->id, perhaps? */
    if (!data->sub) {
        data->bg = &BACKGROUND;
    } else {
        data->bg = &BACKGROUND_SUB;
    }
    data->bg_taken[2] = data->bg_taken[3] = 0;

    NDS_OAM_Init(&(data->oam_copy), data->sub); /* init sprites. */

    return renderer;
}

static int
NDS_ActivateRenderer(SDL_Renderer * renderer)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;

    return 0;
}

static int
NDS_DisplayModeChanged(SDL_Renderer * renderer)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;

    return 0;
}

static int
NDS_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    NDS_TextureData *txdat = NULL;
    int i;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks
        (texture->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown texture format");
        return -1;
    }

    /* conditional statements on w/h to place it as bg/sprite
       depending on which one it fits. */
    if (texture->w <= 64 && texture->h <= 64) {
        int whichspr = -1;
        printf("Tried to make a sprite.\n");
        txdat->type = NDSTX_SPR;
        for (i = 0; i < SPRITE_COUNT; ++i) {
            if (data->oam_copy.spriteBuffer[i].attribute[0] & ATTR0_DISABLED) {
                whichspr = i;
                break;
            }
        }
        if (whichspr >= 0) {
            SpriteEntry *sprent = &(data->oam_copy.spriteBuffer[whichspr]);
            int maxside = texture->w > texture->h ? texture->w : texture->h;
            int pitch;

            texture->driverdata = SDL_calloc(1, sizeof(NDS_TextureData));
            txdat = (NDS_TextureData *) texture->driverdata;
            if (!txdat) {
                SDL_OutOfMemory();
                return -1;
            }

            sprent->objMode = OBJMODE_BITMAP;
            sprent->posX = 0;
            sprent->posY = 0;
            sprent->colMode = OBJCOLOR_16;      /* OBJCOLOR_256 for INDEX8 */

            /* the first 32 sprites get transformation matrices.
               first come, first served */
            if (whichspr < MATRIX_COUNT) {
                sprent->isRotoscale = 1;
                sprent->rsMatrixIdx = whichspr;
            }

            /* containing shape (square or 2:1 rectangles) */
            sprent->objShape = OBJSHAPE_SQUARE;
            if (texture->w / 2 >= texture->h) {
                sprent->objShape = OBJSHAPE_WIDE;
            } else if (texture->h / 2 >= texture->w) {
                sprent->objShape = OBJSHAPE_TALL;
            }

            /* size in pixels */
            /* FIXME: "pitch" is hardcoded for 2bytes per pixel. */
            sprent->objSize = OBJSIZE_64;
            pitch = 128;
            if (maxside <= 8) {
                sprent->objSize = OBJSIZE_8;
                pitch = 16;
            } else if (maxside <= 16) {
                sprent->objSize = OBJSIZE_16;
                pitch = 32;
            } else if (maxside <= 32) {
                sprent->objSize = OBJSIZE_32;
                pitch = 64;
            }

            /* FIXME: this is hard-coded and will obviously only work for one
               sprite-texture.  tells it to look at the beginning of SPRITE_GFX
               for its pixels. */
            sprent->tileIdx = 0;

            /* now for the texture data */
            txdat->type = NDSTX_SPR;
            txdat->hw_index = whichspr;
            txdat->dim.hdx = 0x100;
            txdat->dim.hdy = 0;
            txdat->dim.vdx = 0;
            txdat->dim.vdy = 0x100;
            txdat->dim.pitch = pitch;
            txdat->dim.bpp = bpp;
            txdat->vram_pixels = (u16 *) (data->sub ? SPRITE_GFX_SUB : SPRITE_GFX);     /* FIXME: use tileIdx*boundary
                                                                                           to point to proper location */
        } else {
            SDL_SetError("Out of NDS sprites.");
        }
    } else if (texture->w <= 256 && texture->h <= 256) {
        int whichbg = -1, base = 0;
        if (!data->bg_taken[2]) {
            whichbg = 2;
        } else if (!data->bg_taken[3]) {
            whichbg = 3;
            base = 4;
        }
        if (whichbg >= 0) {
            texture->driverdata = SDL_calloc(1, sizeof(NDS_TextureData));
            txdat = (NDS_TextureData *) texture->driverdata;
            if (!txdat) {
                SDL_OutOfMemory();
                return -1;
            }

            /* this is hard-coded to being 256x256 ABGR1555 for now. */
            data->bg->control[whichbg] = (bpp == 8) ?
                BG_BMP8_256x256 : BG_BMP16_256x256;

            data->bg->control[whichbg] |= BG_BMP_BASE(base);

            data->bg->scroll[whichbg].x = 0;
            data->bg->scroll[whichbg].y = 0;

            txdat->type = NDSTX_BG;
            txdat->hw_index = whichbg;
            txdat->dim.hdx = 0x100;
            txdat->dim.hdy = 0;
            txdat->dim.vdx = 0;
            txdat->dim.vdy = 0x100;
            txdat->dim.pitch = 512;
            txdat->dim.bpp = bpp;
            txdat->vram_pixels = (u16 *) (data->sub ?
                                          BG_BMP_RAM_SUB(base) :
                                          BG_BMP_RAM(base));

            /*txdat->size = txdat->dim.pitch * texture->h; */
        } else {
            SDL_SetError("Out of NDS backgrounds.");
        }
    } else {
        SDL_SetError("Texture too big for NDS hardware.");
    }

    if (!texture->driverdata) {
        return -1;
    }

    return 0;
}

static int
NDS_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                       void **pixels, int *pitch)
{
    NDS_TextureData *txdat = (NDS_TextureData *) texture->driverdata;
    *pixels = txdat->vram_pixels;
    *pitch = txdat->dim.pitch;
    return 0;
}

static int
NDS_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * rect, const void *pixels, int pitch)
{
    NDS_TextureData *txdat;
    Uint8 *src, *dst;
    int row;
    size_t length;

    txdat = (NDS_TextureData *) texture->driverdata;

    src = (Uint8 *) pixels;
    dst =
        (Uint8 *) txdat->vram_pixels + rect->y * txdat->dim.pitch +
        rect->x * ((txdat->dim.bpp + 1) / 8);
    length = rect->w * ((txdat->dim.bpp + 1) / 8);

    if (rect->w == texture->w) {
        dmaCopy(src, dst, length * rect->h);
    } else {
        for (row = 0; row < rect->h; ++row) {
            dmaCopy(src, dst, length);
            src += pitch;
            dst += txdat->dim.pitch;
        }
    }

    return 0;
}

static int
NDS_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * rect, int markDirty, void **pixels,
                int *pitch)
{
    NDS_TextureData *txdat = (NDS_TextureData *) texture->driverdata;

    *pixels = (void *) ((u8 *) txdat->vram_pixels + rect->y
                        * txdat->dim.pitch +
                        rect->x * ((txdat->dim.bpp + 1) / 8));
    *pitch = txdat->dim.pitch;

    return 0;
}

static void
NDS_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    /* stub! */
}

static void
NDS_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 int numrects, const SDL_Rect * rects)
{
    /* stub! */
}

static int
NDS_RenderFill(SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b,
               Uint8 a, const SDL_Rect * rect)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    SDL_Rect real_rect = *rect;
    u16 color;
    int i, j;

    color = RGB8(r, g, b);      /* <-- macro in libnds that makes an ARGB1555 pixel */
    /* TODO: make a single-color sprite and stretch it.
       calculate the "HDX" width modifier of the sprite by:
       let S be the actual sprite's width (like, 32 pixels for example)
       let R be the rectangle's width (maybe 50 pixels)
       HDX = (R<<8) / S;
       (it's fixed point, hence the bit shift.  same goes for vertical.
       be sure to use 32-bit int's for the bit shift before the division!)
     */

    return 0;
}

static int
NDS_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    NDS_TextureData *txdat = (NDS_TextureData *) texture->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    int i;
    int bpp = SDL_BYTESPERPIXEL(texture->format);
    int pitch = txdat->dim.pitch;

    if (txdat->type == NDSTX_BG) {
        bg_rotation *bgrot = (txdat->hw_index == 2) ?
            &(data->bg->bg2_rotation) : &(data->bg->bg3_rotation);
        bgrot->xdx = txdat->dim.hdx;
        bgrot->xdy = txdat->dim.hdy;
        bgrot->ydx = txdat->dim.vdx;
        bgrot->ydy = txdat->dim.vdy;
        bgrot->centerX = 0;
        bgrot->centerY = 0;

        data->bg->scroll[txdat->hw_index].x = dstrect->x;
        data->bg->scroll[txdat->hw_index].y = dstrect->y;
    } else {
        /* sprites not fully implemented yet */
        SpriteEntry *spr = &(data->oam_copy.spriteBuffer[txdat->hw_index]);
        spr->posX = dstrect->x;
        spr->posY = dstrect->y;
        if (txdat->hw_index < MATRIX_COUNT && spr->isRotoscale) {
            SpriteRotation *sprot =
                &(data->oam_copy.matrixBuffer[txdat->hw_index]);
            sprot->hdx = txdat->dim.hdx;
            sprot->hdy = txdat->dim.hdy;
            sprot->vdx = txdat->dim.vdx;
            sprot->vdy = txdat->dim.vdy;
        }
    }

    return 0;
}


static void
NDS_RenderPresent(SDL_Renderer * renderer)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);

    /* update sprites */
    NDS_OAM_Update(&(data->oam_copy), data->sub);
    /* vsync for NDS */
    if (renderer->info.flags & SDL_RENDERER_PRESENTVSYNC) {
        swiWaitForVBlank();
    }
}

static void
NDS_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    NDS_TextureData *txdat = texture->driverdata;
    /* free anything else allocated for texture */
    SDL_free(txdat);
}

static void
NDS_DestroyRenderer(SDL_Renderer * renderer)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    int i;

    if (data) {
        /* free anything else relevant if anything else is allocated. */
        SDL_free(data);
    }
    SDL_free(renderer);
}

static int
NDS_SetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                      const SDL_Color * colors, int firstcolor, int ncolors)
{
    NDS_TextureData *txdat = (NDS_TextureData *) texture->driverdata;
    /* set 8-bit modes in the background control registers
       for backgrounds, BGn_CR |= BG_256_COLOR */

    return 0;
}

static int
NDS_GetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                      SDL_Color * colors, int firstcolor, int ncolors)
{
    NDS_TextureData *txdat = (NDS_TextureData *) texture->driverdata;
    /* stub! */
    return 0;
}

static int
NDS_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    /* stub! */
    return 0;
}

static int
NDS_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    /* stub! */
    return 0;
}

static int
NDS_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    /* stub! */
    return 0;
}

static int
NDS_SetTextureScaleMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    /* stub! (note: NDS hardware scaling is nearest neighbor.) */
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
