/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

/* SVGAlib based SDL video driver implementation.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#if defined(linux)
#include <linux/vt.h>
#elif defined(__FreeBSD__)
#include <sys/consio.h>
#else
#error You must choose your operating system here
#endif
#include <vga.h>
#include <vgamouse.h>
#include <vgakeyboard.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"
#include "SDL_svgavideo.h"
#include "SDL_svgaevents_c.h"
#include "SDL_svgamouse_c.h"


/* Initialization/Query functions */
static int SVGA_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **SVGA_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *SVGA_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int SVGA_SetColors(_THIS, int firstcolor, int ncolors,
			  SDL_Color *colors);
static void SVGA_VideoQuit(_THIS);

/* Hardware surface functions */
static int SVGA_AllocHWSurface(_THIS, SDL_Surface *surface);
static int SVGA_LockHWSurface(_THIS, SDL_Surface *surface);
static int SVGA_FlipHWSurface(_THIS, SDL_Surface *surface);
static void SVGA_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void SVGA_FreeHWSurface(_THIS, SDL_Surface *surface);

/* SVGAlib driver bootstrap functions */

static int SVGA_Available(void)
{
	/* Check to see if we are root and stdin is a virtual console */
	int console;
	
	/* SVGALib 1.9.x+ doesn't require root (via /dev/svga) */
	int svgalib2 = -1;

	/* See if we are connected to a virtual terminal */
	console = STDIN_FILENO;
#if 0 /* This is no longer needed, SVGAlib can switch consoles for us */
	if ( console >= 0 ) {
		struct stat sb;
		struct vt_mode dummy;

		if ( (fstat(console, &sb) < 0) ||
		     (ioctl(console, VT_GETMODE, &dummy) < 0) ) {
			console = -1;
		}
	}
#endif /* 0 */

	/* See if SVGAlib 2.0 is available */
	svgalib2 = open("/dev/svga", O_RDONLY);
	if (svgalib2 != -1) {
		close(svgalib2);
	}

	return(((svgalib2 != -1) || (geteuid() == 0)) && (console >= 0));
}

static void SVGA_DeleteDevice(SDL_VideoDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_VideoDevice *SVGA_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			free(device);
		}
		return(0);
	}
	memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = SVGA_VideoInit;
	device->ListModes = SVGA_ListModes;
	device->SetVideoMode = SVGA_SetVideoMode;
	device->SetColors = SVGA_SetColors;
	device->UpdateRects = NULL;
	device->VideoQuit = SVGA_VideoQuit;
	device->AllocHWSurface = SVGA_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = SVGA_LockHWSurface;
	device->UnlockHWSurface = SVGA_UnlockHWSurface;
	device->FlipHWSurface = SVGA_FlipHWSurface;
	device->FreeHWSurface = SVGA_FreeHWSurface;
	device->SetCaption = NULL;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GrabInput = NULL;
	device->GetWMInfo = NULL;
	device->InitOSKeymap = SVGA_InitOSKeymap;
	device->PumpEvents = SVGA_PumpEvents;

	device->free = SVGA_DeleteDevice;

	return device;
}

VideoBootStrap SVGALIB_bootstrap = {
	"svgalib", "SVGAlib",
	SVGA_Available, SVGA_CreateDevice
};

static int SVGA_AddMode(_THIS, int mode, int actually_add, int force)
{
	vga_modeinfo *modeinfo;

	modeinfo = vga_getmodeinfo(mode);
	if ( force || ( modeinfo->flags & CAPABLE_LINEAR ) ) {
		int i, j;

		i = modeinfo->bytesperpixel-1;
		if ( actually_add ) {
			SDL_Rect saved_rect[2];
			int      saved_mode[2];
			int b;

			/* Add the mode, sorted largest to smallest */
			b = 0;
			j = 0;
			while ( (SDL_modelist[i][j]->w > modeinfo->width) ||
			        (SDL_modelist[i][j]->h > modeinfo->height) ) {
				++j;
			}
			/* Skip modes that are already in our list */
			if ( (SDL_modelist[i][j]->w == modeinfo->width) &&
			     (SDL_modelist[i][j]->h == modeinfo->height) ) {
				return(0);
			}
			/* Insert the new mode */
			saved_rect[b] = *SDL_modelist[i][j];
			saved_mode[b] = SDL_vgamode[i][j];
			SDL_modelist[i][j]->w = modeinfo->width;
			SDL_modelist[i][j]->h = modeinfo->height;
			SDL_vgamode[i][j] = mode;
			/* Everybody scoot down! */
			if ( saved_rect[b].w && saved_rect[b].h ) {
			    for ( ++j; SDL_modelist[i][j]->w; ++j ) {
				saved_rect[!b] = *SDL_modelist[i][j];
				saved_mode[!b] = SDL_vgamode[i][j];
				*SDL_modelist[i][j] = saved_rect[b];
				SDL_vgamode[i][j] = saved_mode[b];
				b = !b;
			    }
			    *SDL_modelist[i][j] = saved_rect[b];
			    SDL_vgamode[i][j] = saved_mode[b];
			}
		} else {
			++SDL_nummodes[i];
		}
	}
	return( force || ( modeinfo->flags & CAPABLE_LINEAR ) );
}

