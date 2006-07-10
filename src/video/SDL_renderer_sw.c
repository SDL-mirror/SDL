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
#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_sysvideo.h"
#include "SDL_rect_c.h"
#include "SDL_yuv_sw_c.h"


/* SDL surface based renderer implementation */

static SDL_Renderer *SDL_SW_CreateRenderer(SDL_Window * window, Uint32 flags);
static int SDL_SW_CreateTexture(SDL_Renderer * renderer,
                                SDL_Texture * texture);
static int SDL_SW_QueryTexturePixels(SDL_Renderer * renderer,
                                     SDL_Texture * texture, void **pixels,
                                     int *pitch);
static int SDL_SW_SetTexturePalette(SDL_Renderer * renderer,
                                    SDL_Texture * texture,
                                    const SDL_Color * colors, int firstcolor,
                                    int ncolors);
static int SDL_SW_GetTexturePalette(SDL_Renderer * renderer,
                                    SDL_Texture * texture, SDL_Color * colors,
                                    int firstcolor, int ncolors);
static int SDL_SW_UpdateTexture(SDL_Renderer * renderer,
                                SDL_Texture * texture, const SDL_Rect * rect,
                                const void *pixels, int pitch);
static int SDL_SW_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                              const SDL_Rect * rect, int markDirty,
                              void **pixels, int *pitch);
static void SDL_SW_UnlockTexture(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static void SDL_SW_DirtyTexture(SDL_Renderer * renderer,
                                SDL_Texture * texture, int numrects,
                                const SDL_Rect * rects);
static void SDL_SW_SelectRenderTexture(SDL_Renderer * renderer,
                                       SDL_Texture * texture);
static int SDL_SW_RenderFill(SDL_Renderer * renderer, const SDL_Rect * rect,
                             Uint32 color);
static int SDL_SW_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * srcrect,
                             const SDL_Rect * dstrect, int blendMode,
                             int scaleMode);
static int SDL_SW_RenderReadPixels(SDL_Renderer * renderer,
                                   const SDL_Rect * rect, void *pixels,
                                   int pitch);
static int SDL_SW_RenderWritePixels(SDL_Renderer * renderer,
                                    const SDL_Rect * rect, const void *pixels,
                                    int pitch);
static void SDL_SW_RenderPresent(SDL_Renderer * renderer);
static void SDL_SW_DestroyTexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static void SDL_SW_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver SDL_SW_RenderDriver = {
    SDL_SW_CreateRenderer,
    {
     "software",
     (SDL_Renderer_PresentDiscard |
      SDL_Renderer_PresentCopy |
      SDL_Renderer_PresentFlip2 |
      SDL_Renderer_PresentFlip3 | SDL_Renderer_RenderTarget),
     (SDL_TextureBlendMode_None |
      SDL_TextureBlendMode_Mask | SDL_TextureBlendMode_Blend),
     (SDL_TextureScaleMode_None | SDL_TextureScaleMode_Fast),
     11,
     {
      SDL_PixelFormat_Index8,
      SDL_PixelFormat_RGB555,
      SDL_PixelFormat_RGB565,
      SDL_PixelFormat_RGB888,
      SDL_PixelFormat_BGR888,
      SDL_PixelFormat_ARGB8888,
      SDL_PixelFormat_RGBA8888,
      SDL_PixelFormat_ABGR8888,
      SDL_PixelFormat_BGRA8888,
      SDL_PixelFormat_YUY2,
      SDL_PixelFormat_UYVY},
     0,
     0}
};

typedef struct
{
    int current_screen;
    SDL_Surface *screens[3];
    SDL_Surface *target;
    SDL_Renderer *renderer;
    SDL_DirtyRectList dirty;
} SDL_SW_RenderData;

