/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
static void ph_GL_SwapBuffers(_THIS);

#ifdef HAVE_OPENGL
PdOpenGLContext_t* OGLContext=NULL;
#endif /* HAVE_OPENGL */

static int ph_Available(void)
{
        return 1;
}

static SDL_VideoDevice *ph_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
    if ( device ) {
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
    device->ToggleFullScreen =  ph_ToggleFullScreen;
    device->UpdateMouse = NULL;	
    device->SetColors = ph_SetColors;
    device->UpdateRects = NULL; /* ph_ResizeImage; */
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
    device->GetWMInfo = NULL;
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
    device->GL_GetAttribute = NULL;
    device->GL_MakeCurrent = NULL;
#ifdef HAVE_OPENGL
    device->GL_SwapBuffers = ph_GL_SwapBuffers;
#else
    device->GL_SwapBuffers = NULL;
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

    if ( device ) {
        if ( device->hidden ) {
            free(device->hidden);
            device->hidden = NULL;
        }
        if ( device->gl_data ) {
            free(device->gl_data);
            device->gl_data = NULL;
        }
        free(device);
        device = NULL;
    }
}

static int ph_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	PtArg_t arg[1];
	PhDim_t dim;
	PgColor_t ph_palette[_Pg_MAX_PALETTE];
	int i;
	unsigned long *tempptr;
	int rtnval;
//	PgDisplaySettings_t mysettings;
	PgVideoModeInfo_t my_mode_info;
	PgHWCaps_t my_hwcaps;
	
	if( NULL == ( event = malloc( EVENT_SIZE ) ) )
	   exit( EXIT_FAILURE );

	/* Create a widget 'window' to capture events */
    dim.w=0; //JB test320;
    dim.h=0; //JB test240;
    //We need to return BytesPerPixel as it in used by CreateRGBsurface
	
    PtSetArg(&arg[0], Pt_ARG_DIM, &dim,0);
    
/*    
    PtSetArg(&arg[1], Pt_ARG_RESIZE_FLAGS, Pt_TRUE, Pt_RESIZE_XY_AS_REQUIRED);
    PtSetArg(&arg[2], Pt_ARG_WINDOW_STATE, Pt_TRUE, Ph_WM_STATE_ISFRONT |
                        Ph_WM_STATE_ISMAX |
                        Ph_WM_STATE_ISFOCUS);

    PtSetArg(&arg[3], Pt_ARG_WINDOW_RENDER_FLAGS,Pt_FALSE,~0);
    PtSetArg(&arg[4], Pt_ARG_WINDOW_MANAGED_FLAGS,Pt_TRUE,
                        Ph_WM_FFRONT |
                        Ph_WM_CLOSE |
                        Ph_WM_TOFRONT |
                        Ph_WM_CONSWITCH);
*/    
    
    
	window=PtAppInit(NULL, NULL, NULL, 1, arg);

  	if(window == NULL)
  	{
  		printf("Photon application init failed, probably Photon is not running.\n");
        	exit( EXIT_FAILURE );
  	}

	//PgSetDrawBufferSize(16 *1024);
	PgSetRegion(PtWidgetRid(window));
	PgSetClipping(0,NULL);
	PtRealizeWidget(window);


    /* Get the available video modes */
//    if(ph_GetVideoModes(this) < 0)
//        return -1;

	/* Create the blank cursor */
	SDL_BlankCursor = this->CreateWMCursor(this, blank_cdata, blank_cmask,
                (int)BLANK_CWIDTH, (int)BLANK_CHEIGHT,
                   (int)BLANK_CHOTX, (int)BLANK_CHOTY);

	if(SDL_BlankCursor == NULL)
	  printf("could not create blank cursor\n");

         if (PgGetGraphicsHWCaps(&my_hwcaps) < 0)
         {
                fprintf(stderr,"ph_VideoInit:  GetGraphicsHWCaps failed!! \n");
         }
         if (PgGetVideoModeInfo(my_hwcaps.current_video_mode, &my_mode_info) < 0)
         {
                fprintf(stderr,"ph_VideoInit:  PgGetVideoModeInfo failed\n");
         }

         //We need to return BytesPerPixel as it in used by CreateRGBsurface
         vformat->BitsPerPixel = my_mode_info.bits_per_pixel;
         vformat->BytesPerPixel = my_mode_info.bytes_per_scanline/my_mode_info.width;
         
         //return a palette if we are in 256 color mode           
		if(vformat->BitsPerPixel == 8)
		{
			vformat->palette = malloc(sizeof(SDL_Palette));
			memset(vformat->palette, 0, sizeof(SDL_Palette));
			vformat->palette->ncolors = 256;
			vformat->palette->colors = (SDL_Color *)malloc(256 *sizeof(SDL_Color));
			
			//fill the palette
			rtnval = PgGetPalette(ph_palette);
			if(rtnval < 0)
			   printf("ph_VideoInit:  PgGetPalette failed\n");
			   
                        tempptr = (unsigned long *)vformat->palette->colors;

			for(i=0;i<256; i++)
			{
  				*tempptr = (((unsigned long)ph_palette[i]) << 8);
  				tempptr++;

			}		
		
		}


    currently_fullscreen = 0;
    
    this->info.wm_available = 1;
    
    return 0;
}

