/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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
#include "SDL_config.h"

/*
     File added by Alan Buckley (alan_baa@hotmail.com) for RISC OS compatability
	 23 March 2003

     Implements RISC OS display device management.
	 Routines for full screen and wimp modes are split
	 into other source files.
*/

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_syswm.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_riscostask.h"
#include "SDL_riscosvideo.h"
#include "SDL_riscosevents_c.h"
#include "SDL_riscosmouse_c.h"

#include "kernel.h"
#include "swis.h"

#define RISCOSVID_DRIVER_NAME "riscos"

/* Initialization/Query functions */
static int RISCOS_VideoInit(_THIS, SDL_PixelFormat *vformat);
static void RISCOS_VideoQuit(_THIS);

static SDL_Rect **RISCOS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *RISCOS_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);

/* Hardware surface functions - common to WIMP and FULLSCREEN */
static int RISCOS_AllocHWSurface(_THIS, SDL_Surface *surface);
static int RISCOS_LockHWSurface(_THIS, SDL_Surface *surface);
static void RISCOS_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void RISCOS_FreeHWSurface(_THIS, SDL_Surface *surface);

#define MODE_350(type, xdpi, ydpi) \
	(1 | (xdpi << 1) | (ydpi << 14) | (type << 27))
#define MODE_521(type, xeig, yeig, flags) \
	(0x78000001 | (xeig << 4) | (yeig << 6) | (flags & 0xFF00) | (type << 20))

/* Table of known pixel formats */
const RISCOS_SDL_PixelFormat *RISCOS_SDL_PixelFormats = (const RISCOS_SDL_PixelFormat[]) {
/* 8bpp palettized */
{ { 255,   0x0080, 3,                      28 }, 8, 0, 0, 0 },
/* 12bpp true colour */
{ { 4095,  0x0000, 4, MODE_521(16,1,1,0)      }, 12, 0x0f,     0x0f<<4, 0x0f<<8  },
{ { 4095,  0x4000, 4, MODE_521(16,1,1,0x4000) }, 12, 0x0f<<8,  0x0f<<4, 0x0f     },
/* 15bpp true colour */
{ { 65535, 0x0000, 4, MODE_350(5,90,90)       }, 15, 0x1f,     0x1f<<5, 0x1f<<10 },
{ { 65535, 0x4000, 4, MODE_521(5,1,1,0x4000)  }, 15, 0x1f<<10, 0x1f<<5, 0x1f     },
/* 16bpp true colour */
{ { 65535, 0x0080, 4, MODE_350(10,90,90)      }, 16, 0x1f,     0x3f<<5, 0x1f<<11 },
{ { 65535, 0x4080, 4, MODE_521(10,1,1,0x4000) }, 16, 0x1f<<11, 0x3f<<5, 0x1f     },
/* 32bpp true colour */
{ { -1,    0x0000, 5, MODE_350(6,90,90)       }, 32, 0xff,     0xff<<8, 0xff<<16 },
{ { -1,    0x4000, 5, MODE_521(6,1,1,0x4000)  }, 32, 0xff<<16, 0xff<<8, 0xff     },
/* Terminator */
{ },
};

const RISCOS_SDL_PixelFormat *RISCOS_CurrentPixelFormat()
{
	_kernel_swi_regs regs;
	int vduvars[4];
	const RISCOS_SDL_PixelFormat *fmt;

	vduvars[0] = 3; /* NColour */
	vduvars[1] = 0; /* ModeFlags */
	vduvars[2] = 9; /* Log2BPP */
	vduvars[3] = -1;

	regs.r[0] = (int) vduvars;
	regs.r[1] = (int) vduvars;

	_kernel_swi(OS_ReadVduVariables, &regs, &regs);

	vduvars[1] &= 0xf280; /* Mask out the bits we don't care about */

	fmt = RISCOS_SDL_PixelFormats;
	while (fmt->sdl_bpp)
	{
		if ((fmt->ro.ncolour == vduvars[0]) && (fmt->ro.modeflags == vduvars[1]) && (fmt->ro.log2bpp == vduvars[2]))
		{
			return fmt;
		}
		fmt++;
	}

	return NULL;
}

/* RISC OS driver bootstrap functions */

static int RISCOS_Available(void)
{
	return(1);
}

