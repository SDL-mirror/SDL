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

#include "SDL_DirectFB_video.h"

#define DFB_MAX_MODES 200

struct scn_callback_t
{
    int numscreens;
    DFBScreenID screenid[DFB_MAX_SCREENS];
    DFBDisplayLayerID gralayer[DFB_MAX_SCREENS];
    DFBDisplayLayerID vidlayer[DFB_MAX_SCREENS];
    int aux;                    /* auxiliary integer for callbacks */
};

struct modes_callback_t
{
    int nummodes;
    SDL_DisplayMode *modelist;
};

static const struct {
    DFBSurfacePixelFormat dfb;
    Uint32 sdl;
} pixelformat_tab[] = 
{
    { DSPF_LUT8, SDL_PIXELFORMAT_INDEX8 },              /* 8 bit LUT (8 bit color and alpha lookup from palette) */
    { DSPF_RGB332, SDL_PIXELFORMAT_RGB332 },            /* 8 bit RGB (1 byte, red 3@5, green 3@2, blue 2@0) */
    { DSPF_ARGB4444, SDL_PIXELFORMAT_ARGB4444 },        /* 16 bit ARGB (2 byte, alpha 4@12, red 4@8, green 4@4, blue 4@0) */
    { DSPF_ARGB1555, SDL_PIXELFORMAT_ARGB1555 },        /* 16 bit ARGB (2 byte, alpha 1@15, red 5@10, green 5@5, blue 5@0) */
    { DSPF_RGB16, SDL_PIXELFORMAT_RGB565 },             /* 16 bit RGB (2 byte, red 5@11, green 6@5, blue 5@0) */
    { DSPF_RGB24, SDL_PIXELFORMAT_RGB24 },              /* 24 bit RGB (3 byte, red 8@16, green 8@8, blue 8@0) */
    { DSPF_RGB32, SDL_PIXELFORMAT_RGB888 },             /* 24 bit RGB (4 byte, nothing@24, red 8@16, green 8@8, blue 8@0) */
    { DSPF_ARGB, SDL_PIXELFORMAT_ARGB8888 },            /* 32 bit ARGB (4 byte, alpha 8@24, red 8@16, green 8@8, blue 8@0) */
    { DSPF_RGB444, SDL_PIXELFORMAT_RGB444 },            /* 16 bit RGB (2 byte, nothing @12, red 4@8, green 4@4, blue 4@0) */
    { DSPF_YV12, SDL_PIXELFORMAT_YV12 },                /* 12 bit YUV (8 bit Y plane followed by 8 bit quarter size V/U planes) */
    { DSPF_I420,SDL_PIXELFORMAT_IYUV },                 /* 12 bit YUV (8 bit Y plane followed by 8 bit quarter size U/V planes) */
    { DSPF_YUY2, SDL_PIXELFORMAT_YUY2 },                /* 16 bit YUV (4 byte/ 2 pixel, macropixel contains CbYCrY [31:0]) */
    { DSPF_UYVY, SDL_PIXELFORMAT_UYVY },                /* 16 bit YUV (4 byte/ 2 pixel, macropixel contains YCbYCr [31:0]) */
    { DSPF_RGB555, SDL_PIXELFORMAT_RGB555 },            /* 16 bit RGB (2 byte, nothing @15, red 5@10, green 5@5, blue 5@0) */

#if (DFB_VERSION_ATLEAST(1,2,0))
    { DSPF_BGR555, SDL_PIXELFORMAT_BGR555 },            /* 16 bit BGR (2 byte, nothing @15, blue 5@10, green 5@5, red 5@0) */
#else
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_BGR555 },
#endif

    /* Pfff ... nonmatching formats follow */    
    
    { DSPF_ALUT44, SDL_PIXELFORMAT_UNKNOWN },           /* 8 bit ALUT (1 byte, alpha 4@4, color lookup 4@0) */
 	{ DSPF_A8, SDL_PIXELFORMAT_UNKNOWN },               /* 	8 bit alpha (1 byte, alpha 8@0), e.g. anti-aliased glyphs */
 	{ DSPF_AiRGB, SDL_PIXELFORMAT_UNKNOWN },            /* 	32 bit ARGB (4 byte, inv. alpha 8@24, red 8@16, green 8@8, blue 8@0) */
 	{ DSPF_A1, SDL_PIXELFORMAT_UNKNOWN },               /* 	1 bit alpha (1 byte/ 8 pixel, most significant bit used first) */
 	{ DSPF_NV12, SDL_PIXELFORMAT_UNKNOWN },             /* 	12 bit YUV (8 bit Y plane followed by one 16 bit quarter size CbCr [15:0] plane) */
 	{ DSPF_NV16, SDL_PIXELFORMAT_UNKNOWN },             /* 	16 bit YUV (8 bit Y plane followed by one 16 bit half width CbCr [15:0] plane) */
 	{ DSPF_ARGB2554, SDL_PIXELFORMAT_UNKNOWN },         /* 	16 bit ARGB (2 byte, alpha 2@14, red 5@9, green 5@4, blue 4@0) */
 	{ DSPF_NV21, SDL_PIXELFORMAT_UNKNOWN },             /* 	12 bit YUV (8 bit Y plane followed by one 16 bit quarter size CrCb [15:0] plane) */
 	{ DSPF_AYUV, SDL_PIXELFORMAT_UNKNOWN },             /* 	32 bit AYUV (4 byte, alpha 8@24, Y 8@16, Cb 8@8, Cr 8@0) */
 	{ DSPF_A4, SDL_PIXELFORMAT_UNKNOWN },               /* 	4 bit alpha (1 byte/ 2 pixel, more significant nibble used first) */
 	{ DSPF_ARGB1666, SDL_PIXELFORMAT_UNKNOWN },         /* 	1 bit alpha (3 byte/ alpha 1@18, red 6@16, green 6@6, blue 6@0) */
 	{ DSPF_ARGB6666, SDL_PIXELFORMAT_UNKNOWN },         /* 	6 bit alpha (3 byte/ alpha 6@18, red 6@16, green 6@6, blue 6@0) */
 	{ DSPF_RGB18, SDL_PIXELFORMAT_UNKNOWN },            /* 	6 bit RGB (3 byte/ red 6@16, green 6@6, blue 6@0) */
 	{ DSPF_LUT2, SDL_PIXELFORMAT_UNKNOWN },             /* 	2 bit LUT (1 byte/ 4 pixel, 2 bit color and alpha lookup from palette) */

