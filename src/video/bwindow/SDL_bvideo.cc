/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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




#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_bwindow.h"
#include "SDL_bclipboard.h"
#include "SDL_bvideo.h"
#include "SDL_bopengl.h"
#include "SDL_bmodes.h"
#include "SDL_bevents.h"

/* FIXME: Undefined functions */
//    #define BE_PumpEvents NULL
    
#if SDL_VIDEO_OPENGL_WGL	/* FIXME: Replace with BeOs's SDL OPENGL stuff */
//    #define BE_GL_LoadLibrary NULL
//    #define BE_GL_GetProcAddress NULL
    #define BE_GL_UnloadLibrary NULL
    #define BE_GL_CreateContext NULL
//    #define BE_GL_MakeCurrent NULL
    #define BE_GL_SetSwapInterval NULL
    #define BE_GL_GetSwapInterval NULL
    #define BE_GL_SwapWindow NULL
    #define BE_GL_DeleteContext NULL
#endif
    #define BE_StartTextInput NULL
    #define BE_StopTextInput NULL
    #define BE_SetTextInputRect NULL

//    #define BE_DeleteDevice NULL

/* End undefined functions */

static SDL_VideoDevice *
BE_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;
    /*SDL_VideoData *data;*/

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
#if 0
    if (device) {
        data = (struct SDL_VideoData *) SDL_calloc(1, sizeof(SDL_VideoData));
    } else {
        data = NULL;
    }
    if (!data) {
        SDL_OutOfMemory();
        if (device) {
            SDL_free(device);
        }
        return NULL;
    }
#endif
    device->driverdata = NULL; /*data;*/

/* TODO: Figure out what sort of initialization needs to go here */

    /* Set the function pointers */
    device->VideoInit = BE_VideoInit;
    device->VideoQuit = BE_VideoQuit;
    device->GetDisplayBounds = BE_GetDisplayBounds;
    device->GetDisplayModes = BE_GetDisplayModes;
    device->SetDisplayMode = BE_SetDisplayMode;
    device->PumpEvents = BE_PumpEvents;

    device->CreateWindow = BE_CreateWindow;
    device->CreateWindowFrom = BE_CreateWindowFrom;
    device->SetWindowTitle = BE_SetWindowTitle;
    device->SetWindowIcon = BE_SetWindowIcon;
    device->SetWindowPosition = BE_SetWindowPosition;
    device->SetWindowSize = BE_SetWindowSize;
    device->ShowWindow = BE_ShowWindow;
    device->HideWindow = BE_HideWindow;
    device->RaiseWindow = BE_RaiseWindow;
    device->MaximizeWindow = BE_MaximizeWindow;
    device->MinimizeWindow = BE_MinimizeWindow;
    device->RestoreWindow = BE_RestoreWindow;
    device->SetWindowFullscreen = BE_SetWindowFullscreen;
    device->SetWindowGammaRamp = BE_SetWindowGammaRamp;
    device->GetWindowGammaRamp = BE_GetWindowGammaRamp;
    device->SetWindowGrab = BE_SetWindowGrab;
    device->DestroyWindow = BE_DestroyWindow;
    device->GetWindowWMInfo = BE_GetWindowWMInfo;
    device->CreateWindowFramebuffer = BE_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = BE_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = BE_DestroyWindowFramebuffer;
    
    device->shape_driver.CreateShaper = NULL;
    device->shape_driver.SetWindowShape = NULL;
    device->shape_driver.ResizeWindowShape = NULL;
    
#if SDL_VIDEO_OPENGL_WGL	/* FIXME: Replace with BeOs's SDL OPENGL stuff */
    device->GL_LoadLibrary = BE_GL_LoadLibrary;
    device->GL_GetProcAddress = BE_GL_GetProcAddress;
    device->GL_UnloadLibrary = BE_GL_UnloadLibrary;
    device->GL_CreateContext = BE_GL_CreateContext;
    device->GL_MakeCurrent = BE_GL_MakeCurrent;
    device->GL_SetSwapInterval = BE_GL_SetSwapInterval;
    device->GL_GetSwapInterval = BE_GL_GetSwapInterval;
    device->GL_SwapWindow = BE_GL_SwapWindow;
    device->GL_DeleteContext = BE_GL_DeleteContext;
#endif
    device->StartTextInput = BE_StartTextInput;
    device->StopTextInput = BE_StopTextInput;
    device->SetTextInputRect = BE_SetTextInputRect;

    device->SetClipboardText = BE_SetClipboardText;
    device->GetClipboardText = BE_GetClipboardText;
    device->HasClipboardText = BE_HasClipboardText;

    device->free = BE_DeleteDevice;

    return device;
}

VideoBootStrap BWINDOW_bootstrap = {
	"bwindow", "BDirectWindow graphics",
	BE_Available, BE_CreateDevice
};

static void BE_DeleteDevice(SDL_VideoDevice * device)
{
	SDL_free(device->driverdata);
	SDL_free(device);
}

/* FIXME: This is the 1.2 function at the moment.  Read through it and
  o understand what it does. */
int BE_VideoInit(_THIS)
{
	/* Initialize the Be Application for appserver interaction */
	if (SDL_InitBeApp() < 0) {
		return -1;
	}
	
	BE_InitModes(_this);

#if SDL_VIDEO_OPENGL
        /* testgl application doesn't load library, just tries to load symbols */
        /* is it correct? if so we have to load library here */
    BE_GL_LoadLibrary(_this, NULL);
#endif

        /* Fill in some window manager capabilities */
//    _this->info.wm_available = 1;

        /* We're done! */
    return (0);
}

static int BE_Available(void)
{
    return (1);
}

void BE_VideoQuit(_THIS)
{

#if 0
        SDL_Win->Quit();
        SDL_Win = NULL;
#endif
#if 0
        if (SDL_BlankCursor != NULL) {
            BE_FreeWMCursor(_this, SDL_BlankCursor);
            SDL_BlankCursor = NULL;
        }
#endif

    BE_QuitModes(_this);

#if SDL_VIDEO_OPENGL
//    if (_this->gl_config.dll_handle != NULL)
//        unload_add_on((image_id) _this->gl_config.dll_handle);
#endif

    SDL_QuitBeApp();
}

#ifdef __cplusplus
}
#endif