SDL_Renderer *
SDL_SW_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_DisplayMode *displayMode = &display->current_mode;
    SDL_Renderer *renderer;
    SDL_SW_RenderData *data;
    int i, n;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks
        (displayMode->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown display format");
        return NULL;
    }

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (SDL_SW_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_SW_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);

    renderer->CreateTexture = SDL_SW_CreateTexture;
    renderer->QueryTexturePixels = SDL_SW_QueryTexturePixels;
    renderer->SetTexturePalette = SDL_SW_SetTexturePalette;
    renderer->GetTexturePalette = SDL_SW_GetTexturePalette;
    renderer->UpdateTexture = SDL_SW_UpdateTexture;
    renderer->LockTexture = SDL_SW_LockTexture;
    renderer->UnlockTexture = SDL_SW_UnlockTexture;
    renderer->DirtyTexture = SDL_SW_DirtyTexture;
    renderer->SelectRenderTexture = SDL_SW_SelectRenderTexture;
    renderer->RenderFill = SDL_SW_RenderFill;
    renderer->RenderCopy = SDL_SW_RenderCopy;
    renderer->RenderReadPixels = SDL_SW_RenderReadPixels;
    renderer->RenderWritePixels = SDL_SW_RenderWritePixels;
    renderer->RenderPresent = SDL_SW_RenderPresent;
    renderer->DestroyTexture = SDL_SW_DestroyTexture;
    renderer->DestroyRenderer = SDL_SW_DestroyRenderer;
    renderer->info = SDL_SW_RenderDriver.info;
    renderer->window = window->id;
    renderer->driverdata = data;

    renderer->info.flags = SDL_Renderer_RenderTarget;

    if (flags & SDL_Renderer_PresentFlip2) {
        renderer->info.flags |= SDL_Renderer_PresentFlip2;
        n = 2;
    } else if (flags & SDL_Renderer_PresentFlip3) {
        renderer->info.flags |= SDL_Renderer_PresentFlip3;
        n = 3;
    } else {
        renderer->info.flags |= SDL_Renderer_PresentCopy;
        n = 1;
    }
    for (i = 0; i < n; ++i) {
        data->screens[i] =
            SDL_CreateRGBSurface(0, window->w, window->h, bpp, Rmask, Gmask,
                                 Bmask, Amask);
        if (!data->screens[i]) {
            SDL_SW_DestroyRenderer(renderer);
            return NULL;
        }
        SDL_SetSurfacePalette(data->screens[i], display->palette);
    }
    data->current_screen = 0;
    data->target = data->screens[0];

    /* Find a render driver that we can use to display data */
    for (i = 0; i < display->num_render_drivers; ++i) {
        SDL_RenderDriver *driver = &display->render_drivers[i];
        if (driver->info.name != SDL_SW_RenderDriver.info.name) {
            data->renderer =
                driver->CreateRenderer(window, SDL_Renderer_PresentDiscard);
            if (data->renderer) {
                break;
            }
        }
    }
    if (i == display->num_render_drivers) {
        SDL_SW_DestroyRenderer(renderer);
        SDL_SetError("Couldn't find display render driver");
        return NULL;
    }
    return renderer;
}

static int
SDL_SW_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        if (texture->access == SDL_TextureAccess_Render) {
            SDL_SetError("Rendering to YUV format textures is not supported");
            return -1;
        }
        texture->driverdata = SDL_SW_CreateYUVTexture(texture);
    } else {
        int bpp;
        Uint32 Rmask, Gmask, Bmask, Amask;

        if (!SDL_PixelFormatEnumToMasks
            (texture->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
            SDL_SetError("Unknown texture format");
            return -1;
        }

        texture->driverdata =
            SDL_CreateRGBSurface(0, texture->w, texture->h, bpp, Rmask, Gmask,
                                 Bmask, Amask);
    }

    if (!texture->driverdata) {
        return -1;
    }
    return 0;
}

static int
SDL_SW_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                          void **pixels, int *pitch)
{
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        return SDL_SW_QueryYUVTexturePixels((SDL_SW_YUVTexture *) texture->
                                            driverdata, pixels, pitch);
    } else {
        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

        *pixels = surface->pixels;
        *pitch = surface->pitch;
        return 0;
    }
}

