/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_timer.h"
#include "SDL_thread.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_endian.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"
#include "SDL_ph_video.h"
#include "SDL_ph_modes_c.h"
#include "SDL_ph_image_c.h"
#include "SDL_ph_events_c.h"
#include "SDL_ph_mouse_c.h"
#include "SDL_ph_wm_c.h"
#include "SDL_phyuv_c.h"
#include "blank_cursor.h"

static int ph_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Surface *ph_SetVideoMode(_THIS, SDL_Surface *current,
                int width, int height, int bpp, Uint32 flags);
static int ph_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void ph_VideoQuit(_THIS);
static void ph_DeleteDevice(SDL_VideoDevice *device);
static void ph_UpdateMouse(_THIS);

#ifdef HAVE_OPENGL
static int ph_SetupOpenGLContext(_THIS, int width, int height, int bpp, Uint32 flags);
static void ph_GL_SwapBuffers(_THIS);
static int ph_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
static int ph_GL_LoadLibrary(_THIS, const char* path);
static void* ph_GL_GetProcAddress(_THIS, const char* proc);

#endif /* HAVE_OPENGL */

static int ph_Available(void)
{
    int phstat=-1;

    phstat=PtInit(0);
    if (phstat==0)
    {
       return 1;
    }
    else
    {
       return 0;
    }
}

static SDL_VideoDevice *ph_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
    if (device) {
        memset(device, 0, (sizeof *device));
        device->hidden = (struct SDL_PrivateVideoData *)
                malloc((sizeof *device->hidden));
        device->gl_data = NULL;
    }
    if ((device == NULL) || (device->hidden == NULL)) {
        SDL_OutOfMemory();
        ph_DeleteDevice(device);
        return(0);
    }
    memset(device->hidden, 0, (sizeof *device->hidden));

    /* Set the driver flags */
    device->handles_any_size = 1; /* JB not true for fullscreen */

    /* Set the function pointers */
    device->CreateYUVOverlay = ph_CreateYUVOverlay;
    device->VideoInit = ph_VideoInit;
    device->ListModes = ph_ListModes;
    device->SetVideoMode = ph_SetVideoMode;
    device->ToggleFullScreen = ph_ToggleFullScreen;
    device->UpdateMouse = ph_UpdateMouse;
    device->SetColors = ph_SetColors;
    device->UpdateRects = NULL;         /* set up in ph_SetupUpdateFunction */
    device->VideoQuit = ph_VideoQuit;
    device->AllocHWSurface = ph_AllocHWSurface;
    device->CheckHWBlit = NULL;
    device->FillHWRect = NULL;
    device->SetHWColorKey = NULL;
    device->SetHWAlpha = NULL;
    device->LockHWSurface = ph_LockHWSurface;
    device->UnlockHWSurface = ph_UnlockHWSurface;
    device->FlipHWSurface = ph_FlipHWSurface;
    device->FreeHWSurface = ph_FreeHWSurface;
    device->SetCaption = ph_SetCaption;
    device->SetIcon = NULL;
    device->IconifyWindow = ph_IconifyWindow;
    device->GrabInput = ph_GrabInput;
    device->GetWMInfo = ph_GetWMInfo;
    device->FreeWMCursor = ph_FreeWMCursor;
    device->CreateWMCursor = ph_CreateWMCursor;
    device->ShowWMCursor = ph_ShowWMCursor;
    device->WarpWMCursor = ph_WarpWMCursor;
    device->CheckMouseMode = ph_CheckMouseMode;
    device->InitOSKeymap = ph_InitOSKeymap;
    device->PumpEvents = ph_PumpEvents;

    /* OpenGL support. */
    device->GL_MakeCurrent = NULL;
#ifdef HAVE_OPENGL
    device->GL_SwapBuffers = ph_GL_SwapBuffers;
    device->GL_GetAttribute = ph_GL_GetAttribute;
    device->GL_LoadLibrary = ph_GL_LoadLibrary;
    device->GL_GetProcAddress = ph_GL_GetProcAddress;
#else
    device->GL_SwapBuffers = NULL;
    device->GL_GetAttribute = NULL;
    device->GL_LoadLibrary = NULL;
    device->GL_GetProcAddress = NULL;
