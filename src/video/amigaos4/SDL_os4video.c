/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

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

#if SDL_VIDEO_DRIVER_AMIGAOS4

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_hints.h"

#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_os4video.h"
#include "SDL_os4events.h"
#include "SDL_os4framebuffer.h"
#include "SDL_os4mouse.h"
#include "SDL_os4opengl.h"
#include "SDL_os4shape.h"
#include "SDL_os4messagebox.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

#define OS4VID_DRIVER_NAME "os4"

/* Initialization/Query functions */
static int OS4_VideoInit(_THIS);
static void OS4_VideoQuit(_THIS);

/* OS4 driver bootstrap functions */

static int
OS4_Available(void)
{
	return (1);
}

/*
 * Libraries required by OS4 video driver
 */

#define MIN_LIB_VERSION 51

//TODO: add proper error messages, wrap OpenLibrary?
static SDL_bool
OS4_OpenLibraries(_THIS)
{
	dprintf("Called\n");

	GfxBase       = IExec->OpenLibrary("graphics.library", 54);
	LayersBase    = IExec->OpenLibrary("layers.library", 53);
	//P96Base       = IExec->OpenLibrary("Picasso96API.library", 0);
	IntuitionBase = IExec->OpenLibrary("intuition.library", MIN_LIB_VERSION);
	IconBase      = IExec->OpenLibrary("icon.library", MIN_LIB_VERSION);
	WorkbenchBase = IExec->OpenLibrary("workbench.library", MIN_LIB_VERSION);
	KeymapBase    = IExec->OpenLibrary("keymap.library", MIN_LIB_VERSION);
	MiniGLBase    = IExec->OpenLibrary("minigl.library", 2);

	if (!GfxBase || !LayersBase || /*!P96Base ||*/ !IntuitionBase || !IconBase || !WorkbenchBase || !KeymapBase) {
		dprintf("Failed to open system library\n");
		return SDL_FALSE;
	}

	if (!MiniGLBase) {
		dprintf("Failed to open minigl.library\n");
	}

	IGraphics  = (struct GraphicsIFace *)  IExec->GetInterface(GfxBase, "main", 1, NULL);
	ILayers    = (struct LayersIFace *)    IExec->GetInterface(LayersBase, "main", 1, NULL);
	//IP96       = (struct P96IFace *)       IExec->GetInterface(P96Base, "main", 1, NULL);
	IIntuition = (struct IntuitionIFace *) IExec->GetInterface(IntuitionBase, "main", 1, NULL);
	IIcon      = (struct IconIFace *)      IExec->GetInterface(IconBase, "main", 1, NULL);
	IWorkbench = (struct WorkbenchIFace *) IExec->GetInterface(WorkbenchBase, "main", 1, NULL);
	IKeymap    = (struct KeymapIFace *)    IExec->GetInterface(KeymapBase, "main", 1, NULL);
	IMiniGL	   = (struct MiniGLIFace *)    IExec->GetInterface(MiniGLBase, "main", 1, NULL);

	if (!IGraphics || !ILayers || /*!IP96 ||*/ !IIntuition || !IIcon || !IWorkbench || !IKeymap) {
		dprintf("Failed to get library interface\n");
		return SDL_FALSE;
	}

	if (!IMiniGL) {
		dprintf("Failed to open MiniGL interace\n");
	} else {
		dprintf("MiniGL opened\n");
	}

	return SDL_TRUE;
}

static void
OS4_CloseLibraries(_THIS)
{
	dprintf("Called\n");

	if (IMiniGL) {
		IExec->DropInterface((struct Interface *) IMiniGL);
		IMiniGL = NULL;
	}

	if (IKeymap) {
		IExec->DropInterface((struct Interface *) IKeymap);
		IKeymap = NULL;
	}
	if (IWorkbench) {
		IExec->DropInterface((struct Interface *) IWorkbench);
		IWorkbench = NULL;
	}
	if (IIcon) {
		IExec->DropInterface((struct Interface *) IIcon);
		IIcon = NULL;
	}
	if (IIntuition) {
		IExec->DropInterface((struct Interface *) IIntuition);
		IIntuition = NULL;
	}
/*
	if (IP96) {
		IExec->DropInterface((struct Interface *) IP96);
		IP96 = NULL;
	}
*/
	if (ILayers) {
		IExec->DropInterface((struct Interface *) ILayers);
		ILayers = NULL;
	}
	if (IGraphics) {
		IExec->DropInterface((struct Interface *) IGraphics);
		IGraphics = NULL;
	}

	if (MiniGLBase) {
		IExec->CloseLibrary(MiniGLBase);
		MiniGLBase = NULL;
	}

	if (KeymapBase) {
		IExec->CloseLibrary(KeymapBase);
		KeymapBase = NULL;
	}
	if (WorkbenchBase) {
		IExec->CloseLibrary(WorkbenchBase);
		WorkbenchBase = NULL;
	}
	if (IconBase) {
		IExec->CloseLibrary(IconBase);
		IconBase = NULL;
	}
	if (IntuitionBase) {
		IExec->CloseLibrary(IntuitionBase);
		IntuitionBase = NULL;
	}
/*
	if (P96Base) {
		IExec->CloseLibrary(P96Base);
		P96Base = NULL;
	}
*/
	if (LayersBase) {
		IExec->CloseLibrary(LayersBase);
		LayersBase = NULL;
	}
	if (GfxBase) {
		IExec->CloseLibrary(GfxBase);
		GfxBase = NULL;
	}
}

