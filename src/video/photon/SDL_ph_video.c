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

#ifdef HAVE_OPENGL
int ph_SetupOpenGLContext(_THIS, int width, int height, int bpp, Uint32 flags);
static void ph_GL_SwapBuffers(_THIS);
static int ph_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
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
    if ( (device == NULL) || (device->hidden == NULL) ) {
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
    device->UpdateMouse = NULL;	
    device->SetColors = ph_SetColors;
    device->UpdateRects = NULL;         /* ph_ResizeImage */
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
    device->GL_LoadLibrary = NULL;
    device->GL_GetProcAddress = NULL;
    device->GL_MakeCurrent = NULL;
#ifdef HAVE_OPENGL
    device->GL_SwapBuffers = ph_GL_SwapBuffers;
    device->GL_GetAttribute = ph_GL_GetAttribute;
#else
    device->GL_SwapBuffers = NULL;
    device->GL_GetAttribute = NULL;
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

static int ph_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
    PgVideoModeInfo_t my_mode_info;
    PgHWCaps_t my_hwcaps;

    window=NULL;
    desktoppal=SDLPH_PAL_NONE;
#ifdef HAVE_OPENGL
    oglctx=NULL;
#endif /* HAVE_OPENGL */
    
    captionflag=0;
    old_video_mode=-1;
    old_refresh_rate=-1;
	
    if (NULL == (event = malloc(EVENT_SIZE)))
    {
        exit(EXIT_FAILURE);
    }

    /* Create the blank cursor */
    SDL_BlankCursor = this->CreateWMCursor(this, blank_cdata, blank_cmask,
                                          (int)BLANK_CWIDTH, (int)BLANK_CHEIGHT,
                                          (int)BLANK_CHOTX, (int)BLANK_CHOTY);

    if (SDL_BlankCursor == NULL)
    {
        printf("ph_VideoInit: could not create blank cursor\n");
    }

    if (PgGetGraphicsHWCaps(&my_hwcaps) < 0)
    {
        fprintf(stderr,"ph_VideoInit: GetGraphicsHWCaps failed!! \n");
    }

    if (PgGetVideoModeInfo(my_hwcaps.current_video_mode, &my_mode_info) < 0)
    {
        fprintf(stderr,"ph_VideoInit:  PgGetVideoModeInfo failed\n");
    }

    /* We need to return BytesPerPixel as it in used by CreateRGBsurface */
    vformat->BitsPerPixel = my_mode_info.bits_per_pixel;
    vformat->BytesPerPixel = my_mode_info.bytes_per_scanline/my_mode_info.width;
    desktopbpp = my_mode_info.bits_per_pixel;
    
    /* save current palette */
    if (desktopbpp==8)
    {
        PgGetPalette(ph_palette);
    }
         
    currently_fullscreen = 0;
    
    this->info.wm_available = 1;
    
    return 0;
}

static SDL_Surface *ph_SetVideoMode(_THIS, SDL_Surface *current,
                int width, int height, int bpp, Uint32 flags)
{
    PgDisplaySettings_t settings;
    int mode;
    PtArg_t arg[32];
    PhDim_t dim;
    int rtnval;
    int i;
    unsigned long *tempptr;
    int pargc;

    dim.w=width;
    dim.h=height;

    /* Lock the event thread, in multi-threading environments */
    SDL_Lock_EventThread();

    current->flags = flags;

    /* create window if no OpenGL support selected */
    if ((flags & SDL_OPENGL)!=SDL_OPENGL)
    {
        pargc=0;
        
        // prevent using HWSURFACE in window mode if desktop bpp != chosen bpp
        if ((flags & SDL_HWSURFACE) && (!(flags & SDL_FULLSCREEN)))
        {
           if (desktopbpp!=bpp)
           {
              fprintf(stderr, "ph_SetVideoMode(): SDL_HWSURFACE available only with chosen bpp equal desktop bpp !\n");
              return NULL;
           }
        }

        PtSetArg(&arg[pargc++], Pt_ARG_DIM, &dim, 0);
        PtSetArg(&arg[pargc++], Pt_ARG_RESIZE_FLAGS, Pt_FALSE, Pt_RESIZE_XY_AS_REQUIRED);

        /* enable window minimizing */
        PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE, Ph_WM_HIDE);

        /* remove border and caption if no frame flag selected */
        if ((flags & SDL_NOFRAME) == SDL_NOFRAME)
        {
            PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, Ph_WM_RENDER_TITLE | Ph_WM_RENDER_BORDER);
        }
        else
        {
            PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_TRUE, Ph_WM_RENDER_TITLE | Ph_WM_RENDER_BORDER);
        }

        /* if window is not resizable then remove resize handles and maximize button */
        if ((flags & SDL_RESIZABLE) != SDL_RESIZABLE)
        {
            PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, Ph_WM_RENDER_RESIZE | Ph_WM_RENDER_MAX);
            PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_FALSE, Ph_WM_RESIZE | Ph_WM_MAX);
            PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_FALSE, Ph_WM_RESIZE | Ph_WM_MAX);
        }
        else
        {
            PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_TRUE, Ph_WM_RENDER_RESIZE | Ph_WM_RENDER_MAX);
            /* it is need to be Pt_FALSE to allow the application to process the resize callback */
            PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_FALSE, Ph_WM_RESIZE | Ph_WM_MAX);
            PtSetArg(&arg[pargc++], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_TRUE, Ph_WM_RESIZE | Ph_WM_MAX);
        }

        if (window!=NULL)
        {
            PtUnrealizeWidget(window);
            PtDestroyWidget(window);
            window=NULL;
        }

        window=PtCreateWidget(PtWindow, NULL, pargc, arg);
        PtRealizeWidget(window);
        
        PtFlush();
    }