#if (DFB_VERSION_ATLEAST(1,3,0))
 	{ DSPF_RGBA4444, SDL_PIXELFORMAT_UNKNOWN },         /* 16 bit RGBA (2 byte, red 4@12, green 4@8, blue 4@4, alpha 4@0) */
#endif

#if (DFB_VERSION_ATLEAST(1,4,0))
 	{ DSPF_RGBA5551, SDL_PIXELFORMAT_UNKNOWN },         /* 	16 bit RGBA (2 byte, red 5@11, green 5@6, blue 5@1, alpha 1@0) */
 	{ DSPF_YUV444P, SDL_PIXELFORMAT_UNKNOWN },          /* 	24 bit full YUV planar (8 bit Y plane followed by an 8 bit Cb and an 8 bit Cr plane) */
 	{ DSPF_ARGB8565, SDL_PIXELFORMAT_UNKNOWN },         /* 	24 bit ARGB (3 byte, alpha 8@16, red 5@11, green 6@5, blue 5@0) */
 	{ DSPF_AVYU, SDL_PIXELFORMAT_UNKNOWN },             /* 	32 bit AVYU 4:4:4 (4 byte, alpha 8@24, Cr 8@16, Y 8@8, Cb 8@0) */
 	{ DSPF_VYU, SDL_PIXELFORMAT_UNKNOWN },              /* 	24 bit VYU 4:4:4 (3 byte, Cr 8@16, Y 8@8, Cb 8@0)  */
#endif
 	
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_INDEX1LSB },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_INDEX1MSB },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_INDEX4LSB }, 
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_INDEX4MSB },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_BGR24 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_BGR888 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_RGBA8888 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_ABGR8888 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_BGRA8888 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_ARGB2101010 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_ABGR4444 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_ABGR1555 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_BGR565 },
    { DSPF_UNKNOWN, SDL_PIXELFORMAT_YVYU },                        /**< Packed mode: Y0+V0+Y1+U0 (1 pla	*/
};