static void SVGA_UpdateVideoInfo(_THIS)
{
	vga_modeinfo *modeinfo;

	this->info.wm_available = 0;
	this->info.hw_available = 1;
	modeinfo = vga_getmodeinfo(vga_getcurrentmode());
	this->info.video_mem = modeinfo->memory;
	/* FIXME: Add hardware accelerated blit information */
#ifdef SVGALIB_DEBUG
	printf("Hardware accelerated blit: %savailable\n", modeinfo->haveblit ? "" : "not ");
#endif
}

int SVGA_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int keyboard;
	int i, j;
	int mode, total_modes;

	/* Initialize all variables that we clean on shutdown */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		SDL_nummodes[i] = 0;
		SDL_modelist[i] = NULL;
		SDL_vgamode[i] = NULL;
	}

	/* Initialize the library */
	vga_disabledriverreport();
	if ( vga_init() < 0 ) {
		SDL_SetError("Unable to initialize SVGAlib");
		return(-1);
	}
	vga_setmode(TEXT);

	/* Enable mouse and keyboard support */
	vga_setmousesupport(1);
	keyboard = keyboard_init_return_fd();
	if ( keyboard < 0 ) {
		SDL_SetError("Unable to initialize keyboard");
		return(-1);
	}
	if ( SVGA_initkeymaps(keyboard) < 0 ) {
		return(-1);
	}
	keyboard_seteventhandler(SVGA_keyboardcallback);

	/* Determine the screen depth (use default 8-bit depth) */
	vformat->BitsPerPixel = 8;

	/* Enumerate the available fullscreen modes */
	total_modes = 0;
	for ( mode=vga_lastmodenumber(); mode; --mode ) {
		if ( vga_hasmode(mode) ) {
			if ( SVGA_AddMode(this, mode, 0, 0) ) {
				++total_modes;
			}
		}
	}
	if ( SVGA_AddMode(this, G320x200x256, 0, 1) ) ++total_modes;
	if ( total_modes == 0 ) {
		SDL_SetError("No linear video modes available");
		return(-1);
	}
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		SDL_vgamode[i] = (int *)malloc(SDL_nummodes[i]*sizeof(int));
		if ( SDL_vgamode[i] == NULL ) {
			SDL_OutOfMemory();
			return(-1);
		}
		SDL_modelist[i] = (SDL_Rect **)
				malloc((SDL_nummodes[i]+1)*sizeof(SDL_Rect *));
		if ( SDL_modelist[i] == NULL ) {
			SDL_OutOfMemory();
			return(-1);
		}
		for ( j=0; j<SDL_nummodes[i]; ++j ) {
			SDL_modelist[i][j]=(SDL_Rect *)malloc(sizeof(SDL_Rect));
			if ( SDL_modelist[i][j] == NULL ) {
				SDL_OutOfMemory();
				return(-1);
			}
			memset(SDL_modelist[i][j], 0, sizeof(SDL_Rect));
		}
		SDL_modelist[i][j] = NULL;
	}
	for ( mode=vga_lastmodenumber(); mode; --mode ) {
		if ( vga_hasmode(mode) ) {
			SVGA_AddMode(this, mode, 1, 0);
		}
	}
	SVGA_AddMode(this, G320x200x256, 1, 1);

	/* Free extra (duplicated) modes */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		j = 0;
		while ( SDL_modelist[i][j] && SDL_modelist[i][j]->w ) {
			j++;
		}
		while ( SDL_modelist[i][j] ) {
			free(SDL_modelist[i][j]);
			SDL_modelist[i][j] = NULL;
			j++;
		}
	}

	/* Fill in our hardware acceleration capabilities */
	SVGA_UpdateVideoInfo(this);

	/* We're done! */
	return(0);
}

SDL_Rect **SVGA_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	return(SDL_modelist[((format->BitsPerPixel+7)/8)-1]);
}

/* Various screen update functions available */
static void SVGA_DirectUpdate(_THIS, int numrects, SDL_Rect *rects);
static void SVGA_BankedUpdate(_THIS, int numrects, SDL_Rect *rects);