static SDL_Surface *ph_SetVideoMode(_THIS, SDL_Surface *current,
                int width, int height, int bpp, Uint32 flags)
{
    PgDisplaySettings_t settings;
/*
    PgHWCaps_t my_hwcaps;
    PgVideoModeInfo_t mode_info;
*/
    int mode, actual_width, actual_height;
    PtArg_t arg[5];
    PhDim_t dim;	
    int rtnval;
    PgColor_t ph_palette[_Pg_MAX_PALETTE];
    int i;
    unsigned long *tempptr;

#ifdef HAVE_OPENGL
    uint64_t OGLAttrib[PH_OGL_MAX_ATTRIBS];
#endif // HAVE_OPENGL

    actual_width = width;
    actual_height = height;

    dim.w=width;
    dim.h=height;

    /* Lock the event thread, in multi-threading environments */
    SDL_Lock_EventThread();

     /* Initialize the window */
    if (flags & SDL_FULLSCREEN) /* Direct Context , assume SDL_HWSURFACE also set */
    {
        /* Get the video mode and set it */
        if (flags & SDL_ANYFORMAT)
        {
            if ((mode = get_mode_any_format(width, height, bpp)) == 0)
            {
                fprintf(stderr,"error: get_mode_any_format failed\n");
                exit(1);
            }
        }
        else
        {
            if ((mode = get_mode(width, height, bpp)) == 0)
            {
                 	fprintf(stderr,"error: get_mode failed\n");
                	exit(1);
            }
               
           
        }
        settings.mode = mode;
        settings.refresh = 0;
        settings.flags  = 0;       

        if (PgSetVideoMode( &settings ) < 0)
        {
            fprintf(stderr,"error: PgSetVideoMode failed\n");
        }

		/* Get the true height and width */
		
      current->flags = (flags & (~SDL_RESIZABLE)); /* no resize for Direct Context */

		 /* Begin direct mode */
		 ph_EnterFullScreen(this);

       

    } /* end fullscreen flag */
    else
    {
       if (flags & SDL_HWSURFACE)  /* Use offscreen memory iff SDL_HWSURFACE flag is set */
       {
         /* Hardware surface is Offsceen Context.  ph_ResizeImage handles the switch */
         current->flags = (flags & (~SDL_RESIZABLE)); /* no stretch blit in offscreen context */
       }
       else /* must be SDL_SWSURFACE */
       {
          current->flags = (flags | SDL_RESIZABLE); /* yes we can resize as this is a software surface */
       }

#ifdef HAVE_OPENGL       
       if (flags & SDL_OPENGL) /* for now support OpenGL in window mode only */
       {
          OGLAttrib[0]=PHOGL_ATTRIB_DEPTH_BITS;
          OGLAttrib[1]=bpp;
          OGLAttrib[2]=PHOGL_ATTRIB_NONE;
          OGLContext=PdCreateOpenGLContext(2, &dim, 0, OGLAttrib);
          if (OGLContext==NULL)
          {
             fprintf(stderr,"error: cannot create OpenGL context\n");
             exit(1);
          }
          PhDCSetCurrent(OGLContext);
       }
#endif /* HAVE_OPENGL */
       
    }

	/* If we are setting video to use the palette make sure we have allocated memory for it */
	if(bpp == 8)
	{
		current->format->palette = malloc(sizeof(SDL_Palette));
		memset(current->format->palette, 0, sizeof(SDL_Palette));
		current->format->palette->ncolors = 256;
		current->format->palette->colors = (SDL_Color *)malloc(256 *sizeof(SDL_Color));
		/* fill the palette */
		rtnval = PgGetPalette(ph_palette);

                tempptr = (unsigned long *)current->format->palette->colors;

		for(i=0;i<256; i++)
		{
  			*tempptr = (((unsigned long)ph_palette[i]) << 8);
  			tempptr++;

		}
	}


	/* Current window dimensions */
	PtGetResource( window, Pt_ARG_DIM, &dim, 0 );

	/* If we need to resize the window */
	if((dim.w != width)||(dim.h != height))
	{
	    dim.w=width;
	    dim.h=height; 
	    PtSetArg(&arg[0], Pt_ARG_DIM, &dim,0);
	    PtSetResources( window, 1, arg ); 	
            current->w = width;
            current->h = height;
            current->format->BitsPerPixel = bpp;
            current->format->BytesPerPixel = (bpp+7)/8;
            current->pitch = SDL_CalculatePitch(current);
            /* Must call at least once it setup image planes */
            ph_ResizeImage(this, current, flags);
        }

    SDL_Unlock_EventThread();

    /* We're done! */
    return(current);
}

