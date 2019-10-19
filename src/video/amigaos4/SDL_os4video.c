/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

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

#include <proto/exec.h>

#include "SDL_video.h"
#include "SDL_hints.h"
#include "SDL_version.h"

#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_os4video.h"
#include "SDL_os4events.h"
#include "SDL_os4framebuffer.h"
#include "SDL_os4mouse.h"
#include "SDL_os4opengl.h"
#include "SDL_os4opengles.h"
#include "SDL_os4shape.h"
#include "SDL_os4messagebox.h"
#include "SDL_os4modes.h"
#include "SDL_os4keyboard.h"
#include "SDL_os4library.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

#define OS4VID_DRIVER_NAME "os4"

static int OS4_VideoInit(_THIS);
static void OS4_VideoQuit(_THIS);

SDL_bool (*OS4_ResizeGlContext)(_THIS, SDL_Window * window) = NULL;
void (*OS4_UpdateGlWindowPointer)(_THIS, SDL_Window * window) = NULL;

static int
OS4_Available(void)
{
    return (1);
}

#define MIN_LIB_VERSION 51

static SDL_bool
OS4_OpenLibraries(_THIS)
{
    dprintf("Opening libraries\n");

    GfxBase       = OS4_OpenLibrary("graphics.library", 54);
    LayersBase    = OS4_OpenLibrary("layers.library", 53);
    IntuitionBase = OS4_OpenLibrary("intuition.library", MIN_LIB_VERSION);
    IconBase      = OS4_OpenLibrary("icon.library", MIN_LIB_VERSION);
    WorkbenchBase = OS4_OpenLibrary("workbench.library", MIN_LIB_VERSION);
    KeymapBase    = OS4_OpenLibrary("keymap.library", MIN_LIB_VERSION);
    TextClipBase  = OS4_OpenLibrary("textclip.library", MIN_LIB_VERSION);
    DOSBase       = OS4_OpenLibrary("dos.library", MIN_LIB_VERSION);

    if (GfxBase && LayersBase && IntuitionBase && IconBase &&
        WorkbenchBase && KeymapBase && TextClipBase && DOSBase) {

        IGraphics  = (struct GraphicsIFace *)  OS4_GetInterface(GfxBase);
        ILayers    = (struct LayersIFace *)    OS4_GetInterface(LayersBase);
        IIntuition = (struct IntuitionIFace *) OS4_GetInterface(IntuitionBase);
        IIcon      = (struct IconIFace *)      OS4_GetInterface(IconBase);
        IWorkbench = (struct WorkbenchIFace *) OS4_GetInterface(WorkbenchBase);
        IKeymap    = (struct KeymapIFace *)    OS4_GetInterface(KeymapBase);
        ITextClip  = (struct TextClipIFace *)  OS4_GetInterface(TextClipBase);
        IDOS       = (struct DOSIFace *)       OS4_GetInterface(DOSBase);

        if (IGraphics && ILayers && IIntuition && IIcon &&
            IWorkbench && IKeymap && ITextClip && IDOS) {

            dprintf("All library interfaces OK\n");

            return SDL_TRUE;

        } else {
            dprintf("Failed to get library interfaces\n");
        }
    } else {
        dprintf("Failed to open system libraries\n");
    }

    return SDL_FALSE;
}

static void
OS4_CloseLibraries(_THIS)
{
    dprintf("Closing libraries\n");

    OS4_DropInterface((void *)&IDOS);
    OS4_DropInterface((void *)&ITextClip);
    OS4_DropInterface((void *)&IKeymap);
    OS4_DropInterface((void *)&IWorkbench);
    OS4_DropInterface((void *)&IIcon);
    OS4_DropInterface((void *)&IIntuition);
    OS4_DropInterface((void *)&ILayers);
    OS4_DropInterface((void *)&IGraphics);

    OS4_CloseLibrary(&DOSBase);
    OS4_CloseLibrary(&TextClipBase);
    OS4_CloseLibrary(&KeymapBase);
    OS4_CloseLibrary(&WorkbenchBase);
    OS4_CloseLibrary(&IconBase);
    OS4_CloseLibrary(&IntuitionBase);
    OS4_CloseLibrary(&LayersBase);
    OS4_CloseLibrary(&GfxBase);
}