static int
SDL_SW_SetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Color * colors, int firstcolor,
                         int ncolors)
{
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        SDL_SetError("YUV textures don't have a palette");
        return -1;
    } else {
        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

        return SDL_SetPaletteColors(surface->format->palette, colors,
                                    firstcolor, ncolors);
    }
}

static int
SDL_SW_GetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                         SDL_Color * colors, int firstcolor, int ncolors)
{
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        SDL_SetError("YUV textures don't have a palette");
        return -1;
    } else {
        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

        SDL_memcpy(colors, &surface->format->palette->colors[firstcolor],
                   ncolors * sizeof(*colors));
        return 0;
    }
}

static int
SDL_SW_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                     const SDL_Rect * rect, const void *pixels, int pitch)
{
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        return SDL_SW_UpdateYUVTexture((SDL_SW_YUVTexture *) texture->
                                       driverdata, rect, pixels, pitch);
    } else {
        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
        Uint8 *src, *dst;
        int row;
        size_t length;

        src = (Uint8 *) pixels;
        dst =
            (Uint8 *) surface->pixels + rect->y * surface->pitch +
            rect->x * surface->format->BytesPerPixel;
        length = rect->w * surface->format->BytesPerPixel;
        for (row = 0; row < rect->h; ++row) {
            SDL_memcpy(dst, src, length);
            src += pitch;
            dst += surface->pitch;
        }
        return 0;
    }
}

static int
SDL_SW_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                   const SDL_Rect * rect, int markDirty, void **pixels,
                   int *pitch)
{
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        return SDL_SW_LockYUVTexture((SDL_SW_YUVTexture *) texture->
                                     driverdata, rect, markDirty, pixels,
                                     pitch);
    } else {
        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

        *pixels =
            (void *) ((Uint8 *) surface->pixels + rect->y * surface->pitch +
                      rect->x * surface->format->BytesPerPixel);
        *pitch = surface->pitch;
        return 0;
    }
}

static void
SDL_SW_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        SDL_SW_UnlockYUVTexture((SDL_SW_YUVTexture *) texture->driverdata);
    }
}

static void
SDL_SW_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                    int numrects, const SDL_Rect * rects)
{
}

static void
SDL_SW_SelectRenderTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_SW_RenderData *data = (SDL_SW_RenderData *) renderer->driverdata;

    if (texture) {
        data->target = (SDL_Surface *) texture->driverdata;
    } else {
        data->target = data->screens[data->current_screen];
    }
}

static int
SDL_SW_RenderFill(SDL_Renderer * renderer, const SDL_Rect * rect,
                  Uint32 color)
{
    SDL_SW_RenderData *data = (SDL_SW_RenderData *) renderer->driverdata;
    SDL_Rect real_rect = *rect;
    Uint8 r, g, b, a;

    SDL_AddDirtyRect(&data->dirty, rect);

    a = (Uint8) ((color >> 24) & 0xFF);
    r = (Uint8) ((color >> 16) & 0xFF);
    g = (Uint8) ((color >> 8) & 0xFF);
    b = (Uint8) (color & 0xFF);
    color = SDL_MapRGBA(data->target->format, r, g, b, a);

    return SDL_FillRect(data->target, &real_rect, color);
}

static int
SDL_SW_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * srcrect, const SDL_Rect * dstrect,
                  int blendMode, int scaleMode)
{
    SDL_SW_RenderData *data = (SDL_SW_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);

    SDL_AddDirtyRect(&data->dirty, dstrect);

    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        SDL_Surface *target = data->target;
        void *pixels =
            (Uint8 *) target->pixels + dstrect->y * target->pitch +
            dstrect->x * target->format->BytesPerPixel;
        return SDL_SW_CopyYUVToRGB((SDL_SW_YUVTexture *) texture->driverdata,
                                   srcrect, display->current_mode.format,
                                   dstrect->w, dstrect->h, pixels,
                                   target->pitch);
    } else {
        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
        SDL_Rect real_srcrect = *srcrect;
        SDL_Rect real_dstrect = *dstrect;

        if (blendMode &
            (SDL_TextureBlendMode_Mask | SDL_TextureBlendMode_Blend)) {
            SDL_SetAlpha(surface, SDL_SRCALPHA, 0);
        } else {
            SDL_SetAlpha(surface, 0, 0);
        }
        if (scaleMode != SDL_TextureScaleMode_None &&
            (srcrect->w != dstrect->w || srcrect->h != dstrect->h)) {
            return SDL_SoftStretch(surface, &real_srcrect, data->target,
                                   &real_dstrect);
        } else {
            return SDL_LowerBlit(surface, &real_srcrect, data->target,
                                 &real_dstrect);
        }
    }
}

