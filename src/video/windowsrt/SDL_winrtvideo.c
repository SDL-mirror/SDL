/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

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
#include "SDL_config.h"

#if SDL_VIDEO_DRIVER_WINRT

/* WinRT SDL video driver implementation

   Initial work on this was done by David Ludwig (dludwig@pobox.com), and
   was based off of SDL's "dummy" video driver.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_winrtvideo.h"
#include "SDL_winrtevents_c.h"
#include "SDL_winrtframebuffer_c.h"

#define WINRTVID_DRIVER_NAME "dummy"

/* Initialization/Query functions */
static int WINRT_VideoInit(_THIS);
static int WINRT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
static void WINRT_VideoQuit(_THIS);

/* WinRT driver bootstrap functions */

static int
WINRT_Available(void)
{
    const char *envr = SDL_getenv("SDL_VIDEODRIVER");
    if ((envr) && (SDL_strcmp(envr, WINRTVID_DRIVER_NAME) == 0)) {
        return (1);
    }

    return (0);
}

static void
WINRT_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
WINRT_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        if (device) {
            SDL_free(device);
        }
        return (0);
    }

    /* Set the function pointers */
    device->VideoInit = WINRT_VideoInit;
    device->VideoQuit = WINRT_VideoQuit;
    device->SetDisplayMode = WINRT_SetDisplayMode;
    device->PumpEvents = WINRT_PumpEvents;
    device->CreateWindowFramebuffer = SDL_WINRT_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = SDL_WINRT_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = SDL_WINRT_DestroyWindowFramebuffer;

    device->free = WINRT_DeleteDevice;

    return device;
}

VideoBootStrap WINRT_bootstrap = {
    WINRTVID_DRIVER_NAME, "SDL Windows RT video driver",
    WINRT_Available, WINRT_CreateDevice
};


int
WINRT_VideoInit(_THIS)
{
    SDL_DisplayMode mode;

    /* Use a fake 32-bpp desktop mode */
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.w = 1024;
    mode.h = 768;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_zero(mode);
    SDL_AddDisplayMode(&_this->displays[0], &mode);

    /* We're done! */
    return 0;
}

static int
WINRT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    return 0;
}

void
WINRT_VideoQuit(_THIS)
{
}

#endif /* SDL_VIDEO_DRIVER_WINRT */

/* vi: set ts=4 sw=4 expandtab: */