static void
OS4_FindApplicationName(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    size_t size;
    const size_t maxPathLen = 255;
    char pathBuffer[maxPathLen];

    if (IDOS->GetCliProgramName(pathBuffer, maxPathLen - 1)) {
        dprintf("GetCliProgramName: '%s'\n", pathBuffer);
    } else {
        dprintf("Failed to get CLI program name, checking task node\n");

        struct Task* me = IExec->FindTask(NULL);
        SDL_snprintf(pathBuffer, maxPathLen, "%s", ((struct Node *)me)->ln_Name);
    }

    size = SDL_strlen(pathBuffer) + 1;

    data->appName = SDL_malloc(size);

    if (data->appName) {
        SDL_snprintf(data->appName, size, pathBuffer);
    }

    dprintf("Application name: '%s'\n", data->appName);
}

static SDL_bool
OS4_AllocSystemResources(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    dprintf("Called\n");

    if (!OS4_OpenLibraries(_this)) {
        return SDL_FALSE;
    }

    OS4_FindApplicationName(_this);

    if (!(data->userPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_DONE))) {
        SDL_SetError("Couldn't allocate message port");
        return SDL_FALSE;
    }

    if (!(data->appMsgPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_DONE))) {
        SDL_SetError("Couldn't allocate AppMsg port");
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
                                             ASOIOR_Size,       sizeof(struct IOStdReq),
                                             ASOIOR_ReplyPort,  data->inputPort,
                                             TAG_DONE))) {

        SDL_SetError("Couldn't allocate input request");
        return SDL_FALSE;
    }

    if (IExec->OpenDevice("input.device", 0, (struct IORequest *)data->inputReq, 0))
    {
        SDL_SetError("Couldn't open input.device");
        return SDL_FALSE;
    }

    IInput = (struct InputIFace *)OS4_GetInterface((struct Library *)data->inputReq->io_Device);
    if (!IInput) {
        SDL_SetError("Failed to get IInput interface");
        return SDL_FALSE;
    }

    return SDL_TRUE;
}

