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

/* This is the rect EnumModes2 uses */
struct DirectFBEnumRect
{
    SDL_Rect r;
    struct DirectFBEnumRect *next;
};

struct DirectFB_GLContext
{
    IDirectFBGL *context;
};

/* Initialization/Query functions */
static int DirectFB_VideoInit(_THIS);
static void DirectFB_VideoQuit(_THIS);

static int DirectFB_CreateWindow(_THIS, SDL_Window * window);
static int DirectFB_CreateWindowFrom(_THIS, SDL_Window * window,
                                     const void *data);
static void DirectFB_SetWindowTitle(_THIS, SDL_Window * window);
static void DirectFB_SetWindowPosition(_THIS, SDL_Window * window);
static void DirectFB_SetWindowSize(_THIS, SDL_Window * window);
static void DirectFB_ShowWindow(_THIS, SDL_Window * window);
static void DirectFB_HideWindow(_THIS, SDL_Window * window);
static void DirectFB_RaiseWindow(_THIS, SDL_Window * window);
static void DirectFB_MaximizeWindow(_THIS, SDL_Window * window);
static void DirectFB_MinimizeWindow(_THIS, SDL_Window * window);
static void DirectFB_RestoreWindow(_THIS, SDL_Window * window);
static void DirectFB_SetWindowGrab(_THIS, SDL_Window * window);
static void DirectFB_DestroyWindow(_THIS, SDL_Window * window);
static SDL_bool DirectFB_GetWindowWMInfo(_THIS, SDL_Window * window,
                                         struct SDL_SysWMinfo *info);

static void DirectFB_GetDisplayModes(_THIS);
static int DirectFB_SetDisplayMode(_THIS, SDL_DisplayMode * mode);

static int DirectFB_SetDisplayGammaRamp(_THIS, Uint16 * ramp);
static int DirectFB_GetDisplayGammaRamp(_THIS, Uint16 * ramp);

#if SDL_DIRECTFB_OPENGL
static int DirectFB_GL_LoadLibrary(_THIS, const char *path);
static void DirectFB_GL_UnloadLibrary(_THIS);
static void *DirectFB_GL_GetProcAddress(_THIS, const char *proc);
static SDL_GLContext DirectFB_GL_CreateContext(_THIS, SDL_Window * window);
static int DirectFB_GL_MakeCurrent(_THIS, SDL_Window * window,
                                   SDL_GLContext context);
static int DirectFB_GL_SetSwapInterval(_THIS, int interval);
static int DirectFB_GL_GetSwapInterval(_THIS);
static void DirectFB_GL_SwapWindow(_THIS, SDL_Window * window);
static void DirectFB_GL_DeleteContext(_THIS, SDL_GLContext context);

#endif

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
    SDL_DFB_CALLOC(device->gl_data, 1, sizeof(*device->gl_data));

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

VideoBootStrap DirectFB_bootstrap = {
    "directfb", "DirectFB",
    DirectFB_Available, DirectFB_CreateDevice
};

static DFBEnumerationResult
EnumModesCallback(int width, int height, int bpp, void *data)
{
    SDL_VideoDisplay *this = (SDL_VideoDisplay *) data;
    DFB_DisplayData *dispdata = (DFB_DisplayData *) this->driverdata;
    SDL_DisplayMode mode;

    mode.w = width;
    mode.h = height;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    mode.format = 0;

    if (dispdata->nummodes < DFB_MAX_MODES) {
        dispdata->modelist[dispdata->nummodes++] = mode;
    }

    SDL_DFB_DEBUG("w %d h %d bpp %d\n", width, height, bpp);
    return DFENUM_OK;
}