static void RISCOS_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *RISCOS_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = RISCOS_VideoInit;
	device->VideoQuit = RISCOS_VideoQuit;

	device->ListModes = RISCOS_ListModes;
	device->SetVideoMode = RISCOS_SetVideoMode;
	device->CreateYUVOverlay = NULL;
	device->AllocHWSurface = RISCOS_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = RISCOS_LockHWSurface;
	device->UnlockHWSurface = RISCOS_UnlockHWSurface;
	device->FreeHWSurface = RISCOS_FreeHWSurface;
	
	device->FreeWMCursor = RISCOS_FreeWMCursor;
	device->CreateWMCursor = RISCOS_CreateWMCursor;
	device->CheckMouseMode = RISCOS_CheckMouseMode;
    device->GrabInput = RISCOS_GrabInput;

	device->InitOSKeymap = RISCOS_InitOSKeymap;

	device->GetWMInfo = RISCOS_GetWmInfo;

	device->free = RISCOS_DeleteDevice;

/* Can't get Toggle screen to work if program starts up in Full screen mode so
   disable it here and re-enable it when a wimp screen is chosen */
    device->ToggleFullScreen = NULL; /*RISCOS_ToggleFullScreen;*/

	/* Set other entries for fullscreen mode */
	FULLSCREEN_SetDeviceMode(device);

	/* Mouse pointer needs to use the WIMP ShowCursor version so
	   that it doesn't modify the pointer until the SDL Window is
	   entered or the application goes full screen */
	device->ShowWMCursor = WIMP_ShowWMCursor;

	return device;
}

VideoBootStrap RISCOS_bootstrap = {
	RISCOSVID_DRIVER_NAME, "RISC OS video driver",
	RISCOS_Available, RISCOS_CreateDevice
};


int RISCOS_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	_kernel_swi_regs regs;
	int vars[3];
	const RISCOS_SDL_PixelFormat *fmt;
	SDL_PixelFormat *fmt2 = NULL;

	if (RISCOS_InitTask() == 0)
	{
		return 0;
	}

	vars[0] = 11; /* XWndLimit - num x pixels -1 */
	vars[1] = 12; /* YWndLimit - num y pixels -1 */
	vars[2] = -1; /* Terminate list */
	regs.r[0] = (int)vars;
	regs.r[1] = (int)vars;

	_kernel_swi(OS_ReadVduVariables, &regs, &regs);

	/* Determine the current screen size */
	this->info.current_w = vars[0] + 1;
	this->info.current_h = vars[1] + 1;

	fmt = RISCOS_CurrentPixelFormat();
	if (fmt != NULL)
	{
		fmt2 = SDL_AllocFormat(fmt->sdl_bpp,fmt->rmask,fmt->gmask,fmt->bmask,0);
	}
	if (fmt2 != NULL)
	{
		*vformat = *fmt2;
		SDL_free(fmt2);
	}
	else
	{
		/* Panic! */
		vformat->BitsPerPixel = 8;
		vformat->Bmask = 0;
		vformat->Gmask = 0;
		vformat->Rmask = 0;
		vformat->BytesPerPixel = 1;
	}

	/* Fill in some window manager capabilities */
	this->info.wm_available = 1;

	/* We're done! */
	return(0);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void RISCOS_VideoQuit(_THIS)
{
	RISCOS_ExitTask();

	if (this->hidden->alloc_bank) SDL_free(this->hidden->alloc_bank);
	this->hidden->alloc_bank = 0;
}


SDL_Rect **RISCOS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	if (flags & SDL_FULLSCREEN)
	{
		/* Build mode list when first required. */
		if (SDL_nummodes[0] == 0) FULLSCREEN_BuildModeList(this);

		return(SDL_modelist[((format->BitsPerPixel+7)/8)-1]);
	} else
		return (SDL_Rect **)-1;
}


/* Set up video mode */
SDL_Surface *RISCOS_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	if (flags & SDL_FULLSCREEN)
	{
	    RISCOS_StoreWimpMode();
		/* Dump wimp window on switch to full screen */
  	    if (this->hidden->window_handle) WIMP_DeleteWindow(this);

		return FULLSCREEN_SetVideoMode(this, current, width, height, bpp, flags);
	} else
	{
	    RISCOS_RestoreWimpMode();
		return WIMP_SetVideoMode(this, current, width, height, bpp, flags);
	}
}


/* We don't actually allow hardware surfaces other than the main one */
static int RISCOS_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}
static void RISCOS_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int RISCOS_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void RISCOS_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}


int RISCOS_GetWmInfo(_THIS, SDL_SysWMinfo *info)
{
	SDL_VERSION(&(info->version));
	info->wimpVersion = RISCOS_GetWimpVersion();
	info->taskHandle = RISCOS_GetTaskHandle();
	info->window = this->hidden->window_handle;

	return 1;
}
/* Toggle full screen mode.
   Returns 1 if successful otherwise 0
*/

int RISCOS_ToggleFullScreen(_THIS, int fullscreen)
{
    if (fullscreen)
    {
       return FULLSCREEN_ToggleFromWimp(this);
    } else
    {
       return WIMP_ToggleFromFullScreen(this);
    }

   return 0;
}