#ifdef HAVE_OPENGL
    if (flags & SDL_OPENGL)
    {
        /* ph_SetupOpenGLContext creates also window as need */
        if (ph_SetupOpenGLContext(this, width, height, bpp, flags)==0)
        {
            /* setup OGL update function ... ugly method */
            ph_ResizeImage(this, current, flags); 
        }
        else
        {
            /* if context creation fail, report no OpenGL to high level */
            current->flags=(flags & (~SDL_OPENGL));
        }
#else
    if (flags & SDL_OPENGL) /* if no built-in OpenGL support */
    {
        fprintf(stderr, "ph_SetVideoMode(): no OpenGL support, try to recompile library.\n");
        current->flags=(flags & (~SDL_OPENGL));
        return NULL;
#endif /* HAVE_OPENGL */
    }
    else
    {
        /* Initialize the window */
        if (flags & SDL_FULLSCREEN) /* Direct Context , assume SDL_HWSURFACE also set */
        {
            /* Get the video mode and set it */
            if (flags & SDL_ANYFORMAT)
            {
                if ((mode = get_mode_any_format(width, height, bpp)) == 0)
                {
                    fprintf(stderr,"ph_SetVideoMode(): get_mode_any_format failed !\n");
                    exit(1);
                }
            }
            else
            {
                if ((mode = get_mode(width, height, bpp)) == 0)
                {
                    fprintf(stderr,"ph_SetVideoMode(): get_mode failed !\n");
                    exit(1);
                }
            }
            
            if (bpp==8)
            {
               desktoppal=SDLPH_PAL_SYSTEM;
            }
            
            /* save old video mode caps */
            PgGetVideoMode(&settings);
            old_video_mode=settings.mode;
            old_refresh_rate=settings.refresh;

            /* setup new video mode */
            settings.mode = mode;
            settings.refresh = 0;
            settings.flags = 0;

            if (PgSetVideoMode(&settings) < 0)
            {
                fprintf(stderr,"ph_SetVideoMode(): PgSetVideoMode failed !\n");
            }

            current->flags = (flags & (~SDL_RESIZABLE)); /* no resize for Direct Context */

            /* Begin direct mode */
            ph_EnterFullScreen(this);

        } /* end fullscreen flag */
        else
        {
            /* Use offscreen memory iff SDL_HWSURFACE flag is set */
            if (flags & SDL_HWSURFACE)
            {
                /* no stretch blit in offscreen context */
                current->flags = (flags & (~SDL_RESIZABLE));
            }

            /* using palette emulation code in window mode */
            if (bpp==8)
            {
                if (desktopbpp>=15)
                {
                    desktoppal=SDLPH_PAL_EMULATE;
                }
                else
                {
                    desktoppal=SDLPH_PAL_SYSTEM;
                }
            }
            else
            {
               desktoppal=SDLPH_PAL_NONE;
            }
        }

	/* If we are setting video to use the palette make sure we have allocated memory for it */
	if (bpp==8)
	{
            current->format->palette = malloc(sizeof(SDL_Palette));
            memset(current->format->palette, 0, sizeof(SDL_Palette));
            current->format->palette->ncolors = 256;
            current->format->palette->colors = (SDL_Color *)malloc(256 *sizeof(SDL_Color));
            /* fill the palette */
            rtnval = PgGetPalette(ph_palette);

            tempptr = (unsigned long *)current->format->palette->colors;

            for(i=0; i<256; i++)
            {
                *tempptr = (((unsigned long)ph_palette[i]) << 8);
                tempptr++;
            }
        }

    }

    current->w = width;
    current->h = height;
    current->format->BitsPerPixel = bpp;
    current->format->BytesPerPixel = (bpp+7)/8;
    current->pitch = SDL_CalculatePitch(current);

    /* Must call at least once it setup image planes */
    rtnval = ph_ResizeImage(this, current, flags);
    
    if (rtnval==-1)
    {
        fprintf(stderr,"ph_SetVideoMode(): ph_ResizeImage failed !\n");
        return NULL;
    }

    /* delayed set caption call */
    if (captionflag)
    {
        ph_SetCaption(this, this->wm_title, NULL);
    }

    /* finish window drawing */
    PtFlush();

    SDL_Unlock_EventThread();

    /* We're done! */
    return (current);
}