static int
DFBToSDLPixelFormat(DFBSurfacePixelFormat pixelformat, Uint32 * fmt)
{
    switch (pixelformat) {
    case DSPF_ALUT44:
        *fmt = SDL_PIXELFORMAT_INDEX4LSB;
        break;
    case DSPF_LUT8:
        *fmt = SDL_PIXELFORMAT_INDEX8;
        break;
    case DSPF_RGB332:
        *fmt = SDL_PIXELFORMAT_RGB332;
        break;
    case DSPF_ARGB4444:
        *fmt = SDL_PIXELFORMAT_ARGB4444;
        break;
    case SDL_PIXELFORMAT_ARGB1555:
        *fmt = SDL_PIXELFORMAT_ARGB1555;
        break;
    case DSPF_RGB16:
        *fmt = SDL_PIXELFORMAT_RGB565;
        break;
    case DSPF_RGB24:
        *fmt = SDL_PIXELFORMAT_RGB24;
        break;
    case DSPF_RGB32:
        *fmt = SDL_PIXELFORMAT_RGB888;
        break;
    case DSPF_ARGB:
        *fmt = SDL_PIXELFORMAT_ARGB8888;
        break;
    case DSPF_YV12:
        *fmt = SDL_PIXELFORMAT_YV12;
        break;                  /* Planar mode: Y + V + U  (3 planes) */
    case DSPF_I420:
        *fmt = SDL_PIXELFORMAT_IYUV;
        break;                  /* Planar mode: Y + U + V  (3 planes) */
    case DSPF_YUY2:
        *fmt = SDL_PIXELFORMAT_YUY2;
        break;                  /* Packed mode: Y0+U0+Y1+V0 (1 plane) */
    case DSPF_UYVY:
        *fmt = SDL_PIXELFORMAT_UYVY;
        break;                  /* Packed mode: U0+Y0+V0+Y1 (1 plane) */
    default:
        return -1;
    }
    return 0;
}

static DFBEnumerationResult
cbScreens(DFBScreenID screen_id, DFBScreenDescription desc,
          void *callbackdata)
{
    DFB_DeviceData *devdata = (DFB_DeviceData *) callbackdata;

    devdata->screenid[devdata->numscreens++] = screen_id;
    return DFENUM_OK;
}

DFBEnumerationResult
cbLayers(DFBDisplayLayerID layer_id, DFBDisplayLayerDescription desc,
         void *callbackdata)
{
    DFB_DeviceData *devdata = (DFB_DeviceData *) callbackdata;

    if (desc.caps & DLCAPS_SURFACE) {
        if ((desc.type & DLTF_GRAPHICS) && (desc.type & DLTF_VIDEO)) {
            if (devdata->vidlayer[devdata->aux] == -1)
                devdata->vidlayer[devdata->aux] = layer_id;
        } else if (desc.type & DLTF_GRAPHICS) {
            if (devdata->gralayer[devdata->aux] == -1)
                devdata->gralayer[devdata->aux] = layer_id;
        }
    }
    return DFENUM_OK;
}