static SDL_bool
OS4_AllocSystemResources(_THIS)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

	dprintf("Called\n");

	if (!OS4_OpenLibraries(_this)) {
		return SDL_FALSE;
	}

	if (!(data->userport = IExec->AllocSysObject(ASOT_PORT, 0))) {
		SDL_SetError("Couldn't allocate message port");
		return SDL_FALSE;
	}

	/* Create the pool we'll be using (Shared, might be used from threads) */
	if (!(data->pool = IExec->AllocSysObjectTags(ASOT_MEMPOOL,
		ASOPOOL_MFlags,    MEMF_SHARED,
		ASOPOOL_Threshold, 16384,
		ASOPOOL_Puddle,    16384,
		ASOPOOL_Protected, TRUE,
		TAG_DONE))) {
	
		SDL_SetError("Couldn't allocate pool");
		return SDL_FALSE;
	}

	/* inputPort, inputReq and and input.device are created for WarpMouse functionality. (In SDL1
	they were created in library constructor for an unknown reason) */
	if (!(data->inputPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_DONE))) {

		SDL_SetError("Couldn't allocate input port");
		return SDL_FALSE;
	}

	if (!(data->inputReq = IExec->AllocSysObjectTags(ASOT_IOREQUEST,
											 ASOIOR_Size, 		sizeof(struct IOStdReq),
											 ASOIOR_ReplyPort,	data->inputPort,
											 TAG_DONE))) {

		SDL_SetError("Couldn't allocate input request");
		return SDL_FALSE;
	}
	
	if (IExec->OpenDevice("input.device", 0, (struct IORequest *)data->inputReq, 0))
	{
		SDL_SetError("Couldn't open input.device");
		return SDL_FALSE;
	}

	return SDL_TRUE;
}

static void
OS4_FreeSystemResources(_THIS)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
	
	dprintf("Called\n");

	if (data->inputReq) {
		dprintf("Deleting input.device\n");
		//IExec->AbortIO((struct IORequest *)data->inputReq);
		//IExec->WaitIO((struct IORequest *)data->inputReq);
		IExec->CloseDevice((struct IORequest *)data->inputReq);
		
		dprintf("Deleting IORequest\n");
		IExec->FreeSysObject(ASOT_IOREQUEST, (void *)data->inputReq);
	}

	if (data->inputPort) {
		dprintf("Deleting MsgPort\n");
		IExec->FreeSysObject(ASOT_PORT, (void *)data->inputPort);
	}

	if (data->pool) {
		IExec->FreeSysObject(ASOT_MEMPOOL, data->pool);
	}

	if (data->userport) {
	   	IExec->FreeSysObject(ASOT_PORT, data->userport);
	}

	OS4_CloseLibraries(_this);
}

static void
OS4_DeleteDevice(SDL_VideoDevice * device)
{
	dprintf("Called\n");

	OS4_FreeSystemResources(device);

	SDL_free(device->driverdata);
	SDL_free(device);
}

