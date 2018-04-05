/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

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

#if SDL_VIDEO_DRIVER_SWITCH

/* SDL internals */
#include "../SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_windowevents_c.h"
#include "SDL_switchtouch.h"

#include <switch.h>

#define SWITCH_DATA "_SDL_SwitchData"
#define SCREEN_WIDTH    1280
#define SCREEN_HEIGHT   720

typedef struct
{
    SDL_Surface *surface;
    int x_offset;
    int y_offset;
} SWITCH_WindowData;

static int SWITCH_VideoInit(_THIS);

static int SWITCH_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode);

static void SWITCH_VideoQuit(_THIS);

static void SWITCH_PumpEvents(_THIS);

static int SWITCH_CreateWindowFramebuffer(_THIS, SDL_Window *window, Uint32 *format, void **pixels, int *pitch);

static int SWITCH_UpdateWindowFramebuffer(_THIS, SDL_Window *window, const SDL_Rect *rects, int numrects);

static void SWITCH_DestroyWindowFramebuffer(_THIS, SDL_Window *window);

static int SWITCH_Available(void)
{
    return 1;
}

static void SWITCH_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device);
}

static SDL_VideoDevice *SWITCH_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return NULL;
    }

    device->VideoInit = SWITCH_VideoInit;
    device->VideoQuit = SWITCH_VideoQuit;
    device->SetDisplayMode = SWITCH_SetDisplayMode;
    device->PumpEvents = SWITCH_PumpEvents;
    device->CreateWindowFramebuffer = SWITCH_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = SWITCH_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = SWITCH_DestroyWindowFramebuffer;

    device->free = SWITCH_DeleteDevice;

    return device;
}

VideoBootStrap SWITCH_bootstrap = {
    "Switch", "Video driver for Nintendo Switch (libnx)",
    SWITCH_Available, SWITCH_CreateDevice
};

static int SWITCH_VideoInit(_THIS)
{
    SDL_DisplayMode mode;

    gfxInitResolution(SCREEN_WIDTH, SCREEN_HEIGHT);
    gfxInitDefault();
    gfxSetMode(GfxMode_TiledDouble);

    // add default mode (1280x720)
    mode.format = SDL_PIXELFORMAT_ABGR8888;
    mode.w = SCREEN_WIDTH;
    mode.h = SCREEN_HEIGHT;
    mode.refresh_rate = 60;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }
    SDL_AddDisplayMode(&_this->displays[0], &mode);

    // allow any resolution
    mode.w = 0;
    mode.h = 0;
    SDL_AddDisplayMode(&_this->displays[0], &mode);

    // init touch
    SWITCH_InitTouch();

    return 0;
}

static void SWITCH_VideoQuit(_THIS)
{
    SWITCH_QuitTouch();
    gfxExit();
}

static int SWITCH_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    SDL_SendWindowEvent(display->fullscreen_window,
                        SDL_WINDOWEVENT_RESIZED, mode->w, mode->h);

    return 0;
}

static void SWITCH_PumpEvents(_THIS)
{
    if (!appletMainLoop()) {
        SDL_Event ev;
        ev.type = SDL_QUIT;
        SDL_PushEvent(&ev);
        return;
    }

    hidScanInput();
    SWITCH_PollTouch();
}

static void SWITCH_SetResolution(u32 width, u32 height)
{
    u32 x, y, w, h, i;
    u32 *fb;

    // clear framebuffers before switching res
    for (i = 0; i < 2; i++) {

        fb = (u32 *) gfxGetFramebuffer(&w, &h);

        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                fb[gfxGetFramebufferDisplayOffset(x, y)] =
                    (u32) RGBA8_MAXALPHA(0, 0, 0);
            }
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxConfigureResolution(width, height);
}

static int SWITCH_CreateWindowFramebuffer(_THIS, SDL_Window *window, Uint32 *format, void **pixels, int *pitch)
{
    int bpp;
    Uint32 r, g, b, a;
    SDL_Surface *surface;
    SWITCH_WindowData *data;

    // create sdl surface framebuffer
    SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ABGR8888, &bpp, &r, &g, &b, &a);
    surface = SDL_CreateRGBSurface(0, window->w, window->h, bpp, r, g, b, a);
    if (!surface) {
        return -1;
    }

    // hold a pointer to our stuff
    data = SDL_calloc(1, sizeof(SWITCH_WindowData));
    data->surface = surface;

    // use switch hardware scaling in fullscreen mode
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        float scaling = (float) window->h / (float) SCREEN_HEIGHT;
        float w = SDL_min(SCREEN_WIDTH, SCREEN_WIDTH * scaling);
        // calculate x offset, to respect aspect ratio
        // round down to multiple of 4 for faster fb writes
        data->x_offset = ((int) (w - window->w) / 2) & ~3;
        data->y_offset = 0;
        SWITCH_SetResolution((u32) w, (u32) window->h);
        printf("gfxConfigureResolution: %i x %i (window: %i x %i, offset: %i x %i)\n",
               (int) w, window->h, window->w, window->h, data->x_offset, data->y_offset);
    }
    else {
        data->x_offset = ((SCREEN_WIDTH - window->w) / 2) & ~3;
        data->y_offset = (SCREEN_HEIGHT - window->h) / 2;
        SWITCH_SetResolution(0, 0);
        printf("gfxConfigureResolution: %i x %i (window: %i x %i, offset: %i x %i)\n",
               1280, 720, window->w, window->h, data->x_offset, data->y_offset);
    }

    *format = SDL_PIXELFORMAT_ABGR8888;
    *pixels = surface->pixels;
    *pitch = surface->pitch;

    SDL_SetWindowData(window, SWITCH_DATA, data);

    // inform SDL we're ready to accept inputs
    SDL_SetKeyboardFocus(window);

    return 0;
}

static int SWITCH_UpdateWindowFramebuffer(_THIS, SDL_Window *window, const SDL_Rect *rects, int numrects)
{
    SWITCH_WindowData *data = (SWITCH_WindowData *) SDL_GetWindowData(window, SWITCH_DATA);

    u32 fb_w, fb_h;
    int x, y, w = window->w, h = window->h;
    u32 *src = (u32 *) data->surface->pixels;
    u32 *dst = (u32 *) gfxGetFramebuffer(&fb_w, &fb_h);

    // prevent fb overflow in case of resolution change outside SDL
    if (data->x_offset + w > fb_w) {
        w = fb_w - data->x_offset;
    }
    if (data->y_offset + h > fb_h) {
        h = fb_h - data->y_offset;
    }

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x += 4) {
            *((u128 *) &dst[gfxGetFramebufferDisplayOffset(
                (u32) (x + data->x_offset), (u32) (y + data->y_offset))]) =
                *((u128 *) &src[y * w + x]);
        }
    }

    gfxFlushBuffers();
    gfxSwapBuffers();
    // TODO: handle SDL_RENDERER_PRESENTVSYNC (SW_RenderDriver not accepting flags)
    gfxWaitForVsync();

    return 0;
}

static void SWITCH_DestroyWindowFramebuffer(_THIS, SDL_Window *window)
{
    SWITCH_WindowData *data = (SWITCH_WindowData *) SDL_GetWindowData(window, SWITCH_DATA);
    SDL_FreeSurface(data->surface);
    SDL_free(data);
}

#endif /* SDL_VIDEO_DRIVER_SWITCH */