static int
DirectFB_VideoInit(_THIS)
{
#if (DIRECTFB_MAJOR_VERSION == 0) && (DIRECTFB_MINOR_VERSION == 9) && (DIRECTFB_MICRO_VERSION < 23)
    DFBCardCapabilities caps;
#else
    DFBGraphicsDeviceDescription caps;
#endif
    DFBDisplayLayerConfig dlc;
    struct DirectFBEnumRect *rect;
    IDirectFB *dfb = NULL;
    IDirectFBDisplayLayer *layer = NULL;

    SDL_VideoDisplay display;
    DFB_DisplayData *dispdata;
    DFB_DeviceData *devdata;
    SDL_DisplayMode mode;
    int i;
    DFBResult ret;
    int tcw[DFB_MAX_SCREENS];
    int tch[DFB_MAX_SCREENS];

    SDL_DFB_CHECKERR(DirectFBInit(NULL, NULL));
    SDL_DFB_CHECKERR(DirectFBCreate(&dfb));

    SDL_DFB_CALLOC(devdata, 1, sizeof(*devdata));
    devdata->numscreens = 0;
    for (i = 0; i < DFB_MAX_SCREENS; i++) {
        devdata->gralayer[i] = -1;
        devdata->vidlayer[i] = -1;
    }
    SDL_DFB_CHECKERR(dfb->EnumScreens(dfb, &cbScreens, devdata));
    for (i = 0; i < devdata->numscreens; i++) {
        IDirectFBScreen *screen;

        SDL_DFB_CHECKERR(dfb->GetScreen(dfb, devdata->screenid[i], &screen));

        devdata->aux = i;
        SDL_DFB_CHECKERR(screen->EnumDisplayLayers
                         (screen, &cbLayers, devdata));
#if (DIRECTFB_MAJOR_VERSION >= 1)
        screen->GetSize(screen, &tcw[i], &tch[i]);
#else
        /* FIXME: this is only used to center windows
         *        Should be done otherwise, e.g. get surface from layer
         */
        tcw[i] = 800;
        tch[i] = 600;
#endif
        screen->Release(screen);
    }

    /* Query card capabilities */

    dfb->GetDeviceDescription(dfb, &caps);

    SDL_DFB_DEBUG("SDL directfb video driver - %s %s\n", __DATE__, __TIME__);
    SDL_DFB_DEBUG("Using %s (%s) driver.\n", caps.name, caps.vendor);
    SDL_DFB_DEBUG("Found %d screens\n", devdata->numscreens);

    for (i = 0; i < devdata->numscreens; i++) {
        //SDL_DFB_CHECKERR( dfb->GetDisplayLayer (dfb, DLID_PRIMARY, &layer) );
        SDL_DFB_CHECKERR(dfb->GetDisplayLayer
                         (dfb, devdata->gralayer[i], &layer));
        //SDL_DFB_CHECKERR( dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_FALSE, &events) );

        SDL_DFB_CHECKERR(layer->SetCooperativeLevel
                         (layer, DLSCL_ADMINISTRATIVE));
        layer->EnableCursor(layer, 1);
        SDL_DFB_CHECKERR(layer->SetCursorOpacity(layer, 0xC0));
        SDL_DFB_CHECKERR(layer->SetCooperativeLevel(layer, DLSCL_SHARED));

        /* Query layer configuration to determine the current mode and pixelformat */
        layer->GetConfiguration(layer, &dlc);

        DFBToSDLPixelFormat(dlc.pixelformat, &mode.format);

        mode.w = dlc.width;
        mode.h = dlc.height;
        mode.refresh_rate = 0;
        mode.driverdata = NULL;

        SDL_DFB_CALLOC(dispdata, 1, sizeof(*dispdata));

        dispdata->layer = layer;
        dispdata->pixelformat = dlc.pixelformat;
        dispdata->cw = tcw[i];
        dispdata->ch = tch[i];

        /* YUV - Video layer */

        dispdata->vidID = devdata->vidlayer[i];
        dispdata->vidIDinuse = 0;

        SDL_zero(display);

        display.desktop_mode = mode;
        display.current_mode = mode;
        display.driverdata = dispdata;

        /* Enumerate the available fullscreen modes */
        SDL_DFB_CALLOC(dispdata->modelist, DFB_MAX_MODES,
                       sizeof(SDL_DisplayMode));
        SDL_DFB_CHECKERR(dfb->EnumVideoModes
                         (dfb, EnumModesCallback, &display));

        SDL_AddVideoDisplay(&display);
    }

    devdata->initialized = 1;
    devdata->dfb = dfb;
    devdata->firstwin = NULL;

    _this->driverdata = devdata;


#if SDL_DIRECTFB_OPENGL
    /* Opengl */
    _this->gl_data->gl_active = 0;
    _this->gl_data->gl_context = NULL;
#endif

    DirectFB_AddRenderDriver(_this);
    DirectFB_InitMouse(_this);
    DirectFB_InitKeyboard(_this);
    //devdata->mouse = SDL_AddMouse(&mouse, -1);

    return 0;


  error:
    //FIXME: Cleanup not complete, Free existing displays
    SDL_DFB_FREE(dispdata);
    SDL_DFB_FREE(dispdata->modelist);
    SDL_DFB_RELEASE(layer);
    SDL_DFB_RELEASE(dfb);
    return -1;
}

