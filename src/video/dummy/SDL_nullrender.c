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
#include "../SDL_sysvideo.h"


/* SDL surface based renderer implementation */

static SDL_Renderer *SDL_DUMMY_CreateRenderer(SDL_Window * window,
                                              Uint32 flags);
static int SDL_DUMMY_CreateTexture(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int SDL_DUMMY_RenderReadPixels(SDL_Renderer * renderer,
                                      const SDL_Rect * rect, void *pixels,
                                      int pitch);
static int SDL_DUMMY_RenderWritePixels(SDL_Renderer * renderer,
                                       const SDL_Rect * rect,
                                       const void *pixels, int pitch);
static void SDL_DUMMY_RenderPresent(SDL_Renderer * renderer);
static void SDL_DUMMY_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver SDL_DUMMY_RenderDriver = {
    SDL_DUMMY_CreateRenderer,
    {
     "minimal",
     (SDL_Renderer_Minimal | SDL_Renderer_PresentDiscard |
      SDL_Renderer_PresentCopy),
     SDL_TextureBlendMode_None,
     SDL_TextureScaleMode_None,
     0,
     {0},
     0,
     0}
};

typedef struct
{
    SDL_Surface *surface;
} SDL_DUMMY_RenderData;

SDL_Renderer *
SDL_DUMMY_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_DisplayMode *displayMode = &display->current_mode;
    SDL_Renderer *renderer;
    SDL_DUMMY_RenderData *data;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks
        (displayMode->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown display format");
        return NULL;
    }

    renderer = (SDL_Renderer *) SDL_malloc(sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(renderer);

    data = (SDL_DUMMY_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_DUMMY_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);

    renderer->RenderReadPixels = SDL_DUMMY_RenderReadPixels;
    renderer->RenderWritePixels = SDL_DUMMY_RenderWritePixels;
    renderer->RenderPresent = SDL_DUMMY_RenderPresent;
    renderer->DestroyRenderer = SDL_DUMMY_DestroyRenderer;
    renderer->info = SDL_DUMMY_RenderDriver.info;
    renderer->window = window->id;
    renderer->driverdata = data;

    data->surface =
        SDL_CreateRGBSurface(0, window->w, window->h, bpp, Rmask, Gmask,
                             Bmask, Amask);
    if (!data->surface) {
        SDL_DUMMY_DestroyRenderer(renderer);
        return NULL;
    }
    SDL_SetSurfacePalette(data->surface, display->palette);

    return renderer;
}

int
SDL_DUMMY_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                           void *pixels, int pitch)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Surface *surface = data->surface;
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

int
SDL_DUMMY_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                            const void *pixels, int pitch)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Surface *surface = data->surface;
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

void
SDL_DUMMY_RenderPresent(SDL_Renderer * renderer)
{
    static int frame_number;
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Surface *surface = data->surface;

    if (SDL_getenv("SDL_VIDEO_DUMMY_SAVE_FRAMES")) {
        char file[128];
        SDL_snprintf(file, sizeof(file), "SDL_window%d-%8.8d.bmp",
                     renderer->window, ++frame_number);
        SDL_SaveBMP(surface, file);
    }
}

void
SDL_DUMMY_DestroyRenderer(SDL_Renderer * renderer)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;

    if (data) {
        if (data->surface) {
            data->surface->format->palette = NULL;
            SDL_FreeSurface(data->surface);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

/* vi: set ts=4 sw=4 expandtab: */