static int
SDL_SW_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                        void *pixels, int pitch)
{
    SDL_SW_RenderData *data = (SDL_SW_RenderData *) renderer->driverdata;
    SDL_Surface *surface = data->target;
    Uint8 *src, *dst;
    int row;
    size_t length;

    src =
        (Uint8 *) surface->pixels + rect->y * surface->pitch +
        rect->x * surface->format->BytesPerPixel;
    dst = (Uint8 *) pixels;
    length = rect->w * surface->format->BytesPerPixel;
    for (row = 0; row < rect->h; ++row) {
        SDL_memcpy(dst, src, length);
        src += surface->pitch;
        dst += pitch;
    }
    return 0;
}

static int
SDL_SW_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                         const void *pixels, int pitch)
{
    SDL_SW_RenderData *data = (SDL_SW_RenderData *) renderer->driverdata;
    SDL_Surface *surface = data->target;
    Uint8 *src, *dst;
    int row;
    size_t length;

    SDL_AddDirtyRect(&data->dirty, rect);

    src = (Uint8 *) pixels;
    dst =
        (Uint8 *) surface->pixels + rect->y * surface->pitch +
        rect->x * surface->format->BytesPerPixel;
    length = rect->w * surface->format->BytesPerPixel;
    for (row = 0; row < rect->h; ++row) {
        SDL_memcpy(dst, src, length);
        src += pitch;
        dst += surface->pitch;
    }
    return 0;
}

static void
SDL_SW_RenderPresent(SDL_Renderer * renderer)
{
    SDL_SW_RenderData *data = (SDL_SW_RenderData *) renderer->driverdata;
    SDL_Surface *surface = data->screens[data->current_screen];
    SDL_DirtyRect *dirty;
    int new_screen;

    /* Send the data to the display */
    for (dirty = data->dirty.list; dirty; dirty = dirty->next) {
        void *pixels =
            (void *) ((Uint8 *) surface->pixels +
                      dirty->rect.y * surface->pitch +
                      dirty->rect.x * surface->format->BytesPerPixel);
        data->renderer->RenderWritePixels(data->renderer, &dirty->rect,
                                          pixels, surface->pitch);
    }
    SDL_ClearDirtyRects(&data->dirty);
    data->renderer->RenderPresent(data->renderer);


    /* Update the flipping chain, if any */
    if (renderer->info.flags & SDL_Renderer_PresentFlip2) {
        new_screen = (data->current_screen + 1) % 2;
    } else if (renderer->info.flags & SDL_Renderer_PresentFlip3) {
        new_screen = (data->current_screen + 1) % 3;
    } else {
        new_screen = 0;
    }
    if (data->target == data->screens[data->current_screen]) {
        data->target = data->screens[new_screen];
    }
    data->current_screen = new_screen;
}

static void
SDL_SW_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        SDL_SW_DestroyYUVTexture((SDL_SW_YUVTexture *) texture->driverdata);
    } else {
        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

        SDL_FreeSurface(surface);
    }
}

static void
SDL_SW_DestroyRenderer(SDL_Renderer * renderer)
{
    SDL_SW_RenderData *data = (SDL_SW_RenderData *) renderer->driverdata;
    int i;

    if (data) {
        for (i = 0; i < SDL_arraysize(data->screens); ++i) {
            if (data->screens[i]) {
                SDL_FreeSurface(data->screens[i]);
            }
        }
        SDL_FreeDirtyRects(&data->dirty);
        SDL_free(data);
    }
    SDL_free(renderer);
}

/* vi: set ts=4 sw=4 expandtab: */