static void
DirectFB_VideoQuit(_THIS)
{
    DFB_DeviceData *devdata = (DFB_DeviceData *) _this->driverdata;
    SDL_DisplayMode tmode;
    DFBResult ret;
    int i;

    tmode = _this->displays[0].desktop_mode;
    tmode.format = SDL_PIXELFORMAT_UNKNOWN;
    DirectFB_SetDisplayMode(_this, &tmode);
    tmode = _this->displays[0].desktop_mode;
    DirectFB_SetDisplayMode(_this, &tmode);

    for (i = 0; i < devdata->numscreens; i++) {
        DFB_DisplayData *dispdata =
            (DFB_DisplayData *) _this->displays[i].driverdata;
        if (dispdata->layer) {
            SDL_DFB_CHECK(dispdata->
                          layer->SetCooperativeLevel(dispdata->layer,
                                                     DLSCL_ADMINISTRATIVE));
            SDL_DFB_CHECK(dispdata->
                          layer->SetCursorOpacity(dispdata->layer, 0x00));
            SDL_DFB_CHECK(dispdata->
                          layer->SetCooperativeLevel(dispdata->layer,
                                                     DLSCL_SHARED));
        }
        SDL_DFB_RELEASE(dispdata->layer);

        /* Free video mode list */
        if (dispdata->modelist) {
            SDL_free(dispdata->modelist);
            dispdata->modelist = NULL;
        }
        // Done by core
        //SDL_free(dispdata);
    }

    SDL_DFB_RELEASE(devdata->dfb);

    SDL_DelMouse(devdata->mouse);
    SDL_DelKeyboard(devdata->keyboard);

#if SDL_DIRECTFB_OPENGL
    DirectFB_GL_UnloadLibrary(_this);
#endif

    devdata->initialized = 0;
}


static DFBSurfacePixelFormat
SDLToDFBPixelFormat(Uint32 format)
{
    switch (format) {
    case SDL_PIXELFORMAT_INDEX4LSB:
        return DSPF_ALUT44;
    case SDL_PIXELFORMAT_INDEX8:
        return DSPF_LUT8;
    case SDL_PIXELFORMAT_RGB332:
        return DSPF_RGB332;
    case SDL_PIXELFORMAT_RGB555:
        return DSPF_ARGB1555;
    case SDL_PIXELFORMAT_ARGB4444:
        return DSPF_ARGB4444;
    case SDL_PIXELFORMAT_ARGB1555:
        return DSPF_ARGB1555;
    case SDL_PIXELFORMAT_RGB565:
        return DSPF_RGB16;
    case SDL_PIXELFORMAT_RGB24:
        return DSPF_RGB24;
    case SDL_PIXELFORMAT_RGB888:
        return DSPF_RGB32;
    case SDL_PIXELFORMAT_ARGB8888:
        return DSPF_ARGB;
    case SDL_PIXELFORMAT_YV12:
        return DSPF_YV12;       /* Planar mode: Y + V + U  (3 planes) */
    case SDL_PIXELFORMAT_IYUV:
        return DSPF_I420;       /* Planar mode: Y + U + V  (3 planes) */
    case SDL_PIXELFORMAT_YUY2:
        return DSPF_YUY2;       /* Packed mode: Y0+U0+Y1+V0 (1 plane) */
    case SDL_PIXELFORMAT_UYVY:
        return DSPF_UYVY;       /* Packed mode: U0+Y0+V0+Y1 (1 plane) */
    case SDL_PIXELFORMAT_YVYU:
        return DSPF_UNKNOWN;    /* Packed mode: Y0+V0+Y1+U0 (1 plane) */
    case SDL_PIXELFORMAT_INDEX1LSB:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_INDEX1MSB:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_INDEX4MSB:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_RGB444:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_BGR24:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_BGR888:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_RGBA8888:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_ABGR8888:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_BGRA8888:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_ARGB2101010:
        return DSPF_UNKNOWN;
    default:
        return DSPF_UNKNOWN;
    }
}

static void
CheckSetDisplayMode(_THIS, DFB_DisplayData * data, SDL_DisplayMode * mode)
{
    DFBDisplayLayerConfig config;
    DFBDisplayLayerConfigFlags failed;

    config.width = mode->w;
    config.height = mode->h;
    config.pixelformat = SDLToDFBPixelFormat(mode->format);
    config.flags = DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT;
    failed = 0;
    data->layer->TestConfiguration(data->layer, &config, &failed);
    if (failed == 0)
        SDL_AddDisplayMode(_this->current_display, mode);

}