#endif /* HAVE_OPENGL */

    device->free = ph_DeleteDevice;
    
    return device;
}

VideoBootStrap ph_bootstrap = {
    "photon", "QNX Photon video output",
    ph_Available, ph_CreateDevice
};

static void ph_DeleteDevice(SDL_VideoDevice *device)
{
    if (device)
    {
        if (device->hidden)
        {
            free(device->hidden);
            device->hidden = NULL;
        }
        if (device->gl_data)
        {
            free(device->gl_data);
            device->gl_data = NULL;
        }
        free(device);
        device = NULL;
    }
}

static PtWidget_t *ph_CreateWindow(_THIS)
{
    PtWidget_t *widget;
    
    widget = PtCreateWidget(PtWindow, NULL, 0, 0);

    return widget;
}

static int ph_SetupWindow(_THIS, int w, int h, int flags)
{
    PtArg_t     args[32];
    PhPoint_t   pos = {0, 0};
    PhDim_t     dim = {w, h};
    int         nargs = 0;
    const char* windowpos;
    const char* iscentered;
    int         x, y;

    PtSetArg(&args[nargs++], Pt_ARG_DIM, &dim, 0);

    if ((flags & SDL_RESIZABLE) == SDL_RESIZABLE)
    {
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_FALSE, Ph_WM_RESIZE | Ph_WM_CLOSE);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE, Ph_WM_MAX | Ph_WM_RESTORE);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_TRUE, Ph_WM_RESIZE | Ph_WM_MOVE | Ph_WM_MAX | Ph_WM_RESTORE | Ph_WM_CLOSE);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_TRUE, Ph_WM_RENDER_RESIZE | Ph_WM_RENDER_MAX | Ph_WM_RENDER_COLLAPSE | Ph_WM_RENDER_RETURN);
    }
    else
    {
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_FALSE, Ph_WM_RESIZE | Ph_WM_MAX | Ph_WM_RESTORE | Ph_WM_CLOSE);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_FALSE, Ph_WM_RESIZE | Ph_WM_MAX | Ph_WM_RESTORE);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_TRUE, Ph_WM_MOVE | Ph_WM_CLOSE);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, Ph_WM_RENDER_RESIZE | Ph_WM_RENDER_MAX | Ph_WM_RENDER_COLLAPSE | Ph_WM_RENDER_RETURN);
    }

    if (((flags & SDL_NOFRAME)==SDL_NOFRAME) || ((flags & SDL_FULLSCREEN)==SDL_FULLSCREEN))
    {
       if ((flags & SDL_RESIZABLE) != SDL_RESIZABLE)
       {
           PtSetArg(&args[nargs++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, Pt_TRUE);
       }
       else
       {
           PtSetArg(&args[nargs++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, Pt_TRUE);
           PtSetArg(&args[nargs++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_TRUE, Ph_WM_RENDER_RESIZE | Ph_WM_RENDER_BORDER);
       }
    }
    else
    {
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_TRUE, Ph_WM_RENDER_BORDER | Ph_WM_RENDER_TITLE |
                                 Ph_WM_RENDER_CLOSE | Ph_WM_RENDER_MENU | Ph_WM_RENDER_MIN);
    }

    if ((flags & SDL_FULLSCREEN) == SDL_FULLSCREEN)
    {
        PtSetArg(&args[nargs++], Pt_ARG_POS, &pos, 0);
        PtSetArg(&args[nargs++], Pt_ARG_BASIC_FLAGS, Pt_TRUE, Pt_BASIC_PREVENT_FILL);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE, Ph_WM_FFRONT | Ph_WM_MAX);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_STATE, Pt_TRUE, Ph_WM_STATE_ISFRONT | Ph_WM_STATE_ISFOCUS | Ph_WM_STATE_ISALTKEY);
    }
    else
    {
        windowpos = getenv("SDL_VIDEO_WINDOW_POS");
	iscentered = getenv("SDL_VIDEO_CENTERED");

        if ((iscentered) || ((windowpos) && (strcmp(windowpos, "center")==0)))
        {
            pos.x = (desktop_mode.width - w)/2;
            pos.y = (desktop_mode.height - h)/2;
            PtSetArg(&args[nargs++], Pt_ARG_POS, &pos, 0);
	}
        else
        {
            if (windowpos)
            {
                if (sscanf(windowpos, "%d,%d", &x, &y) == 2 )
                {
                    pos.x=x;
                    pos.y=y;
                    PtSetArg(&args[nargs++], Pt_ARG_POS, &pos, 0);
                }
	    }
        }


        PtSetArg(&args[nargs++], Pt_ARG_FILL_COLOR, Pg_BLACK, 0);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_STATE, Pt_FALSE, Ph_WM_STATE_ISFRONT | Ph_WM_STATE_ISMAX | Ph_WM_STATE_ISALTKEY);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE, Ph_WM_HIDE);
        PtSetArg(&args[nargs++], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_TRUE, Ph_WM_HIDE);
        PtSetArg(&args[nargs++], Pt_ARG_RESIZE_FLAGS, Pt_FALSE, Pt_RESIZE_XY_AS_REQUIRED);
    }

    PtSetResources(window, nargs, args);
    PtRealizeWidget(window);
    PtWindowToFront(window);

    return 0;
}

