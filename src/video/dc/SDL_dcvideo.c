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

    BERO
    bero@geocities.co.jp

    based on SDL_nullvideo.c by

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Dummy SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@linuxgames.com). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "DC" by Sam Lantinga.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"

#include "SDL_dcvideo.h"
#include "SDL_dcevents_c.h"
#include "SDL_dcmouse_c.h"

#include <dc/video.h>
#include <dc/pvr.h>

#ifdef HAVE_OPENGL
#include <GL/gl.h>
#endif


/* Initialization/Query functions */
static int DC_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **DC_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *DC_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int DC_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void DC_VideoQuit(_THIS);

/* Hardware surface functions */
static int DC_AllocHWSurface(_THIS, SDL_Surface *surface);
static int DC_LockHWSurface(_THIS, SDL_Surface *surface);
static void DC_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void DC_FreeHWSurface(_THIS, SDL_Surface *surface);
static int DC_FlipHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void DC_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

/* OpenGL */
static void *DC_GL_GetProcAddress(_THIS, const char *proc);
static int DC_GL_LoadLibrary(_THIS, const char *path);
static int DC_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
static void DC_GL_SwapBuffers(_THIS);

/* DC driver bootstrap functions */

static int DC_Available(void)
{
	return 1;
}

static void DC_DeleteDevice(SDL_VideoDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_VideoDevice *DC_CreateDevice(int devindex)
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
	device->VideoInit = DC_VideoInit;
	device->ListModes = DC_ListModes;
	device->SetVideoMode = DC_SetVideoMode;
	device->CreateYUVOverlay = NULL;
	device->SetColors = DC_SetColors;
	device->UpdateRects = DC_UpdateRects;
	device->VideoQuit = DC_VideoQuit;
	device->AllocHWSurface = DC_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = DC_LockHWSurface;
	device->UnlockHWSurface = DC_UnlockHWSurface;
	device->FlipHWSurface = DC_FlipHWSurface;
	device->FreeHWSurface = DC_FreeHWSurface;
#ifdef	HAVE_OPENGL
	device->GL_LoadLibrary = DC_GL_LoadLibrary;
	device->GL_GetProcAddress = DC_GL_GetProcAddress;
	device->GL_GetAttribute = DC_GL_GetAttribute;
	device->GL_MakeCurrent = NULL;
	device->GL_SwapBuffers = DC_GL_SwapBuffers;
#endif
	device->SetCaption = NULL;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GrabInput = NULL;
	device->GetWMInfo = NULL;
	device->InitOSKeymap = DC_InitOSKeymap;
	device->PumpEvents = DC_PumpEvents;

	device->free = DC_DeleteDevice;

	return device;
}

VideoBootStrap DC_bootstrap = {
	"dcvideo", "Dreamcast Video",
	DC_Available, DC_CreateDevice
};


int DC_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	/* Determine the screen depth (use default 8-bit depth) */
	/* we change this during the SDL_SetVideoMode implementation... */
	vformat->BitsPerPixel = 16;
	vformat->Rmask = 0x0000f800;
	vformat->Gmask = 0x000007e0;
	vformat->Bmask = 0x0000001f;

	/* We're done! */
	return(0);
}

const static SDL_Rect
	RECT_800x600 = {0,0,800,600},
	RECT_640x480 = {0,0,640,480},
	RECT_320x240 = {0,0,320,240};
const static SDL_Rect *vid_modes[] = {
	&RECT_800x600,
	&RECT_640x480,
	&RECT_320x240,
	NULL
};

SDL_Rect **DC_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	switch(format->BitsPerPixel) {
	case 15:
	case 16:
		return &vid_modes;
	case 32:
		if (!(flags & SDL_OPENGL))
		return &vid_modes;
	default:
		return NULL;
	}
//	return (SDL_Rect **) -1;
}

pvr_init_params_t params = {
        /* Enable opaque and translucent polygons with size 16 */
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16 },

        /* Vertex buffer size */
        512*1024
};

#ifdef HAVE_OPENGL
static int pvr_inited;
#endif

SDL_Surface *DC_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	int disp_mode,pixel_mode,pitch;
	Uint32 Rmask, Gmask, Bmask;

	if (width==320 && height==240) disp_mode=DM_320x240;
	else if (width==640 && height==480) disp_mode=DM_640x480;
	else if (width==800 && height==600) disp_mode=DM_800x608;
	else {
		SDL_SetError("Couldn't find requested mode in list");
		return(NULL);
	}

	switch(bpp) {
	case 15: pixel_mode = PM_RGB555; pitch = width*2;
		/* 5-5-5 */
		Rmask = 0x00007c00;
		Gmask = 0x000003e0;
		Bmask = 0x0000001f;
		break;
	case 16: pixel_mode = PM_RGB565; pitch = width*2;
		/* 5-6-5 */
		Rmask = 0x0000f800;
		Gmask = 0x000007e0;
		Bmask = 0x0000001f;
		break;
	case 24: bpp = 32;
	case 32: pixel_mode = PM_RGB888; pitch = width*4;
		Rmask = 0x00ff0000;
		Gmask = 0x0000ff00;
		Bmask = 0x000000ff;
#ifdef	HAVE_OPENGL
		if (!(flags & SDL_OPENGL))
#endif
		break;
	default:
		SDL_SetError("Couldn't find requested mode in list");
		return(NULL);
	}