SDL_Surface *SVGA_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	int mode;
	int vgamode;
	vga_modeinfo *modeinfo;
	int screenpage_len;

	/* Try to set the requested linear video mode */
	bpp = (bpp+7)/8-1;
	for ( mode=0; SDL_modelist[bpp][mode]; ++mode ) {
		if ( (SDL_modelist[bpp][mode]->w == width) &&
		     (SDL_modelist[bpp][mode]->h == height) ) {
			break;
		}
	}
	if ( SDL_modelist[bpp][mode] == NULL ) {
		SDL_SetError("Couldn't find requested mode in list");
		return(NULL);
	}
	vga_setmode(SDL_vgamode[bpp][mode]);
	vga_setpage(0);

	vgamode=SDL_vgamode[bpp][mode];
	if ((vga_setlinearaddressing()<0) && (vgamode!=G320x200x256)) {
		SDL_SetError("Unable to set linear addressing");
		return(NULL);
	}
	modeinfo = vga_getmodeinfo(SDL_vgamode[bpp][mode]);

	/* Update hardware acceleration info */
	SVGA_UpdateVideoInfo(this);

	/* Allocate the new pixel format for the screen */
	bpp = (bpp+1)*8;
	if ( (bpp == 16) && (modeinfo->colors == 32768) ) {
		bpp = 15;
	}
	if ( ! SDL_ReallocFormat(current, bpp, 0, 0, 0, 0) ) {
		return(NULL);
	}

	/* Set up the new mode framebuffer */
	current->flags = (SDL_FULLSCREEN|SDL_HWSURFACE);
	if ( bpp == 8 ) {
		/* FIXME: What about DirectColor? */
		current->flags |= SDL_HWPALETTE;
	}
	current->w = width;
	current->h = height;
	current->pitch = modeinfo->linewidth;
	current->pixels = vga_getgraphmem();

	/* set double-buffering */
	if ( flags & SDL_DOUBLEBUF )
	{
	    /* length of one screen page in bytes */
	    screenpage_len=current->h*modeinfo->linewidth;

	    /* if start address should be aligned */
	    if ( modeinfo->linewidth_unit )
	    {
		if ( screenpage_len % modeinfo->linewidth_unit )    
		{
		    screenpage_len += modeinfo->linewidth_unit - ( screenpage_len % modeinfo->linewidth_unit );
		}
	    }

	    /* if we heve enough videomemory =  ak je dost videopamete  */
	    if ( modeinfo->memory > ( screenpage_len * 2 / 1024 ) )
	    {
		current->flags |= SDL_DOUBLEBUF;
		flip_page = 0;
		flip_offset[0] = 0;
		flip_offset[1] = screenpage_len;
		flip_address[0] = vga_getgraphmem();
		flip_address[1] = flip_address[0]+screenpage_len;
		SVGA_FlipHWSurface(this,current);
	    }
	} 

	/* Set the blit function */
	this->UpdateRects = SVGA_DirectUpdate;

	/* Set up the mouse handler again (buggy SVGAlib 1.40) */
	mouse_seteventhandler(SVGA_mousecallback);

	/* We're done */
	return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int SVGA_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}
static void SVGA_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int SVGA_LockHWSurface(_THIS, SDL_Surface *surface)
{
	/* The waiting is done in SVGA_FlipHWSurface() */
	return(0);
}
static void SVGA_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static int SVGA_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	vga_setdisplaystart(flip_offset[flip_page]);
	flip_page=!flip_page;
	surface->pixels=flip_address[flip_page];
	vga_waitretrace();
	return(0);
}

static void SVGA_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	return;
}

/* FIXME: Can this be used under SVGAlib? */
static void SVGA_BankedUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	return;
}

int SVGA_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
        int i;

	for(i = 0; i < ncolors; i++) {
	        vga_setpalette(firstcolor + i,
			       colors[i].r>>2,
			       colors[i].g>>2,
			       colors[i].b>>2);
	}
	return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void SVGA_VideoQuit(_THIS)
{
	int i, j;

	/* Reset the console video mode */
	if ( this->screen && (this->screen->w && this->screen->h) ) {
		vga_setmode(TEXT);
	}
	keyboard_close();

	/* Free video mode lists */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		if ( SDL_modelist[i] != NULL ) {
			for ( j=0; SDL_modelist[i][j]; ++j )
				free(SDL_modelist[i][j]);
			free(SDL_modelist[i]);
			SDL_modelist[i] = NULL;
		}
		if ( SDL_vgamode[i] != NULL ) {
			free(SDL_vgamode[i]);
			SDL_vgamode[i] = NULL;
		}
	}
	if ( this->screen && (this->screen->flags & SDL_HWSURFACE) ) {
		/* Direct screen access, no memory buffer */
		this->screen->pixels = NULL;
	}
}

