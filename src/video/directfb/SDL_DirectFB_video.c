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

    SDL1.3 implementation by couriersud@arcor.de
	
*/


#include "SDL_config.h"

/* DirectFB video driver implementation.
*/

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <directfb.h>
#include <directfb_version.h>

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
#include "SDL_DirectFB_video.h"
#include "SDL_DirectFB_events.h"
#include "SDL_DirectFB_render.h"
#include "SDL_DirectFB_mouse.h"

/* Initialization/Query functions */
static int DirectFB_VideoInit(_THIS);
static void DirectFB_VideoQuit(_THIS);

static int DirectFB_Available(void);
static SDL_VideoDevice *DirectFB_CreateDevice(int devindex);

static int DirectFB_SetDisplayGammaRamp(_THIS, Uint16 * ramp);
static int DirectFB_GetDisplayGammaRamp(_THIS, Uint16 * ramp);

VideoBootStrap DirectFB_bootstrap = {
    "directfb", "DirectFB",
    DirectFB_Available, DirectFB_CreateDevice
};

/* DirectFB driver bootstrap functions */

static int
DirectFB_Available(void)
{
    return 1;
}

static void
DirectFB_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device->driverdata);
    SDL_free(device);
}

static SDL_VideoDevice *
DirectFB_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    SDL_DFB_CALLOC(device, 1, sizeof(SDL_VideoDevice));

    /* Set the function pointers */

    /* Set the function pointers */
    device->VideoInit = DirectFB_VideoInit;
    device->VideoQuit = DirectFB_VideoQuit;
    device->GetDisplayModes = DirectFB_GetDisplayModes;
    device->SetDisplayMode = DirectFB_SetDisplayMode;
#if 0
    device->SetDisplayGammaRamp = DirectFB_SetDisplayGammaRamp;
    device->GetDisplayGammaRamp = DirectFB_GetDisplayGammaRamp;
#else
    device->SetDisplayGammaRamp = NULL;
    device->GetDisplayGammaRamp = NULL;
#endif
    device->PumpEvents = DirectFB_PumpEventsWindow;
    device->CreateWindow = DirectFB_CreateWindow;
    device->CreateWindowFrom = DirectFB_CreateWindowFrom;
    device->SetWindowTitle = DirectFB_SetWindowTitle;
    device->SetWindowPosition = DirectFB_SetWindowPosition;
    device->SetWindowSize = DirectFB_SetWindowSize;
    device->ShowWindow = DirectFB_ShowWindow;
    device->HideWindow = DirectFB_HideWindow;
    device->RaiseWindow = DirectFB_RaiseWindow;
    device->MaximizeWindow = DirectFB_MaximizeWindow;
    device->MinimizeWindow = DirectFB_MinimizeWindow;
    device->RestoreWindow = DirectFB_RestoreWindow;
    device->SetWindowGrab = DirectFB_SetWindowGrab;
    device->DestroyWindow = DirectFB_DestroyWindow;
    device->GetWindowWMInfo = DirectFB_GetWindowWMInfo;

#if SDL_DIRECTFB_OPENGL
    device->GL_LoadLibrary = DirectFB_GL_LoadLibrary;
    device->GL_GetProcAddress = DirectFB_GL_GetProcAddress;
    device->GL_MakeCurrent = DirectFB_GL_MakeCurrent;

    device->GL_CreateContext = DirectFB_GL_CreateContext;
    device->GL_SetSwapInterval = DirectFB_GL_SetSwapInterval;
    device->GL_GetSwapInterval = DirectFB_GL_GetSwapInterval;
    device->GL_SwapWindow = DirectFB_GL_SwapWindow;
    device->GL_DeleteContext = DirectFB_GL_DeleteContext;

#endif

    device->free = DirectFB_DeleteDevice;

    return device;
  error:
    if (device)
        free(device);
    return (0);
}

static int
DirectFB_VideoInit(_THIS)
{
    IDirectFB *dfb = NULL;
    DFB_DeviceData *devdata;
    char *stemp;
    DFBResult ret;

    SDL_DFB_CHECKERR(DirectFBInit(NULL, NULL));
    SDL_DFB_CHECKERR(DirectFBCreate(&dfb));

    SDL_DFB_CALLOC(devdata, 1, sizeof(*devdata));

    devdata->use_yuv_underlays = 0;     /* default: off */
    stemp = getenv(DFBENV_USE_YUV_UNDERLAY);
    if (stemp)
        devdata->use_yuv_underlays = atoi(stemp);

    /* Create global Eventbuffer for axis events */
    SDL_DFB_CHECKERR(dfb->
                     CreateInputEventBuffer(dfb, DICAPS_AXES /*DICAPS_ALL */ ,
                                            DFB_TRUE, &devdata->events));

    devdata->initialized = 1;
    devdata->dfb = dfb;
    devdata->firstwin = NULL;

    _this->driverdata = devdata;

    DirectFB_InitModes(_this);

#if SDL_DIRECTFB_OPENGL
    DirectFB_GL_Initialize(_this);
#endif

    DirectFB_AddRenderDriver(_this);
    DirectFB_InitMouse(_this);
    DirectFB_InitKeyboard(_this);


    return 0;


  error:
    SDL_DFB_RELEASE(dfb);
    return -1;
}

static void
DirectFB_VideoQuit(_THIS)
{
    DFB_DeviceData *devdata = (DFB_DeviceData *) _this->driverdata;

    DirectFB_QuitModes(_this);
    DirectFB_QuitKeyboard(_this);
    DirectFB_QuitMouse(_this);

    SDL_DFB_RELEASE(devdata->events);
    SDL_DFB_RELEASE(devdata->dfb);
    SDL_DFB_FREE(_this->driverdata);

#if SDL_DIRECTFB_OPENGL
    DirectFB_GL_Shutdown(_this);
#endif

    devdata->initialized = 0;
}

static int
DirectFB_SetDisplayGammaRamp(_THIS, Uint16 * ramp)
{
    return -1;
}

static int
DirectFB_GetDisplayGammaRamp(_THIS, Uint16 * ramp)
{
    return -1;
}