static Uint32
DFBToSDLPixelFormat(DFBSurfacePixelFormat pixelformat)
{
    int i;
    
    for (i=0; pixelformat_tab[i].dfb != DSPF_UNKNOWN; i++)
        if (pixelformat_tab[i].dfb == pixelformat)
        {
            return pixelformat_tab[i].sdl;
        }
    return SDL_PIXELFORMAT_UNKNOWN;
}

static DFBSurfacePixelFormat
SDLToDFBPixelFormat(Uint32 format)
{
    int i;
    
    for (i=0; pixelformat_tab[i].dfb != DSPF_UNKNOWN; i++)
        if (pixelformat_tab[i].sdl == format)
        {
            return pixelformat_tab[i].dfb;
        }
    return  DSPF_UNKNOWN;
}

static DFBEnumerationResult
EnumModesCallback(int width, int height, int bpp, void *data)
{
    struct modes_callback_t *modedata = (struct modes_callback_t *) data;
    SDL_DisplayMode mode;

    mode.w = width;
    mode.h = height;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    mode.format = SDL_PIXELFORMAT_UNKNOWN;

    if (modedata->nummodes < DFB_MAX_MODES) {
        modedata->modelist[modedata->nummodes++] = mode;
    }

    return DFENUM_OK;
}

static DFBEnumerationResult
cbScreens(DFBScreenID screen_id, DFBScreenDescription desc,
          void *callbackdata)
{
    struct scn_callback_t *devdata = (struct scn_callback_t *) callbackdata;

    devdata->screenid[devdata->numscreens++] = screen_id;
    return DFENUM_OK;
}