static void
OS4_FreeSystemResources(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    dprintf("Called\n");

    OS4_DropInterface((void *)&IInput);

    if (data->inputReq) {
        dprintf("Closing input.device\n");
        //IExec->AbortIO((struct IORequest *)data->inputReq);
        //IExec->WaitIO((struct IORequest *)data->inputReq);
        IExec->CloseDevice((struct IORequest *)data->inputReq);

        dprintf("Freeing IORequest\n");
        IExec->FreeSysObject(ASOT_IOREQUEST, (void *)data->inputReq);
    }

    if (data->inputPort) {
        dprintf("Freeing MsgPort\n");
        IExec->FreeSysObject(ASOT_PORT, (void *)data->inputPort);
    }

    if (data->pool) {
        dprintf("Freeing memory pool\n");
        IExec->FreeSysObject(ASOT_MEMPOOL, data->pool);
    }

    if (data->appMsgPort) {
        struct Message *msg;

        dprintf("Replying app messages\n");

        while ((msg = IExec->GetMsg(data->appMsgPort))) {
            IExec->ReplyMsg((struct Message *) msg);
        }

        dprintf("Freeing app message port\n");

        IExec->FreeSysObject(ASOT_PORT, data->appMsgPort);
    }

    if (data->userPort) {
        dprintf("Freeing user port\n");
        IExec->FreeSysObject(ASOT_PORT, data->userPort);
    }

    if (data->appName) {
        SDL_free(data->appName);
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

static void
OS4_SetMiniGLFunctions(SDL_VideoDevice * device)
{
    device->GL_GetProcAddress = OS4_GL_GetProcAddress;
    device->GL_UnloadLibrary = OS4_GL_UnloadLibrary;
    device->GL_MakeCurrent = OS4_GL_MakeCurrent;
    device->GL_GetDrawableSize = OS4_GL_GetDrawableSize;
    device->GL_SetSwapInterval = OS4_GL_SetSwapInterval;
    device->GL_GetSwapInterval = OS4_GL_GetSwapInterval;
    device->GL_SwapWindow = OS4_GL_SwapWindow;
    device->GL_CreateContext = OS4_GL_CreateContext;
    device->GL_DeleteContext = OS4_GL_DeleteContext;

    OS4_ResizeGlContext = OS4_GL_ResizeContext;
    OS4_UpdateGlWindowPointer = OS4_GL_UpdateWindowPointer;
}

#if SDL_VIDEO_OPENGL_ES2
static void
OS4_SetGLESFunctions(SDL_VideoDevice * device)
{
    /* Some functions are recycled from SDL_os4opengl.c 100% ... */
    device->GL_GetProcAddress = OS4_GLES_GetProcAddress;
    device->GL_UnloadLibrary = OS4_GLES_UnloadLibrary;
    device->GL_MakeCurrent = OS4_GLES_MakeCurrent;
    device->GL_GetDrawableSize = OS4_GL_GetDrawableSize;
    device->GL_SetSwapInterval = OS4_GL_SetSwapInterval;
    device->GL_GetSwapInterval = OS4_GL_GetSwapInterval;
    device->GL_SwapWindow = OS4_GLES_SwapWindow;
    device->GL_CreateContext = OS4_GLES_CreateContext;
    device->GL_DeleteContext = OS4_GLES_DeleteContext;

    OS4_ResizeGlContext = OS4_GLES_ResizeContext;
    OS4_UpdateGlWindowPointer = OS4_GLES_UpdateWindowPointer;
}
#endif

static SDL_bool
OS4_IsMiniGL(_THIS)
{
    if ((_this->gl_config.profile_mask == 0) &&
        (_this->gl_config.major_version == 1) &&
        (_this->gl_config.minor_version == 3)) {
            dprintf("OpenGL 1.3 requested\n");
            return SDL_TRUE;
    }

    return SDL_FALSE;
}

#if SDL_VIDEO_OPENGL_ES2
static SDL_bool
OS4_IsOpenGLES2(_THIS)
{
    if ((_this->gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_ES) &&
        (_this->gl_config.major_version == 2) &&
        (_this->gl_config.minor_version == 0)) {
            dprintf("OpenGL ES 2.0 requested\n");
            return SDL_TRUE;
    }

    return SDL_FALSE;
}
#endif

static int
OS4_LoadGlLibrary(_THIS, const char * path)
{
    dprintf("Profile_mask %d, major ver %d, minor ver %d\n",
        _this->gl_config.profile_mask,
        _this->gl_config.major_version,
        _this->gl_config.minor_version);

    if (OS4_IsMiniGL(_this)) {
        OS4_SetMiniGLFunctions(_this);
        return OS4_GL_LoadLibrary(_this, path);
    }

#if SDL_VIDEO_OPENGL_ES2
    if (OS4_IsOpenGLES2(_this)) {
        OS4_SetGLESFunctions(_this);
        return OS4_GLES_LoadLibrary(_this, path);
    }
#endif

    dprintf("Invalid OpenGL version\n");
    SDL_SetError("Invalid OpenGL version");
    return -1;
}

static void
OS4_SetFunctionPointers(SDL_VideoDevice * device)
{
    device->VideoInit = OS4_VideoInit;
    device->VideoQuit = OS4_VideoQuit;

    device->GetDisplayBounds = OS4_GetDisplayBounds;
    device->GetDisplayModes = OS4_GetDisplayModes;
    device->SetDisplayMode = OS4_SetDisplayMode;

    device->CreateSDLWindow = OS4_CreateWindow;
    device->CreateSDLWindowFrom = OS4_CreateWindowFrom;
    device->SetWindowTitle = OS4_SetWindowTitle;
    //device->SetWindowIcon = OS4_SetWindowIcon;
    device->SetWindowPosition = OS4_SetWindowPosition;
    device->SetWindowSize = OS4_SetWindowSize;
    device->ShowWindow = OS4_ShowWindow;
    device->HideWindow = OS4_HideWindow;
    device->RaiseWindow = OS4_RaiseWindow;

    device->SetWindowMinimumSize = OS4_SetWindowMinMaxSize;
    device->SetWindowMaximumSize = OS4_SetWindowMinMaxSize;

    device->MaximizeWindow = OS4_MaximizeWindow;
    device->MinimizeWindow = OS4_MinimizeWindow;
    device->RestoreWindow = OS4_RestoreWindow;

    device->SetWindowResizable = OS4_SetWindowResizable;

    //device->SetWindowBordered = OS4_SetWindowBordered; // Not supported by SetWindowAttrs()?
    device->SetWindowFullscreen = OS4_SetWindowFullscreen;
    //device->SetWindowGammaRamp = OS4_SetWindowGammaRamp;
    //device->GetWindowGammaRamp = OS4_GetWindowGammaRamp;
    device->SetWindowGrab = OS4_SetWindowGrab;
    device->DestroyWindow = OS4_DestroyWindow;

    device->CreateWindowFramebuffer = OS4_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = OS4_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = OS4_DestroyWindowFramebuffer;

    //device->OnWindowEnter = OS4_OnWindowEnter;
    device->SetWindowHitTest = OS4_SetWindowHitTest;

    device->SetWindowOpacity = OS4_SetWindowOpacity;
    device->GetWindowBordersSize = OS4_GetWindowBordersSize;

    device->shape_driver.CreateShaper = OS4_CreateShaper;
    device->shape_driver.SetWindowShape = OS4_SetWindowShape;
    device->shape_driver.ResizeWindowShape = OS4_ResizeWindowShape;

    device->GetWindowWMInfo = OS4_GetWindowWMInfo;

    device->GL_LoadLibrary = OS4_LoadGlLibrary;
    OS4_SetMiniGLFunctions(device);

    device->PumpEvents = OS4_PumpEvents;
    //device->SuspendScreenSaver = OS4_SuspendScreenSaver;
    device->SetClipboardText = OS4_SetClipboardText;
    device->GetClipboardText = OS4_GetClipboardText;
    device->HasClipboardText = OS4_HasClipboardText;
    //device->ShowMessageBox = OS4_ShowMessageBox; Can be called without video initialization

    device->free = OS4_DeleteDevice;
}

static SDL_VideoDevice *
OS4_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;
    SDL_VideoData *data;
    SDL_version version;

    SDL_GetVersion(&version);

    dprintf("*** SDL %d.%d.%d video initialization starts ***\n",
        version.major, version.minor, version.patch);

#ifdef __AMIGADATE__
    dprintf("Build date: " __AMIGADATE__ "\n");
#endif

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
        /* If we return with NULL, SDL_VideoQuit() can't clean up OS4 stuff. So let's do it now. */
        OS4_FreeSystemResources(device);

        SDL_free(device);
        SDL_free(data);

        SDL_Unsupported();

        return NULL;
    }

    OS4_SetFunctionPointers(device);

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
        return SDL_SetError("Failed to initialize modes");
    }

    OS4_InitKeyboard(_this);
    OS4_InitMouse(_this);

    // We don't want SDL to change  window setup in SDL_OnWindowFocusLost()
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    return 0;
}

void
OS4_VideoQuit(_THIS)
{
    dprintf("Called\n");

    OS4_QuitMouse(_this);
    OS4_QuitKeyboard(_this);
    OS4_QuitModes(_this);
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

/* Native window apps may be interested in calling this */
struct MsgPort *
OS4_GetSharedMessagePort()
{
    SDL_VideoDevice *vd = SDL_GetVideoDevice();

    if (vd) {
        SDL_VideoData *data = (SDL_VideoData *) vd->driverdata;
        if (data) {
            return data->userPort;
        }
    }

    return NULL;
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