static void
DirectFB_GetDisplayModes(_THIS)
{
    //SDL_DisplayData *data = (SDL_DisplayData *) SDL_CurrentDisplay.driverdata;
    //SDL_DisplayMode mode;
    //SDL_AddDisplayMode(_this->current_display, &mode);

    SDL_DFB_DEVICEDATA(_this);
    DFB_DisplayData *data = (DFB_DisplayData *) SDL_CurrentDisplay.driverdata;
    int i;
    SDL_DisplayMode mode;

    for (i = 0; i < data->nummodes; ++i) {
        mode = data->modelist[i];
        //mode.format = SDL_PIXELFORMAT_UNKNOWN;

        mode.format = SDL_PIXELFORMAT_INDEX8;
        CheckSetDisplayMode(_this, data, &mode);
        mode.format = SDL_PIXELFORMAT_RGB565;
        CheckSetDisplayMode(_this, data, &mode);
        mode.format = SDL_PIXELFORMAT_RGB24;
        CheckSetDisplayMode(_this, data, &mode);
        mode.format = SDL_PIXELFORMAT_RGB888;
        CheckSetDisplayMode(_this, data, &mode);
    }
}

int
DirectFB_SetDisplayMode(_THIS, SDL_DisplayMode * mode)
{
    SDL_DFB_DEVICEDATA(_this);
    DFB_DisplayData *data = (DFB_DisplayData *) SDL_CurrentDisplay.driverdata;
    DFBDisplayLayerConfig config, rconfig;
    DFBDisplayLayerConfigFlags fail = 0;
    DFBResult ret;
    DFB_WindowData *win;

    SDL_DFB_CHECKERR(data->layer->SetCooperativeLevel(data->layer,
                                                      DLSCL_ADMINISTRATIVE));

    SDL_DFB_CHECKERR(data->layer->GetConfiguration(data->layer, &config));
    config.flags = DLCONF_WIDTH | DLCONF_HEIGHT;        // | DLCONF_BUFFERMODE;
    if (mode->format != SDL_PIXELFORMAT_UNKNOWN) {
        config.flags |= DLCONF_PIXELFORMAT;
        config.pixelformat = SDLToDFBPixelFormat(mode->format);
        data->pixelformat = config.pixelformat;
    }
    config.width = mode->w;
    config.height = mode->h;

    //config.buffermode = DLBM_BACKVIDEO;

    //config.pixelformat = GetFormatForBpp (bpp, HIDDEN->layer);

    data->layer->TestConfiguration(data->layer, &config, &fail);
    if (fail & (DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT)) {
        SDL_DFB_DEBUG("Error setting mode %dx%d-%x\n", mode->w, mode->h,
                      mode->format);
        return -1;
    }
    SDL_DFB_DEBUG("Trace\n");
    config.flags &= ~fail;
    SDL_DFB_CHECKERR(data->layer->SetConfiguration(data->layer, &config));
    SDL_DFB_CHECKERR(data->layer->SetCooperativeLevel(data->layer,
                                                      DLSCL_ADMINISTRATIVE));

    /* Double check */
    SDL_DFB_CHECKERR(data->layer->GetConfiguration(data->layer, &rconfig));

    if ((config.width != rconfig.width) ||
        (config.height != rconfig.height) ||
        ((mode->format != SDL_PIXELFORMAT_UNKNOWN)
         && (config.pixelformat != rconfig.pixelformat))) {
        SDL_DFB_DEBUG("Error setting mode %dx%d-%x\n", mode->w, mode->h,
                      mode->format);
        return -1;
    }

    data->pixelformat = rconfig.pixelformat;
    data->cw = config.width;
    data->ch = config.height;
    SDL_CurrentDisplay.current_mode = *mode;

    /*
     * FIXME: video mode switch is currently broken
     * 
     * DirectFB 1.2.0-rc1 even has a broken cursor after a switch
     * The following code needs to be revisited whether it is still 
     * needed once the switch works again.
     */

    win = devdata->firstwin;

    while (win) {
        SDL_DFB_RELEASE(win->surface);
        SDL_DFB_CHECKERR(win->window->GetSurface(win->window, &win->surface));
        win = win->next;
    }


    return 0;
  error:
    return -1;
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

static int
DirectFB_CreateWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_DISPLAYDATA(_this, window);
    DFB_WindowData *windata;
    DFBWindowOptions wopts;
    DFBWindowDescription desc;
    int ret, x, y;

    SDL_DFB_DEBUG("Trace x %d y %d w %d h %d\n", window->x, window->y,
                  window->w, window->h);
    window->driverdata = NULL;
    SDL_DFB_CALLOC(window->driverdata, 1, sizeof(DFB_WindowData));
    windata = (DFB_WindowData *) window->driverdata;

    SDL_DFB_CHECKERR(devdata->
                     dfb->SetCooperativeLevel(devdata->dfb, DFSCL_NORMAL));
    SDL_DFB_CHECKERR(dispdata->
                     layer->SetCooperativeLevel(dispdata->layer,
                                                DLSCL_ADMINISTRATIVE));

    /* Fill the window description. */
    if (window->x == SDL_WINDOWPOS_CENTERED) {
        x = (dispdata->cw - window->w) / 2;
    } else if (window->x == SDL_WINDOWPOS_UNDEFINED) {
        x = 0;
    } else {
        x = window->x;
    }
    if (window->y == SDL_WINDOWPOS_CENTERED) {
        y = (dispdata->ch - window->h) / 2;
    } else if (window->y == SDL_WINDOWPOS_UNDEFINED) {
        y = 0;
    } else {
        y = window->y;
    }
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        x = 0;
        y = 0;
    }

    desc.flags = DWDESC_WIDTH | DWDESC_HEIGHT /*| DWDESC_CAPS */  | DWDESC_PIXELFORMAT
        | DWDESC_SURFACE_CAPS;