DFBEnumerationResult
cbLayers(DFBDisplayLayerID layer_id, DFBDisplayLayerDescription desc,
         void *callbackdata)
{
    struct scn_callback_t *devdata = (struct scn_callback_t *) callbackdata;

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

static void
CheckSetDisplayMode(_THIS, SDL_VideoDisplay * display, DFB_DisplayData * data, SDL_DisplayMode * mode)
{
    SDL_DFB_DEVICEDATA(_this);
    DFBDisplayLayerConfig config;
    DFBDisplayLayerConfigFlags failed;

    SDL_DFB_CHECKERR(data->layer->SetCooperativeLevel(data->layer,
                                                      DLSCL_ADMINISTRATIVE));
    config.width = mode->w;
    config.height = mode->h;
    config.pixelformat = SDLToDFBPixelFormat(mode->format);
    config.flags = DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT;
    if (devdata->use_yuv_underlays) {
        config.flags |= DLCONF_OPTIONS;
        config.options = DLOP_ALPHACHANNEL;
    }
    failed = 0;
    data->layer->TestConfiguration(data->layer, &config, &failed);
    SDL_DFB_CHECKERR(data->layer->SetCooperativeLevel(data->layer,
                                                      DLSCL_SHARED));
    if (failed == 0)
        SDL_AddDisplayMode(display, mode);
    else
        SDL_DFB_ERR("Mode %d x %d not available: %x\n", mode->w,
                      mode->h, failed);

    return;
  error:
    return;
}

void
DirectFB_InitModes(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);
    IDirectFBDisplayLayer *layer = NULL;
    SDL_VideoDisplay display;
    DFB_DisplayData *dispdata = NULL;
    SDL_DisplayMode mode;
    DFBGraphicsDeviceDescription caps;
    DFBDisplayLayerConfig dlc;
    struct scn_callback_t *screencbdata;

    int tcw[DFB_MAX_SCREENS];
    int tch[DFB_MAX_SCREENS];
    int i;
    DFBResult ret;

    SDL_DFB_CALLOC(screencbdata, 1, sizeof(*screencbdata));

    screencbdata->numscreens = 0;

    for (i = 0; i < DFB_MAX_SCREENS; i++) {
        screencbdata->gralayer[i] = -1;
        screencbdata->vidlayer[i] = -1;
    }

    SDL_DFB_CHECKERR(devdata->dfb->EnumScreens(devdata->dfb, &cbScreens,
                                               screencbdata));

    for (i = 0; i < screencbdata->numscreens; i++) {
        IDirectFBScreen *screen;

        SDL_DFB_CHECKERR(devdata->dfb->GetScreen(devdata->dfb,
                                                 screencbdata->screenid
                                                 [i], &screen));

        screencbdata->aux = i;
        SDL_DFB_CHECKERR(screen->EnumDisplayLayers(screen, &cbLayers,
                                                   screencbdata));
        screen->GetSize(screen, &tcw[i], &tch[i]);
        screen->Release(screen);
    }

    /* Query card capabilities */

    devdata->dfb->GetDeviceDescription(devdata->dfb, &caps);

    SDL_DFB_DEBUG("SDL directfb video driver - %s %s\n", __DATE__, __TIME__);
    SDL_DFB_DEBUG("Using %s (%s) driver.\n", caps.name, caps.vendor);
    SDL_DFB_DEBUG("Found %d screens\n", screencbdata->numscreens);

    for (i = 0; i < screencbdata->numscreens; i++) {
        SDL_DFB_CHECKERR(devdata->dfb->GetDisplayLayer(devdata->dfb,
                                                       screencbdata->gralayer
                                                       [i], &layer));

        SDL_DFB_CHECKERR(layer->SetCooperativeLevel(layer,
                                                    DLSCL_ADMINISTRATIVE));
        layer->EnableCursor(layer, 1);
        SDL_DFB_CHECKERR(layer->SetCursorOpacity(layer, 0xC0));

        if (devdata->use_yuv_underlays) {
            dlc.flags = DLCONF_PIXELFORMAT | DLCONF_OPTIONS;
            dlc.pixelformat = DSPF_ARGB;
            dlc.options = DLOP_ALPHACHANNEL;

            ret = SDL_DFB_CHECK(layer->SetConfiguration(layer, &dlc));
            if (ret != DFB_OK) {
                /* try AiRGB if the previous failed */
                dlc.pixelformat = DSPF_AiRGB;
                SDL_DFB_CHECKERR(layer->SetConfiguration(layer, &dlc));
            }
        }

        /* Query layer configuration to determine the current mode and pixelformat */
        dlc.flags = DLCONF_ALL;
        SDL_DFB_CHECKERR(layer->GetConfiguration(layer, &dlc));

        mode.format = DFBToSDLPixelFormat(dlc.pixelformat);
        
        if (mode.format == SDL_PIXELFORMAT_UNKNOWN) {
            SDL_DFB_ERR("Unknown dfb pixelformat %x !\n", dlc.pixelformat);
            goto error;
        }

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

        dispdata->vidID = screencbdata->vidlayer[i];
        dispdata->vidIDinuse = 0;

        SDL_zero(display);

        display.desktop_mode = mode;
        display.current_mode = mode;
        display.driverdata = dispdata;

#if (DFB_VERSION_ATLEAST(1,2,0))
        dlc.flags =
            DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT |
            DLCONF_OPTIONS;
        ret = layer->SetConfiguration(layer, &dlc);
#endif

        SDL_DFB_CHECKERR(layer->SetCooperativeLevel(layer, DLSCL_SHARED));

        SDL_AddVideoDisplay(&display);
    }
    SDL_DFB_FREE(screencbdata);
    return;
  error:
    /* FIXME: Cleanup not complete, Free existing displays */
    SDL_DFB_FREE(dispdata);
    SDL_DFB_RELEASE(layer);
    return;
}

void
DirectFB_GetDisplayModes(_THIS, SDL_VideoDisplay * display)
{
    SDL_DFB_DEVICEDATA(_this);
    DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;
    SDL_DisplayMode mode;
    struct modes_callback_t data;
    int i;

    data.nummodes = 0;
    /* Enumerate the available fullscreen modes */
    SDL_DFB_CALLOC(data.modelist, DFB_MAX_MODES, sizeof(SDL_DisplayMode));
    SDL_DFB_CHECKERR(devdata->dfb->EnumVideoModes(devdata->dfb,
                                                  EnumModesCallback, &data));

    for (i = 0; i < data.nummodes; ++i) {
        mode = data.modelist[i];

        mode.format = SDL_PIXELFORMAT_ARGB8888;
        CheckSetDisplayMode(_this, display, dispdata, &mode);
        mode.format = SDL_PIXELFORMAT_RGB888;
        CheckSetDisplayMode(_this, display, dispdata, &mode);
        mode.format = SDL_PIXELFORMAT_RGB24;
        CheckSetDisplayMode(_this, display, dispdata, &mode);
        mode.format = SDL_PIXELFORMAT_RGB565;
        CheckSetDisplayMode(_this, display, dispdata, &mode);
        mode.format = SDL_PIXELFORMAT_INDEX8;
        CheckSetDisplayMode(_this, display, dispdata, &mode);
    }

    SDL_DFB_FREE(data.modelist);
error:
    return;
}