//  if ( bpp != current->format->BitsPerPixel ) {
	if ( ! SDL_ReallocFormat(current, bpp, Rmask, Gmask, Bmask, 0) ) {
		return(NULL);
	}
//  }

	/* Set up the new mode framebuffer */
	current->flags = (SDL_FULLSCREEN|SDL_HWSURFACE);
	current->w = width;
	current->h = height;
	current->pitch = pitch;

#ifdef HAVE_OPENGL
	if (pvr_inited) {
		pvr_inited = 0;
		pvr_shutdown();
	}
#endif

	vid_set_mode(disp_mode,pixel_mode);

	current->pixels = vram_s;

#ifdef	HAVE_OPENGL
	if (flags & SDL_OPENGL) {
		this->gl_config.driver_loaded = 1;
		current->flags = SDL_FULLSCREEN | SDL_OPENGL;
		current->pixels = NULL;
		pvr_inited = 1;
		pvr_init(&params);
		glKosInit();
		glKosBeginFrame();
	} else
#endif
	if (flags | SDL_DOUBLEBUF) {
		current->flags |= SDL_DOUBLEBUF;
		current->pixels = (void*)((int)current->pixels | 0x400000);
	}

	/* We're done */
	return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int DC_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}
static void DC_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int DC_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void DC_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static int DC_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	if (surface->flags & SDL_DOUBLEBUF) {
		vid_set_start((int)surface->pixels & 0xffffff);
		surface->pixels = (void*)((int)surface->pixels ^ 0x400000);
	}
	return(0);
}

static void DC_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	/* do nothing. */
}

static int DC_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	/* do nothing of note. */
	return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
static void DC_VideoQuit(_THIS)
{
#ifdef HAVE_OPENGL
	if (pvr_inited) {
		pvr_inited = 0;
		pvr_shutdown();
	}
#endif
}

#ifdef HAVE_OPENGL

void dmyfunc(void) {}

typedef void (*funcptr)();
const static struct {
	char *name;
	funcptr addr;
} glfuncs[] = {
#define	DEF(func)	{#func,&func}
	DEF(glBegin),
	DEF(glBindTexture),
	DEF(glBlendFunc),
	DEF(glColor4f),
//	DEF(glCopyImageID),
	DEF(glDisable),
	DEF(glEnable),
	DEF(glEnd),
	DEF(glFlush),
	DEF(glGenTextures),
	DEF(glGetString),
	DEF(glLoadIdentity),
	DEF(glMatrixMode),
	DEF(glOrtho),
	DEF(glPixelStorei),
//	DEF(glPopAttrib),
//	DEF(glPopClientAttrib),
	{"glPopAttrib",&dmyfunc},
	{"glPopClientAttrib",&dmyfunc},
	DEF(glPopMatrix),
//	DEF(glPushAttrib),
//	DEF(glPushClientAttrib),
	{"glPushAttrib",&dmyfunc},
	{"glPushClientAttrib",&dmyfunc},
	DEF(glPushMatrix),
	DEF(glTexCoord2f),
	DEF(glTexEnvf),
	DEF(glTexImage2D),
	DEF(glTexParameteri),
	DEF(glTexSubImage2D),
	DEF(glVertex2i),
	DEF(glViewport),
#undef	DEF
};

static void *DC_GL_GetProcAddress(_THIS, const char *proc)
{
	void *ret;
	int i;

	ret = glKosGetProcAddress(proc);
	if (ret) return ret;

	for(i=0;i<sizeof(glfuncs)/sizeof(glfuncs[0]);i++) {
		if (strcmp(proc,glfuncs[i].name)==0) return glfuncs[i].addr;
	}

	return NULL;
}

static int DC_GL_LoadLibrary(_THIS, const char *path)
{
	this->gl_config.driver_loaded = 1;

	return 0;
}

static int DC_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value)
{
	GLenum mesa_attrib;
	int val;

	switch(attrib) {
	case SDL_GL_RED_SIZE:
		val = 5;
		break;
	case SDL_GL_GREEN_SIZE:
		val = 6;
		break;
	case SDL_GL_BLUE_SIZE:
		val = 5;
		break;
	case SDL_GL_ALPHA_SIZE:
		val = 0;
		break;
	case SDL_GL_DOUBLEBUFFER:
		val = 1;
		break;
	case SDL_GL_DEPTH_SIZE:
		val = 16; /* or 32? */
		break;
	case SDL_GL_STENCIL_SIZE:
		val = 0;
		break;
	case SDL_GL_ACCUM_RED_SIZE:
		val = 0;
		break;
	case SDL_GL_ACCUM_GREEN_SIZE:
		val = 0;
	case SDL_GL_ACCUM_BLUE_SIZE:
		val = 0;
		break;
	case SDL_GL_ACCUM_ALPHA_SIZE:
		val = 0;
		break;
	default :
		return -1;
	}
	*value = val;
	return 0;
}

static void DC_GL_SwapBuffers(_THIS)
{
	glKosFinishFrame();
	glKosBeginFrame();
}
#endif
