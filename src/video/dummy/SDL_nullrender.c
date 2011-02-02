/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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
#include "../SDL_yuv_sw_c.h"
#include "../SDL_renderer_sw.h"


/* SDL surface based renderer implementation */

static SDL_Renderer *SDL_DUMMY_CreateRenderer(SDL_Window * window,
                                              Uint32 flags);
static int SDL_DUMMY_RenderDrawPoints(SDL_Renderer * renderer,
                                      const SDL_Point * points, int count);
static int SDL_DUMMY_RenderDrawLines(SDL_Renderer * renderer,
                                     const SDL_Point * points, int count);
static int SDL_DUMMY_RenderFillRects(SDL_Renderer * renderer,
                                     const SDL_Rect ** rects, int count);
static int SDL_DUMMY_RenderCopy(SDL_Renderer * renderer,
                                SDL_Texture * texture,
                                const SDL_Rect * srcrect,
                                const SDL_Rect * dstrect);
static int SDL_DUMMY_RenderReadPixels(SDL_Renderer * renderer,
                                      const SDL_Rect * rect,
                                      Uint32 format,
                                      void * pixels, int pitch);
static int SDL_DUMMY_RenderWritePixels(SDL_Renderer * renderer,
                                       const SDL_Rect * rect,
                                       Uint32 format,
                                       const void * pixels, int pitch);
static void SDL_DUMMY_RenderPresent(SDL_Renderer * renderer);
static void SDL_DUMMY_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver SDL_DUMMY_RenderDriver = {
    SDL_DUMMY_CreateRenderer,
    {
     "dummy",
     (0),
     }
};

typedef struct
{
    SDL_Surface *screen;
} SDL_DUMMY_RenderData;

SDL_Renderer *
SDL_DUMMY_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = window->display;
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

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (SDL_DUMMY_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_DUMMY_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);

    renderer->RenderDrawPoints = SDL_DUMMY_RenderDrawPoints;
    renderer->RenderDrawLines = SDL_DUMMY_RenderDrawLines;
    renderer->RenderFillRects = SDL_DUMMY_RenderFillRects;
    renderer->RenderCopy = SDL_DUMMY_RenderCopy;
    renderer->RenderReadPixels = SDL_DUMMY_RenderReadPixels;
    renderer->RenderWritePixels = SDL_DUMMY_RenderWritePixels;
    renderer->RenderPresent = SDL_DUMMY_RenderPresent;
    renderer->DestroyRenderer = SDL_DUMMY_DestroyRenderer;
    renderer->info.name = SDL_DUMMY_RenderDriver.info.name;
    renderer->info.flags = 0;
    renderer->window = window;
    renderer->driverdata = data;
    Setup_SoftwareRenderer(renderer);

    data->screen =
        SDL_CreateRGBSurface(0, window->w, window->h, bpp, Rmask, Gmask,
                             Bmask, Amask);
    if (!data->screen) {
        SDL_DUMMY_DestroyRenderer(renderer);
        return NULL;
    }
    SDL_SetSurfacePalette(data->screen, display->palette);

    return renderer;
}

static int
SDL_DUMMY_RenderDrawPoints(SDL_Renderer * renderer,
                           const SDL_Point * points, int count)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screen;

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(target->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        return SDL_DrawPoints(target, points, count, color);
    } else {
        return SDL_BlendPoints(target, points, count, renderer->blendMode,
                               renderer->r, renderer->g, renderer->b,
                               renderer->a);
    }
}

static int
SDL_DUMMY_RenderDrawLines(SDL_Renderer * renderer,
                          const SDL_Point * points, int count)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screen;

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(target->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        return SDL_DrawLines(target, points, count, color);
    } else {
        return SDL_BlendLines(target, points, count, renderer->blendMode,
                              renderer->r, renderer->g, renderer->b,
                              renderer->a);
    }
}

static int
SDL_DUMMY_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
                          int count)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screen;

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(target->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        return SDL_FillRects(target, rects, count, color);
    } else {
        return SDL_BlendFillRects(target, rects, count,
                                  renderer->blendMode,
                                  renderer->r, renderer->g, renderer->b,
                                  renderer->a);
    }
}

static int
SDL_DUMMY_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                     const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;

    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        SDL_Surface *target = data->screen;
        void *pixels =
            (Uint8 *) target->pixels + dstrect->y * target->pitch +
            dstrect->x * target->format->BytesPerPixel;
        return SDL_SW_CopyYUVToRGB((SDL_SW_YUVTexture *) texture->driverdata,
                                   srcrect, display->current_mode.format,
                                   dstrect->w, dstrect->h, pixels,
                                   target->pitch);
    } else {
        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
        SDL_Surface *target = data->screen;
        SDL_Rect real_srcrect = *srcrect;
        SDL_Rect real_dstrect = *dstrect;

        return SDL_LowerBlit(surface, &real_srcrect, target, &real_dstrect);
    }
}

static int
SDL_DUMMY_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                           Uint32 format, void * pixels, int pitch)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    SDL_Surface *screen = data->screen;
    Uint32 screen_format = display->current_mode.format;
    Uint8 *screen_pixels = (Uint8 *) screen->pixels +
                            rect->y * screen->pitch +
                            rect->x * screen->format->BytesPerPixel;
    int screen_pitch = screen->pitch;

    return SDL_ConvertPixels(rect->w, rect->h,
                             screen_format, screen_pixels, screen_pitch,
                             format, pixels, pitch);
}

static int
SDL_DUMMY_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                            Uint32 format, const void * pixels, int pitch)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    SDL_Surface *screen = data->screen;
    Uint32 screen_format = display->current_mode.format;
    Uint8 *screen_pixels = (Uint8 *) screen->pixels +
                            rect->y * screen->pitch +
                            rect->x * screen->format->BytesPerPixel;
    int screen_pitch = screen->pitch;

    return SDL_ConvertPixels(rect->w, rect->h,
                             format, pixels, pitch,
                             screen_format, screen_pixels, screen_pitch);
}

static void
SDL_DUMMY_RenderPresent(SDL_Renderer * renderer)
{
    static int frame_number;
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;

    /* Send the data to the display */
    if (SDL_getenv("SDL_VIDEO_DUMMY_SAVE_FRAMES")) {
        char file[128];
        SDL_snprintf(file, sizeof(file), "SDL_window%d-%8.8d.bmp",
                     renderer->window->id, ++frame_number);
        SDL_SaveBMP(data->screen, file);
    }
}

static void
SDL_DUMMY_DestroyRenderer(SDL_Renderer * renderer)
{
    SDL_DUMMY_RenderData *data =
        (SDL_DUMMY_RenderData *) renderer->driverdata;
    int i;

    if (data) {
        if (data->screen) {
            SDL_FreeSurface(data->screen);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

/* vi: set ts=4 sw=4 expandtab: */