static const struct ColourMasks* ph_GetColourMasks(int bpp)
{
    /* The alpha mask doesn't appears to be needed */
    static const struct ColourMasks phColorMasks[5] = {
        /*  8 bit      */  {0, 0, 0, 0, 8},
        /* 15 bit ARGB */  {0x7C00, 0x03E0, 0x001F, 0x8000, 15},
        /* 16 bit  RGB */  {0xF800, 0x07E0, 0x001F, 0x0000, 16},
        /* 24 bit  RGB */  {0xFF0000, 0x00FF00, 0x0000FF, 0x000000, 24},
        /* 32 bit ARGB */  {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 32},
    };

    switch (bpp)
    {
        case 8:
             return &phColorMasks[0];
        case 15:
             return &phColorMasks[1];
        case 16:
             return &phColorMasks[2];
        case 24:
             return &phColorMasks[3];
        case 32:
             return &phColorMasks[4];
    }
    return NULL;
}

static int ph_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
    PgHWCaps_t my_hwcaps;
    int i;

    window=NULL;
    desktoppal=SDLPH_PAL_NONE;
#ifdef HAVE_OPENGL
    oglctx=NULL;
#endif /* HAVE_OPENGL */
    
    old_video_mode=-1;
    old_refresh_rate=-1;
	
    if (NULL == (event = malloc(EVENT_SIZE)))
    {
        SDL_OutOfMemory();
        return -1;
    }
    memset(event, 0x00, EVENT_SIZE);

    window = ph_CreateWindow(this);
    if (window == NULL)
    {
        SDL_SetError("ph_VideoInit(): Couldn't create video window !\n");
        return -1;
    }

    /* Create the blank cursor */
    SDL_BlankCursor = this->CreateWMCursor(this, blank_cdata, blank_cmask,
                                          (int)BLANK_CWIDTH, (int)BLANK_CHEIGHT,
                                          (int)BLANK_CHOTX, (int)BLANK_CHOTY);

    if (SDL_BlankCursor == NULL)
    {
        return -1;
    }

    if (PgGetGraphicsHWCaps(&my_hwcaps) < 0)
    {
        SDL_SetError("ph_VideoInit(): GetGraphicsHWCaps function failed !\n");
        this->FreeWMCursor(this, SDL_BlankCursor);
        return -1;
    }

    if (PgGetVideoModeInfo(my_hwcaps.current_video_mode, &desktop_mode) < 0)
    {
        SDL_SetError("ph_VideoInit(): PgGetVideoModeInfo function failed !\n");
        this->FreeWMCursor(this, SDL_BlankCursor);
        return -1;
    }

    /* We need to return BytesPerPixel as it in used by CreateRGBsurface */
    vformat->BitsPerPixel = desktop_mode.bits_per_pixel;
    vformat->BytesPerPixel = desktop_mode.bytes_per_scanline/desktop_mode.width;
    desktopbpp = desktop_mode.bits_per_pixel;
    
    /* save current palette */
    if (desktopbpp==8)
    {
        PgGetPalette(savedpal);
        PgGetPalette(syspalph);
    }
    else
    {
        for(i=0; i<_Pg_MAX_PALETTE; i++)
        {
            savedpal[i]=PgRGB(0, 0, 0);
            syspalph[i]=PgRGB(0, 0, 0);
        }
    }
         
    currently_fullscreen = 0;
    currently_hided = 0;
    current_overlay = NULL;

    OCImage.direct_context = NULL;
    OCImage.offscreen_context = NULL;
    OCImage.offscreen_backcontext = NULL;
    OCImage.oldDC = NULL;
    OCImage.CurrentFrameData = NULL;
    OCImage.FrameData0 = NULL;
    OCImage.FrameData1 = NULL;

    
    this->info.wm_available = 1;
    
    return 0;
}