static void ph_VideoQuit(_THIS)
{
    ph_DestroyImage(this, SDL_VideoSurface); 

    if (currently_fullscreen)
    {
        PdDirectStop( directContext );
        PdReleaseDirectContext( directContext );
        directContext=0;	
        currently_fullscreen = 0;
    }
}


static int ph_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	PgColor_t *in, *out;
	int i, j;
	int alloct_all = 1;

    	colors = this->screen->format->palette->colors;

	in = alloca( ncolors*sizeof(PgColor_t)  );
	if ( in == NULL  ) {
		return 0;
	}
	memset(in,0,ncolors*sizeof(PgColor_t));

    out = alloca( ncolors*sizeof(PgColor_t)  );
    if ( out == NULL  ) {
		return 0;
    }
	
	for (i=0,j=firstcolor;i<ncolors;i++,j++)
	{
		in[i] |= colors[j].r<<16 ;
		in[i] |= colors[j].g<<8 ;
		in[i] |= colors[j].b ; 
	}

	if ( (this->screen->flags & SDL_HWPALETTE) == SDL_HWPALETTE ) 
	{
		if( PgSetPalette( in, 0, 0, ncolors, Pg_PALSET_HARD, 0) < 0 )
		{
			fprintf(stderr,"error: PgSetPalette(..,Pg_PALSET_HARD)  failed\n");
			return 0;
		}  
	}
	else 
	{
		if ( PgColorMatch(ncolors, in, out) < 0 )
        {
            fprintf(stderr,"error: PgColorMatch failed\n");
            return 0;
        }
		for (i=0;i<ncolors;i++)
		{
			if (memcmp(&in[i],&out[i],sizeof(PgColor_t)))
			{
				alloct_all = 0;
				break;
			}
		}
        if( PgSetPalette( out, 0, 0, ncolors, Pg_PALSET_SOFT, 0) < 0 )
        {
            fprintf(stderr,"error: PgSetPalette(..,Pg_PALSET_SOFT) failed\n");
            return 0;
        }
	}
	return alloct_all;
}

#ifdef HAVE_OPENGL
void ph_GL_SwapBuffers(_THIS)
{
   PgSetRegion(PtWidgetRid(window));
   PdOpenGLContextSwapBuffers(OGLContext);
}
#endif // HAVE_OPENGL

/*
static int ph_ResizeWindow(_THIS,
            SDL_Surface *screen, int w, int h, Uint32 flags)
{
	PhWindowEvent_t winevent;

	memset( &winevent, 0, sizeof(winevent) ); 
	winevent.event_f = Ph_WM_RESIZE;
	winevent.size.w = w;
	winevent.size.h = h;
	winevent.rid = PtWidgetRid( window );
	if (PtForwardWindowEvent( &winevent ) < 0)
	{
		fprintf(stderr,"error: PtForwardWindowEvent failed.\n");
	}
	current_w = w;
	current_h = h;
    return(0);
}
*/
