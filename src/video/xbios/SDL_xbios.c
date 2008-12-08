/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

/*
 * Xbios SDL video driver
 * 
 * Patrice Mandin
 */

#include <sys/stat.h>
#include <unistd.h>

/* Mint includes */
#include <mint/cookie.h>
#include <mint/osbind.h>
#include <mint/falcon.h>

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "../ataricommon/SDL_ataric2p_s.h"
#include "../ataricommon/SDL_atarievents_c.h"
#include "../ataricommon/SDL_atarimxalloc_c.h"
#include "../ataricommon/SDL_atarigl_c.h"
#include "SDL_xbios.h"
#include "SDL_xbiosmodes.h"

/* Debug print info */
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#define DEBUG_VIDEO_XBIOS 1
#else
#define DEBUG_PRINT(what)
#undef DEBUG_VIDEO_XBIOS
#endif

/* Initialization/Query functions */
static int XBIOS_VideoInit(_THIS);
static void XBIOS_VideoQuit(_THIS);

/* Xbios driver bootstrap functions */

static int
XBIOS_Available(void)
{
    unsigned long cookie_vdo, cookie_mil, cookie_hade, cookie_scpn;

    /* Milan/Hades Atari clones do not have an Atari video chip */
    if ((Getcookie(C__MIL, &cookie_mil) == C_FOUND) ||
        (Getcookie(C_hade, &cookie_hade) == C_FOUND)) {
        return 0;
    }

    /* Cookie _VDO present ? if not, assume ST machine */
    if (Getcookie(C__VDO, &cookie_vdo) != C_FOUND) {
        cookie_vdo = VDO_ST << 16;
    }

    /* Test if we have a monochrome monitor plugged in */
    switch (cookie_vdo >> 16) {
    case VDO_ST:
    case VDO_STE:
        if (Getrez() == (ST_HIGH >> 8))
            return 0;
        break;
    case VDO_TT:
        if ((EgetShift() & ES_MODE) == TT_HIGH)
            return 0;
        break;
    case VDO_F30:
        if (VgetMonitor() == MONITOR_MONO)
            return 0;
        /*if (Getcookie(C_SCPN, &cookie_scpn) == C_FOUND) {
           if (!SDL_XBIOS_SB3Usable((scpn_cookie_t *) cookie_scpn)) {
           return 0;
           }
           } */
        break;
    default:
        return 0;
    }

    return 1;
}

static void
XBIOS_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device->driverdata);
    SDL_free(device);
}

static SDL_VideoDevice *
XBIOS_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;
    SDL_VideoData *data;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (device) {
        data = (struct SDL_VideoData *) SDL_calloc(1, sizeof(SDL_VideoData));
    }
    if (!device || !data) {
        SDL_OutOfMemory();
        if (device) {
            SDL_free(device);
        }
        return NULL;
    }
    device->driverdata = data;

    /* Video functions */
    device->VideoInit = XBIOS_VideoInit;
    device->VideoQuit = XBIOS_VideoQuit;

    /* Modes */
    device->GetDisplayModes = SDL_XBIOS_GetDisplayModes;
    device->SetDisplayMode = SDL_XBIOS_SetDisplayMode;

    /* Events */
    device->PumpEvents = SDL_Atari_PumpEvents;

#if SDL_VIDEO_OPENGL
    /* OpenGL functions */
    device->GL_LoadLibrary = SDL_AtariGL_LoadLibrary;
    device->GL_GetProcAddress = SDL_AtariGL_GetProcAddress;
    device->GL_CreateContext = NULL;
    device->GL_MakeCurrent = SDL_AtariGL_MakeCurrent;
    device->GL_SetSwapInterval = NULL;
    device->GL_GetSwapInterval = NULL;
    device->GL_SwapWindow = XBIOS_GL_SwapBuffers;
    device->GL_DeleteContext = NULL;
#endif

    device->free = XBIOS_DeleteDevice;

    return device;
}

VideoBootStrap XBIOS_bootstrap = {
    "xbios", "Atari Xbios driver",
    XBIOS_Available, XBIOS_CreateDevice
};

static int
XBIOS_VideoInit(_THIS)
{
    /* Save screensaver settings */

    /* Init video mode list, save current video mode settings */
    SDL_XBIOS_InitModes(_this);

    return (0);
}

static void
XBIOS_VideoQuit(_THIS)
{
    Atari_ShutdownEvents();

    /* Restore screensaver settings */

    /* Restore previous video mode settings */
    SDL_XBIOS_QuitModes(_this);
}