static SDL_Surface *ph_SetVideoMode(_THIS, SDL_Surface *current,
                int width, int height, int bpp, Uint32 flags)
{
    const struct ColourMasks* mask;

    /* Lock the event thread, in multi-threading environments */
    SDL_Lock_EventThread();

    current->flags = flags;

    /* if we do not have desired fullscreen mode, then fallback into window mode */
    if (((current->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) && (ph_GetVideoMode(width, height, bpp)==0))
    {
       current->flags &= ~SDL_FULLSCREEN;
       current->flags &= ~SDL_NOFRAME;
       current->flags &= ~SDL_RESIZABLE;
    }

    ph_SetupWindow(this, width, height, current->flags);

    mask = ph_GetColourMasks(bpp);
    if (mask != NULL)
    {
        SDL_ReallocFormat(current, mask->bpp, mask->red, mask->green, mask->blue, 0);
    }
    else
    {
        SDL_SetError("ph_SetVideoMode(): desired bpp is not supported by photon !\n");
        return NULL;
    }

#ifdef HAVE_OPENGL
    if (current->flags & SDL_OPENGL)
    {
        /* ph_SetupOpenGLContext creates also window as need */
        if (ph_SetupOpenGLContext(this, width, height, bpp, flags)==0)
        {
            ph_SetupUpdateFunction(this, current, flags); 
        }
        else
        {
            /* if context creation fail, report no OpenGL to high level */
            current->flags &= ~SDL_OPENGL;
            return NULL;
        }
#else
    if (current->flags & SDL_OPENGL) /* if no built-in OpenGL support */
    {
        SDL_SetError("ph_SetVideoMode(): no OpenGL support, try to recompile library.\n");
        current->flags &= ~SDL_OPENGL;
        return NULL;
#endif /* HAVE_OPENGL */
    }
    else
    {
        /* Initialize internal variables */
        if ((current->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN)
        {
            if (bpp==8)
            {
               desktoppal=SDLPH_PAL_SYSTEM;
            }

            current->flags &= ~SDL_RESIZABLE; /* no resize for Direct Context */
            current->flags |= SDL_HWSURFACE;
        }
        else
        {
            /* remove this if we'll support non-fullscreen sw/hw+doublebuf */
            current->flags &= ~SDL_DOUBLEBUF;

            /* Use offscreen memory if SDL_HWSURFACE flag is set */
            if ((current->flags & SDL_HWSURFACE) == SDL_HWSURFACE)
            {

                if (desktopbpp!=bpp)
                {
                   current->flags &= ~SDL_HWSURFACE;
                }
            }

            /* using palette emulation code in window mode */
            if (bpp==8)
            {
                if (desktopbpp>=15)
                {
                    desktoppal = SDLPH_PAL_EMULATE;
                }
                else
                {
                    desktoppal = SDLPH_PAL_SYSTEM;
                }
            }
            else
            {
               desktoppal = SDLPH_PAL_NONE;
            }
        }
    }

    current->w = width;
    current->h = height;

    if (desktoppal==SDLPH_PAL_SYSTEM)
    {
       current->flags|=SDL_HWPALETTE;
    }

    /* Must call at least once for setup image planes */
    if (ph_SetupUpdateFunction(this, current, current->flags)==-1)
    {
        return NULL;
    }

    /* finish window drawing, if we are not in fullscreen, of course */
    if ((current->flags & SDL_FULLSCREEN) != SDL_FULLSCREEN)
    {
       PtFlush();
    }
    else
    {
       PgFlush();
    }

    SDL_Unlock_EventThread();

    /* We're done! */
    return (current);
}

static void ph_VideoQuit(_THIS)
{
#ifdef HAVE_OPENGL
    PhRegion_t region_info;
#endif /* HAVE_OPENGL */

    /* restore palette */
    if (desktopbpp==8)
    {
        PgSetPalette(syspalph, 0, -1, 0, 0, 0);
        PgSetPalette(savedpal, 0, 0, _Pg_MAX_PALETTE, Pg_PALSET_GLOBAL | Pg_PALSET_FORCE_EXPOSE, 0);
        PgFlush();
    }

    ph_DestroyImage(this, SDL_VideoSurface); 

#ifdef HAVE_OPENGL
    /* prevent double SEGFAULT during parachute mode */
    if (this->screen)
    {
        if (((this->screen->flags & SDL_FULLSCREEN)==SDL_FULLSCREEN) &&
            ((this->screen->flags & SDL_OPENGL)==SDL_OPENGL))
        {
            region_info.cursor_type=Ph_CURSOR_POINTER;
            region_info.rid=PtWidgetRid(window);
            PhRegionChange(Ph_REGION_CURSOR, 0, &region_info, NULL, NULL);
        }
    }

    PtFlush();
#endif /* HAVE_OPENGL */
    
    if (window)
    {
        PtUnrealizeWidget(window);
        PtDestroyWidget(window);
        window=NULL;
    }

#ifdef HAVE_OPENGL
    if (oglctx)
    {
        PhDCSetCurrent(NULL);
        PhDCRelease(oglctx);
        oglctx=NULL;
    }
#endif /* HAVE_OPENGL */

    if (event!=NULL)
    {
        free(event);
        event=NULL;
    }
}

static int ph_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
    int i;
    SDL_Rect updaterect;

    updaterect.x = updaterect.y = 0;
    updaterect.w = this->screen->w;
    updaterect.h = this->screen->h;

    /* palette emulation code, using palette of the PhImage_t struct */
    if (desktoppal==SDLPH_PAL_EMULATE)
    {
        if ((SDL_Image) && (SDL_Image->palette))
        {
            for (i=firstcolor; i<firstcolor+ncolors; i++)
            {
                syspalph[i] = PgRGB(colors[i-firstcolor].r, colors[i-firstcolor].g, colors[i-firstcolor].b);
                SDL_Image->palette[i] = syspalph[i];
            }

            /* image needs to be redrawn */
            this->UpdateRects(this, 1, &updaterect);
        }
    }
    else
    {
        if (desktoppal==SDLPH_PAL_SYSTEM)
        {
            for (i=firstcolor; i<firstcolor+ncolors; i++)
            {
                syspalph[i] = PgRGB(colors[i-firstcolor].r, colors[i-firstcolor].g, colors[i-firstcolor].b);
            }

            if ((this->screen->flags & SDL_FULLSCREEN) != SDL_FULLSCREEN)
            {
                 /* window mode must use soft palette */
                PgSetPalette(&syspalph[firstcolor], 0, firstcolor, ncolors, Pg_PALSET_GLOBAL, 0);
                /* image needs to be redrawn */
                this->UpdateRects(this, 1, &updaterect);
            }
            else
            {
                /* fullscreen mode must use hardware palette */
                PgSetPalette(&syspalph[firstcolor], 0, firstcolor, ncolors, Pg_PALSET_GLOBAL, 0);
            }
        }
        else
        {
            /* SDLPH_PAL_NONE do nothing */
        }
    }
    
    return 1;
}

#ifdef HAVE_OPENGL

int ph_SetupOpenGLContext(_THIS, int width, int height, int bpp, Uint32 flags)
{
    PhDim_t dim;
    uint64_t OGLAttrib[PH_OGL_MAX_ATTRIBS];
    int OGLargc;

    dim.w=width;
    dim.h=height;
    
    if (oglctx!=NULL)
    {
       PhDCSetCurrent(NULL);
       PhDCRelease(oglctx);
       oglctx=NULL;
    }

    OGLargc=0;
    if (this->gl_config.depth_size)
    {
        OGLAttrib[OGLargc++]=PHOGL_ATTRIB_DEPTH_BITS;
        OGLAttrib[OGLargc++]=this->gl_config.depth_size;
    }
    if (this->gl_config.stencil_size)
    {
        OGLAttrib[OGLargc++]=PHOGL_ATTRIB_STENCIL_BITS;
        OGLAttrib[OGLargc++]=this->gl_config.stencil_size;
    }
    OGLAttrib[OGLargc++]=PHOGL_ATTRIB_FORCE_SW;
    if (flags & SDL_FULLSCREEN)
    {
        OGLAttrib[OGLargc++]=PHOGL_ATTRIB_FULLSCREEN;
        OGLAttrib[OGLargc++]=PHOGL_ATTRIB_DIRECT;
        OGLAttrib[OGLargc++]=PHOGL_ATTRIB_FULLSCREEN_BEST;
        OGLAttrib[OGLargc++]=PHOGL_ATTRIB_FULLSCREEN_CENTER;
    }
    OGLAttrib[OGLargc++]=PHOGL_ATTRIB_NONE;

    if (this->gl_config.double_buffer)
    {
        oglctx=PdCreateOpenGLContext(2, &dim, 0, OGLAttrib);
    }
    else
    {
        oglctx=PdCreateOpenGLContext(1, &dim, 0, OGLAttrib);
    }

    if (oglctx==NULL)
    {
        SDL_SetError("ph_SetupOpenGLContext(): cannot create OpenGL context !\n");
        return (-1);
    }

    PhDCSetCurrent(oglctx);

    /* disable mouse for fullscreen */
    if (flags & SDL_FULLSCREEN)
    {
        PhRegion_t region_info;

        region_info.cursor_type=Ph_CURSOR_NONE;
        region_info.rid=PtWidgetRid(window);
        PhRegionChange(Ph_REGION_CURSOR, 0, &region_info, NULL, NULL);
    }

    PtFlush();

    return 0;
}

void ph_GL_SwapBuffers(_THIS)
{
    PgSetRegion(PtWidgetRid(window));
    PdOpenGLContextSwapBuffers(oglctx);
}

int ph_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value)
{
    switch (attrib)
    {
        case SDL_GL_DOUBLEBUFFER:
             *value=this->gl_config.double_buffer;
             break;
        case SDL_GL_STENCIL_SIZE:
             *value=this->gl_config.stencil_size;
             break;
        case SDL_GL_DEPTH_SIZE:
             *value=this->gl_config.depth_size;
             break;
        default:
             *value=0;
             return(-1);
    }
    return 0;
}

int ph_GL_LoadLibrary(_THIS, const char* path)
{
   /* if code compiled with HAVE_OPENGL, the library already linked */
   this->gl_config.driver_loaded = 1;

   return 0;
}

void* ph_GL_GetProcAddress(_THIS, const char* proc)
{
   return NULL;
}

#endif /* HAVE_OPENGL */

static void ph_UpdateMouse(_THIS)
{
    PhCursorInfo_t phcursor;
    short abs_x;
    short abs_y;

    /* Lock the event thread, in multi-threading environments */
    SDL_Lock_EventThread();

    /* synchronizing photon mouse cursor position and SDL mouse position, if cursor appears over window. */
    PtGetAbsPosition(window, &abs_x, &abs_y);
    PhQueryCursor(PhInputGroup(NULL), &phcursor);
    if (((phcursor.pos.x >= abs_x) && (phcursor.pos.x <= abs_x + this->screen->w)) &&
        ((phcursor.pos.y >= abs_y) && (phcursor.pos.y <= abs_y + this->screen->h)))
    {
        SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
        SDL_PrivateMouseMotion(0, 0, phcursor.pos.x-abs_x, phcursor.pos.y-abs_y);
    }
    else
    {
        SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
    }

    /* Unlock the event thread, in multi-threading environments */
    SDL_Unlock_EventThread();
}
