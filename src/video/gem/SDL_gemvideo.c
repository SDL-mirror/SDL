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

/*
	GEM video driver

	Patrice Mandin
	and work from
	Olivier Landemarre, Johan Klockars, Xavier Joubert, Claude Attard
*/

/* Mint includes */
#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <mint/cookie.h>

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"

#include "SDL_gemvideo.h"

/* Initialization/Query functions */
static int GEM_VideoInit(_THIS);
static void GEM_VideoQuit(_THIS);

static void GEM_GL_SwapBuffers(_THIS);


static int
GEM_Available(void)
{
    /* Test if AES available */
    if (appl_init() == -1)
        return 0;

    appl_exit();
    return 1;
}

static void
GEM_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_VideoData *data = (SDL_VideoData *) device->driverdata;

    SDL_free(device->driverdata);
    SDL_free(device);
}

static SDL_VideoDevice *
GEM_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;
    SDL_VideoData *data;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (device) {
        data = (SDL_VideoData *) SDL_calloc(1, sizeof(SDL_VideoData));
    }
    if (!device || !data) {
        SDL_OutOfMemory();
        if (device) {
            SDL_free(device);
        }
        return NULL;
    }
    device->driverdata = data;

    /* Set the function pointers */
    device->VideoInit = GEM_VideoInit;
    device->VideoQuit = GEM_VideoQuit;
    device->GetDisplayModes = GEM_GetDisplayModes;
    device->SetDisplayMode = GEM_SetDisplayMode;
    device->PumpEvents = GEM_PumpEvents;

    device->CreateWindow = GEM_CreateWindow;
    device->CreateWindowFrom = GEM_CreateWindowFrom;
    device->SetWindowTitle = GEM_SetWindowTitle;
    device->SetWindowPosition = GEM_SetWindowPosition;
    device->SetWindowSize = GEM_SetWindowSize;
    device->ShowWindow = GEM_ShowWindow;
    device->HideWindow = GEM_HideWindow;
    device->RaiseWindow = GEM_RaiseWindow;
    device->MaximizeWindow = GEM_MaximizeWindow;
    device->MinimizeWindow = GEM_MinimizeWindow;
    device->RestoreWindow = GEM_RestoreWindow;
    device->SetWindowGrab = GEM_SetWindowGrab;
    device->DestroyWindow = GEM_DestroyWindow;
    device->GetWindowWMInfo = GEM_GetWindowWMInfo;

#if SDL_VIDEO_OPENGL
    device->GL_LoadLibrary = SDL_AtariGL_LoadLibrary;
    device->GL_GetProcAddress = SDL_AtariGL_GetProcAddress;
    device->GL_CreateContext = SDL_AtariGL_CreateContext;
    device->GL_MakeCurrent = SDL_AtariGL_MakeCurrent;
    device->GL_SetSwapInterval = SDL_AtariGL_SetSwapInterval;
    device->GL_GetSwapInterval = SDL_AtariGL_GetSwapInterval;
    device->GL_SwapWindow = GEM_GL_SwapWindow;
    device->GL_DeleteContext = SDL_AtariGL_DeleteContext;
#endif

    device->free = GEM_DeleteDevice;

    return device;
}

VideoBootStrap GEM_bootstrap = {
    "gem", "Atari GEM video driver",
    GEM_Available, GEM_CreateDevice
};

int
GEM_VideoInit(_THIS)
{
    int i;
    short work_in[12], work_out[272], dummy;
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    /* Open AES (Application Environment Services) */
    if (appl_init() == -1) {
        fprintf(stderr, "Can not open AES\n");
        return 1;
    }

    /* Read version and features */
    if (aes_global[0] >= 0x0410) {
        short ap_gout[4];

        data->wfeatures = 0;
        if (appl_getinfo(AES_WINDOW, &ap_gout[0], &ap_gout[1], &ap_gout[2],
	    &ap_gout[3]) == 0) {
            data->wfeatures = ap_gout[0];
        }
    }

    /* Ask VDI physical workstation handle opened by AES */
    data->vdi_handle = graf_handle(&dummy, &dummy, &dummy, &dummy);

    /* Open virtual VDI workstation */
    work_in[0] = Getrez() + 2;
    for (i = 1; i < 10; i++)
        work_in[i] = 1;
    work_in[10] = 2;

    v_opnvwk(work_in, &(data->vdi_handle), work_out);
    if (data->vdi_handle == 0) {
        fprintf(stderr, "Can not open VDI virtual workstation\n");
        return 1;
    }

    /* Set mouse cursor to arrow */
    graf_mouse(ARROW, NULL);

    /* Setup VDI fill functions */
    vsf_color(data->vdi_handle, 0);
    vsf_interior(data->vdi_handle, 1);
    vsf_perimeter(data->vdi_handle, 0);

    GEM_InitModes(_this);

    GEM_InitKeyboard(_this);
    GEM_InitMouse(_this);

    return (0);
}

void
GEM_VideoQuit(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    GEM_QuitMouse(_this);
    GEM_QuitKeyboard(_this);

    GEM_QuitModes(_this);

    /* Close VDI workstation */
    if (data->vdi_handle) {
        v_clsvwk(data->vdi_handle);
    }

    appl_exit();
}

/* vi: set ts=4 sw=4 expandtab: */