#if (DIRECTFB_MAJOR_VERSION == 1) && (DIRECTFB_MINOR_VERSION >= 0)
    /* Needed for 1.2 */
    desc.flags |= DWDESC_POSX | DWDESC_POSY;
    desc.posx = x;
    desc.posy = y;
#else
    if (!(window->flags & SDL_WINDOW_FULLSCREEN)
        && window->x != SDL_WINDOWPOS_UNDEFINED
        && window->y != SDL_WINDOWPOS_UNDEFINED) {
        desc.flags |= DWDESC_POSX | DWDESC_POSY;
        desc.posx = x;
        desc.posy = y;
    }
#endif

    desc.width = window->w;
    desc.height = window->h;
    desc.pixelformat = dispdata->pixelformat;
    desc.caps = 0;              // DWCAPS_DOUBLEBUFFER;
    desc.surface_caps = DSCAPS_DOUBLE | DSCAPS_TRIPLE / DSCAPS_PREMULTIPLIED;

    /* Create the window. */
    SDL_DFB_CHECKERR(dispdata->layer->CreateWindow(dispdata->layer, &desc,
                                                   &windata->window));

    windata->window->GetOptions(windata->window, &wopts);
#if (DIRECTFB_MAJOR_VERSION == 1) && (DIRECTFB_MINOR_VERSION >= 0)

    if (window->flags & SDL_WINDOW_RESIZABLE)
        wopts |= DWOP_SCALE;
    else
        wopts |= DWOP_KEEP_SIZE;
#else
    wopts |= DWOP_KEEP_SIZE;    // if not we will crash ...
#endif

    if (window->flags & SDL_WINDOW_FULLSCREEN)
        wopts |= DWOP_KEEP_POSITION | DWOP_KEEP_STACKING | DWOP_KEEP_SIZE;

    windata->window->SetOptions(windata->window, wopts);
    /* Get the window's surface. */
    SDL_DFB_CHECKERR(windata->
                     window->GetSurface(windata->window, &windata->surface));
    windata->window->SetOpacity(windata->window, 0xFF);
    SDL_DFB_CHECKERR(windata->window->CreateEventBuffer(windata->window,
                                                        &
                                                        (windata->
                                                         eventbuffer)));
    SDL_DFB_CHECKERR(windata->window->
                     EnableEvents(windata->window,
                                  DWET_POSITION | DWET_SIZE | DWET_CLOSE |
                                  DWET_ALL));

    if (window->flags & SDL_WINDOW_FULLSCREEN)
        windata->window->SetStackingClass(windata->window, DWSC_UPPER);
    /* Make it the top most window. */
    windata->window->RaiseToTop(windata->window);

    windata->window->GetID(windata->window, &windata->windowID);
    windata->id = window->id;

#if SDL_DIRECTFB_OPENGL
    if (window->flags & SDL_WINDOW_OPENGL) {
        if (!_this->gl_config.driver_loaded) {
            /* no driver has been loaded, use default (ourselves) */
            if (DirectFB_GL_LoadLibrary(_this, NULL) < 0) {
                goto error;
            }
        }
        _this->gl_data->gl_active = 1;
    }