int
DirectFB_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    /*
     * FIXME: video mode switch is currently broken for 1.2.0
     * 
     */

    SDL_DFB_DEVICEDATA(_this);
    DFB_DisplayData *data = (DFB_DisplayData *) display->driverdata;
    DFBDisplayLayerConfig config, rconfig;
    DFBDisplayLayerConfigFlags fail = 0;

    SDL_DFB_CHECKERR(data->layer->SetCooperativeLevel(data->layer,
                                                      DLSCL_ADMINISTRATIVE));

    SDL_DFB_CHECKERR(data->layer->GetConfiguration(data->layer, &config));
    config.flags = DLCONF_WIDTH | DLCONF_HEIGHT;
    if (mode->format != SDL_PIXELFORMAT_UNKNOWN) {
        config.flags |= DLCONF_PIXELFORMAT;
        config.pixelformat = SDLToDFBPixelFormat(mode->format);
        data->pixelformat = config.pixelformat;
    }
    config.width = mode->w;
    config.height = mode->h;

    if (devdata->use_yuv_underlays) {
        config.flags |= DLCONF_OPTIONS;
        config.options = DLOP_ALPHACHANNEL;
    }

    data->layer->TestConfiguration(data->layer, &config, &fail);

    if (fail &
        (DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT |
         DLCONF_OPTIONS)) {
        SDL_DFB_ERR("Error setting mode %dx%d-%x\n", mode->w, mode->h,
                    mode->format);
        return -1;
    }

    SDL_DFB_DEBUG("Trace\n");
    config.flags &= ~fail;
    SDL_DFB_CHECKERR(data->layer->SetConfiguration(data->layer, &config));
#if (DFB_VERSION_ATLEAST(1,2,0))
    /* Need to call this twice ! */
    SDL_DFB_CHECKERR(data->layer->SetConfiguration(data->layer, &config));
#endif

    /* Double check */
    SDL_DFB_CHECKERR(data->layer->GetConfiguration(data->layer, &rconfig));
    SDL_DFB_CHECKERR(data->
                     layer->SetCooperativeLevel(data->layer, DLSCL_SHARED));

    if ((config.width != rconfig.width) || (config.height != rconfig.height)
        || ((mode->format != SDL_PIXELFORMAT_UNKNOWN)
            && (config.pixelformat != rconfig.pixelformat))) {
        SDL_DFB_ERR("Error setting mode %dx%d-%x\n", mode->w, mode->h,
                    mode->format);
        return -1;
    }

    data->pixelformat = rconfig.pixelformat;
    data->cw = config.width;
    data->ch = config.height;
    display->current_mode = *mode;

    return 0;
  error:
    return -1;
}

void
DirectFB_QuitModes(_THIS)
{
    //DFB_DeviceData *devdata = (DFB_DeviceData *) _this->driverdata;
    SDL_DisplayMode tmode;
    int i;

    for (i = 0; i < _this->num_displays; ++i) {
        SDL_VideoDisplay *display = &_this->displays[i];
        DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;

        SDL_GetDesktopDisplayModeForDisplay(display, &tmode);
        tmode.format = SDL_PIXELFORMAT_UNKNOWN;
        DirectFB_SetDisplayMode(_this, display, &tmode);

        SDL_GetDesktopDisplayModeForDisplay(display, &tmode);
        DirectFB_SetDisplayMode(_this, display, &tmode);

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
        SDL_DFB_RELEASE(dispdata->vidlayer);

    }
}

/* vi: set ts=4 sw=4 expandtab: */