static void ph_VideoQuit(_THIS)
{
#ifdef HAVE_OPENGL
    PhRegion_t region_info;
#endif /* HAVE_OPENGL */

    ph_DestroyImage(this, SDL_VideoSurface); 

    if (currently_fullscreen)
    {
        ph_LeaveFullScreen(this);
    }

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
    
    /* restore palette */
    if (desktoppal!=SDLPH_PAL_NONE)
    {
        PgSetPalette(ph_palette, 0, 0, _Pg_MAX_PALETTE, Pg_PALSET_GLOBAL | Pg_PALSET_FORCE_EXPOSE, 0);
    }

#ifdef HAVE_OPENGL
    if (oglctx)
    {
        PhDCSetCurrent(NULL);
        PhDCRelease(oglctx);
        oglctx=NULL;
    }
#endif /* HAVE_OPENGL */
}

static int ph_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
    int i;
    PhPoint_t point={0, 0};
    PgColor_t syspalph[_Pg_MAX_PALETTE];

    /* palette emulation code, using palette of the PhImage_t struct */
    if (desktoppal==SDLPH_PAL_EMULATE)
    {
        if ((SDL_Image) && (SDL_Image->palette))
        {
            for (i=firstcolor; i<firstcolor+ncolors; i++)
            {
                SDL_Image->palette[i]  = 0x00000000UL;
                SDL_Image->palette[i] |= colors[i-firstcolor].r<<16;
                SDL_Image->palette[i] |= colors[i-firstcolor].g<<8;
                SDL_Image->palette[i] |= colors[i-firstcolor].b;
            }

           /* image needs to be redrawed, very slow method */
           PgDrawPhImage(&point, SDL_Image, 0);
        }
    }
    else
    {
        if (desktoppal==SDLPH_PAL_SYSTEM)
        {
            for (i=firstcolor; i<firstcolor+ncolors; i++)
            {
                syspalph[i]  = 0x00000000UL;
                syspalph[i] |= colors[i-firstcolor].r<<16;
                syspalph[i] |= colors[i-firstcolor].g<<8;
                syspalph[i] |= colors[i-firstcolor].b;
            }

            if ((this->screen->flags & SDL_FULLSCREEN) != SDL_FULLSCREEN)
            {
                /* window mode must use soft palette */
                PgSetPalette((PgColor_t*)&syspalph[firstcolor], 0, firstcolor, ncolors, Pg_PALSET_SOFT, 0);
                /* image needs to be redrawed, very slow method */
                if (SDL_Image)
                {
                   PgDrawPhImage(&point, SDL_Image, 0);
                }
            }
            else
            {
                /* fullscreen mode must use hardware palette */
                PgSetPalette((PgColor_t*)&syspalph[firstcolor], 0, firstcolor, ncolors, Pg_PALSET_GLOBAL, 0);
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
    PtArg_t args[8];
    PhDim_t dim;
    uint64_t OGLAttrib[PH_OGL_MAX_ATTRIBS];
    int OGLargc;
    int pargc;

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
        fprintf(stderr,"ph_SetupOpenGLContext(): cannot create OpenGL context.\n");
        return (-1);
    }

    PhDCSetCurrent(oglctx);

    pargc=0;

    PtSetArg(&args[pargc++], Pt_ARG_DIM, &dim, 0);
    PtSetArg(&args[pargc++], Pt_ARG_RESIZE_FLAGS, Pt_FALSE, Pt_RESIZE_XY_AS_REQUIRED);
    PtSetArg(&args[pargc++], Pt_ARG_FILL_COLOR, Pg_BLACK, 0);

    if (flags & SDL_FULLSCREEN)
    {
        PhPoint_t pos;

        pos.x=0;
        pos.y=0;

        PtSetArg(&args[pargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, ~0);
        PtSetArg(&args[pargc++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE, Ph_WM_FFRONT | Ph_WM_CLOSE | Ph_WM_TOFRONT | Ph_WM_CONSWITCH);
        PtSetArg(&args[pargc++], Pt_ARG_WINDOW_STATE, Pt_TRUE, Ph_WM_STATE_ISFRONT | Ph_WM_STATE_ISFOCUS);
        PtSetArg(&args[pargc++], Pt_ARG_POS, &pos, 0);
    }
    else
    {
        /* remove border and caption if no frame flag selected */
        if ((flags & SDL_NOFRAME) == SDL_NOFRAME)
        {
            PtSetArg(&args[pargc++], Pt_ARG_WINDOW_RENDER_FLAGS, 0, Ph_WM_RENDER_TITLE | Ph_WM_RENDER_BORDER);
        }
        else
        {
           /* if window is not resizable then remove resize handles */
           if ((flags & SDL_RESIZABLE) != SDL_RESIZABLE)
           {
               PtSetArg(&args[pargc++], Pt_ARG_WINDOW_RENDER_FLAGS, 0, Ph_WM_RENDER_RESIZE);
           }
        }
    }

    if (window!=NULL)
    {
        PtUnrealizeWidget(window);
        PtDestroyWidget(window);
        window=NULL;
    }

    window=PtCreateWidget(PtWindow, NULL, pargc, args);
    PtRealizeWidget(window);

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

#endif /* HAVE_OPENGL */