#endif

    /* Add to list ... */

    windata->next = devdata->firstwin;
    windata->opacity = 0xFF;
    devdata->firstwin = windata;

    //SDL_DFB_CHECKERR( windata->surface->GetPalette(windata->surface, &windata->palette) );

    return 0;
  error:
    SDL_DFB_RELEASE(windata->window);
    SDL_DFB_RELEASE(windata->surface);
    return -1;
}

static int
DirectFB_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    SDL_Unsupported();
    return -1;
}

static void
DirectFB_SetWindowTitle(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    SDL_Unsupported();
    //return -1;

}

static void
DirectFB_SetWindowPosition(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);
    int x, y;

    if (window->y == SDL_WINDOWPOS_UNDEFINED)
        y = 0;
    else
        y = window->y;

    if (window->x == SDL_WINDOWPOS_UNDEFINED)
        x = 0;
    else
        x = window->x;

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        x = 0;
        y = 0;
    }
    //if (!(window->flags & SDL_WINDOW_FULLSCREEN))
    windata->window->MoveTo(windata->window, x, y);
}

static void
DirectFB_SetWindowSize(_THIS, SDL_Window * window)
{
    int ret;
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    if (!(window->flags & SDL_WINDOW_FULLSCREEN)) {
        int ch, cw;

        //      SDL_DFB_DEBUG("Resize %d %d %d %d\n", cw, ch, window->w, window->h);
#if (DIRECTFB_MAJOR_VERSION == 1) && (DIRECTFB_MINOR_VERSION >= 0)
        SDL_DFB_CHECKERR(windata->window->
                         ResizeSurface(windata->window, window->w,
                                       window->h));
#else
        SDL_DFB_CHECKERR(windata->window->
                         Resize(windata->window, window->w, window->h));
#endif
        SDL_DFB_CHECKERR(windata->window->GetSize(windata->window, &window->w, &window->h));    /* if a window manager should have decided otherwise */
    }
  error:
    return;
}

static void
DirectFB_ShowWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    windata->window->SetOpacity(windata->window, windata->opacity);

}

static void
DirectFB_HideWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    windata->window->GetOpacity(windata->window, &windata->opacity);
    windata->window->SetOpacity(windata->window, 0);

}

static void
DirectFB_RaiseWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    windata->window->Raise(windata->window);

}

static void
DirectFB_MaximizeWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    SDL_Unsupported();

}

static void
DirectFB_MinimizeWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    SDL_Unsupported();

}

static void
DirectFB_RestoreWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    SDL_Unsupported();

}

static void
DirectFB_SetWindowGrab(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    SDL_Unsupported();

}

static void
DirectFB_DestroyWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);
    DFB_WindowData *p;

    SDL_DFB_DEBUG("Trace\n");

    SDL_DFB_RELEASE(windata->palette);
    SDL_DFB_RELEASE(windata->eventbuffer);
    SDL_DFB_RELEASE(windata->surface);
    SDL_DFB_RELEASE(windata->window);

    /* Remove from list ... */

    p = devdata->firstwin;
    while (p && p->next != windata)
        p = p->next;
    if (p)
        p->next = windata->next;
    else
        devdata->firstwin = windata->next;
    SDL_free(windata);
}

static SDL_bool
DirectFB_GetWindowWMInfo(_THIS, SDL_Window * window,
                         struct SDL_SysWMinfo *info)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);

    SDL_Unsupported();
    return SDL_FALSE;
}

#if SDL_DIRECTFB_OPENGL

#define OPENGL_REQUIRS_DLOPEN
#if defined(OPENGL_REQUIRS_DLOPEN) && defined(SDL_LOADSO_DLOPEN)
#include <dlfcn.h>
#define GL_LoadObject(X)	dlopen(X, (RTLD_NOW|RTLD_GLOBAL))
#define GL_LoadFunction		dlsym
#define GL_UnloadObject		dlclose
#else
#define GL_LoadObject	SDL_LoadObject
#define GL_LoadFunction	SDL_LoadFunction
#define GL_UnloadObject	SDL_UnloadObject
#endif