static SDL_VideoDevice *
OS4_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;
	SDL_VideoData *data;

	dprintf("*** SDL2 video initialization starts ***\n");

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));

	if (device) {
		data = (SDL_VideoData *) SDL_calloc(1, sizeof(SDL_VideoData));
	} else {
		data = NULL;
	}

	if (!data) {
		SDL_free(device);
		SDL_OutOfMemory();
		return NULL;
	}

	device->driverdata = data;

	if (!OS4_AllocSystemResources(device)) {
		SDL_free(device);
		SDL_free(data);
		
		/* If we return with NULL, SDL_VideoQuit() can't clean up OS4 stuff. So let's do it now. */
		OS4_FreeSystemResources(device);
		
		SDL_Unsupported();

		return NULL;
	}

	/* Set the function pointers */
	device->VideoInit = OS4_VideoInit;
	device->VideoQuit = OS4_VideoQuit;

	device->GetDisplayBounds = OS4_GetDisplayBounds;
	device->GetDisplayModes = OS4_GetDisplayModes;
	device->SetDisplayMode = OS4_SetDisplayMode;

	device->CreateWindow = OS4_CreateWindow;
	device->CreateWindowFrom = OS4_CreateWindowFrom;
	device->SetWindowTitle = OS4_SetWindowTitle;
	//device->SetWindowIcon = OS4_SetWindowIcon;
	device->SetWindowPosition = OS4_SetWindowPosition;
	device->SetWindowSize = OS4_SetWindowSize;
	device->ShowWindow = OS4_ShowWindow;
	device->HideWindow = OS4_HideWindow;
	device->RaiseWindow = OS4_RaiseWindow;
	//device->MaximizeWindow = OS4_MaximizeWindow;
	//device->MinimizeWindow = OS4_MinimizeWindow;
	//device->RestoreWindow = OS4_RestoreWindow;
	//device->SetWindowBordered = OS4_SetWindowBordered;
	device->SetWindowFullscreen = OS4_SetWindowFullscreen;
	//device->SetWindowGammaRamp = OS4_SetWindowGammaRamp;
	//device->GetWindowGammaRamp = OS4_GetWindowGammaRamp;
	device->SetWindowGrab = OS4_SetWindowGrab;
	device->DestroyWindow = OS4_DestroyWindow;
	
	device->CreateWindowFramebuffer = OS4_CreateWindowFramebuffer;
	device->UpdateWindowFramebuffer = OS4_UpdateWindowFramebuffer;
	device->DestroyWindowFramebuffer = OS4_DestroyWindowFramebuffer;
	
	//device->OnWindowEnter = OS4_OnWindowEnter;

	device->shape_driver.CreateShaper = OS4_CreateShaper;
	device->shape_driver.SetWindowShape = OS4_SetWindowShape;
	device->shape_driver.ResizeWindowShape = OS4_ResizeWindowShape;

	device->GetWindowWMInfo = OS4_GetWindowWMInfo;

	device->GL_LoadLibrary = OS4_GL_LoadLibrary;
	device->GL_GetProcAddress = OS4_GL_GetProcAddress;
	device->GL_UnloadLibrary = OS4_GL_UnloadLibrary;
	device->GL_CreateContext = OS4_GL_CreateContext;
	device->GL_MakeCurrent = OS4_GL_MakeCurrent;
	device->GL_GetDrawableSize = OS4_GL_GetDrawableSize;
	device->GL_SetSwapInterval = OS4_GL_SetSwapInterval;
	device->GL_GetSwapInterval = OS4_GL_GetSwapInterval;
	device->GL_SwapWindow = OS4_GL_SwapWindow;
	device->GL_DeleteContext = OS4_GL_DeleteContext;

	device->PumpEvents = OS4_PumpEvents;
	//device->SuspendScreenSaver = OS4_SuspendScreenSaver;
	//device->SetClipboardText = OS4_SetClipboardText;
	//device->GetClipboardText = OS4_GetClipboardText;
	//device->HasClipboardText = OS4_HasClipboardText;
	//device->ShowMessageBox = OS4_ShowMessageBox; Can be called without video initialization

	device->free = OS4_DeleteDevice;

	return device;
}

VideoBootStrap OS4_bootstrap = {
	OS4VID_DRIVER_NAME, "SDL AmigaOS 4 video driver",
	OS4_Available, OS4_CreateDevice
};


int
OS4_VideoInit(_THIS)
{
	dprintf("Called\n");

	if (OS4_InitModes(_this) < 0) {
		return -1;
	}

	OS4_InitKeyboard(_this);
	OS4_InitMouse(_this);

	// We don't want SDL to change  window setup in SDL_OnWindowFocusLost()
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

	if (IMiniGL) {
        _this->gl_config.driver_loaded = 1;
	}

	return 0;
}

void
OS4_VideoQuit(_THIS)
{
	dprintf("Called\n");

	OS4_QuitModes(_this);
	OS4_QuitKeyboard(_this);
	OS4_QuitMouse(_this);
}

void *
OS4_SaveAllocPooled(_THIS, uint32 size)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;	  

	return IExec->AllocPooled(data->pool, size);
}

void *
OS4_SaveAllocVecPooled(_THIS, uint32 size)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

	return IExec->AllocVecPooled(data->pool, size);
}

void
OS4_SaveFreePooled(_THIS, void * mem, uint32 size)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

	IExec->FreePooled(data->pool, mem, size);
}

void
OS4_SaveFreeVecPooled(_THIS, void * mem)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

	IExec->FreeVecPooled(data->pool, mem);
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