static int
DirectFB_GL_LoadLibrary(_THIS, const char *path)
{
    SDL_DFB_DEVICEDATA(_this);
#
    void *handle = NULL;

    SDL_DFB_DEBUG("Loadlibrary : %s\n", path);

    if (_this->gl_data->gl_active) {
        SDL_SetError("OpenGL context already created");
        return -1;
    }


    if (path == NULL) {
        path = SDL_getenv("SDL_VIDEO_GL_DRIVER");
        if (path == NULL) {
            path = "libGL.so";
        }
    }

    handle = GL_LoadObject(path);
    if (handle == NULL) {
        SDL_DFB_ERR("Library not found: %s\n", path);
        /* SDL_LoadObject() will call SDL_SetError() for us. */
        return -1;
    }

    SDL_DFB_DEBUG("Loaded library: %s\n", path);

    /* Unload the old driver and reset the pointers */
    DirectFB_GL_UnloadLibrary(_this);

    _this->gl_config.dll_handle = handle;
    _this->gl_config.driver_loaded = 1;
    if (path) {
        SDL_strlcpy(_this->gl_config.driver_path, path,
                    SDL_arraysize(_this->gl_config.driver_path));
    } else {
        *_this->gl_config.driver_path = '\0';
    }

    devdata->glFinish = DirectFB_GL_GetProcAddress(_this, "glFinish");
    devdata->glFlush = DirectFB_GL_GetProcAddress(_this, "glFlush");
    return 0;
}

static void
DirectFB_GL_UnloadLibrary(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);

    int ret;

    if (_this->gl_config.driver_loaded) {

        ret = GL_UnloadObject(_this->gl_config.dll_handle);
        if (ret)
            SDL_DFB_ERR("Error #%d trying to unload library.\n", ret);
        _this->gl_config.dll_handle = NULL;
        _this->gl_config.driver_loaded = 0;
    }
}

static void *
DirectFB_GL_GetProcAddress(_THIS, const char *proc)
{
    SDL_DFB_DEVICEDATA(_this);
    int ret;
    void *handle;

    SDL_DFB_DEBUG("Trace %s\n", proc);
    handle = _this->gl_config.dll_handle;
    return GL_LoadFunction(handle, proc);
}

static SDL_GLContext
DirectFB_GL_CreateContext(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);
    int ret;
    IDirectFBGL *context = NULL;

    SDL_DFB_DEBUG("Trace\n");
    SDL_DFB_CHECKERR(windata->surface->GetGL(windata->surface, &context));
    SDL_DFB_CHECKERR(context->Unlock(context));

    if (DirectFB_GL_MakeCurrent(_this, window, context) < 0) {
        DirectFB_GL_DeleteContext(_this, context);
        return NULL;
    }

    return context;

  error:
    return NULL;
}

static int
DirectFB_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);
    IDirectFBGL *dfb_context = (IDirectFBGL *) context;
    int ret;

    if (dfb_context) {
        dfb_context->Unlock(dfb_context);
        SDL_DFB_CHECKERR(dfb_context->Lock(dfb_context));
    }
    if (windata)
        windata->gl_context = dfb_context;

    return 0;
  error:
    return -1;
}

static int
DirectFB_GL_SetSwapInterval(_THIS, int interval)
{
    SDL_DFB_DEVICEDATA(_this);

    SDL_Unsupported();
    return -1;

}

static int
DirectFB_GL_GetSwapInterval(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);

    SDL_Unsupported();
    return -1;

}

static void
DirectFB_GL_SwapWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    SDL_DFB_DISPLAYDATA(_this, window);
    int ret;
    void *p;
    int pitch;
    DFBRegion region;

    region.x1 = 0;
    region.y1 = 0;
    region.x2 = window->w;
    region.y2 = window->h;

    if (devdata->glFinish)
        devdata->glFinish();
    else if (devdata->glFlush)
        devdata->glFlush();

    SDL_DFB_CHECKERR(windata->gl_context->Unlock(windata->gl_context));
    SDL_DFB_CHECKERR(windata->
                     surface->Flip(windata->surface, &region, DSFLIP_ONSYNC));
    SDL_DFB_CHECKERR(windata->gl_context->Lock(windata->gl_context));

    return;
  error:
    return;
}

static void
DirectFB_GL_DeleteContext(_THIS, SDL_GLContext context)
{
    IDirectFBGL *dfb_context = (IDirectFBGL *) context;
    SDL_DFB_DEVICEDATA(_this);

    dfb_context->Unlock(dfb_context);
    dfb_context->Release(dfb_context);
}

#endif
