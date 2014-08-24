/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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

/*
 * Xbios SDL video driver
 * 
 * Patrice Mandin
 */

#include <sys/stat.h>
#include <unistd.h>

/* Mint includes */
#include <mint/cookie.h>
#include <mint/osbind.h>
#include <mint/falcon.h>

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "../ataricommon/SDL_ataric2p_s.h"
#include "../ataricommon/SDL_atarievents_c.h"
#include "../ataricommon/SDL_atarimxalloc_c.h"
#include "../ataricommon/SDL_atarigl_c.h"
#include "SDL_xbios.h"
#include "SDL_xbios_sb3.h"
#include "SDL_xbios_tveille.h"
#include "SDL_xbios_milan.h"

#define XBIOS_VID_DRIVER_NAME "xbios"

#ifndef C_fVDI
#define C_fVDI 0x66564449L
#endif

/* Debug print info */
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#define DEBUG_VIDEO_XBIOS 1
#else
#define DEBUG_PRINT(what)
#undef DEBUG_VIDEO_XBIOS
#endif

/* Initialization/Query functions */
static int XBIOS_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **XBIOS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *XBIOS_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static void XBIOS_VideoQuit(_THIS);

/* Hardware surface functions */
static int XBIOS_AllocHWSurface(_THIS, SDL_Surface *surface);
static int XBIOS_LockHWSurface(_THIS, SDL_Surface *surface);
static int XBIOS_FlipHWSurface(_THIS, SDL_Surface *surface);
static void XBIOS_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void XBIOS_FreeHWSurface(_THIS, SDL_Surface *surface);
static void XBIOS_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

#if SDL_VIDEO_OPENGL
/* OpenGL functions */
static void XBIOS_GL_SwapBuffers(_THIS);
#endif

/* Xbios driver bootstrap functions */

static int XBIOS_Available(void)
{
	long cookie_vdo, /*cookie_mil,*/ cookie_hade, cookie_scpn;
	long cookie_fvdi;
	const char *envr = SDL_getenv("SDL_VIDEODRIVER");

	/* Milan/Hades Atari clones do not have an Atari video chip */
	if ( /*(Getcookie(C__MIL, &cookie_mil) == C_FOUND) ||*/
		(Getcookie(C_hade, &cookie_hade) == C_FOUND) ) {
		return 0;
	}

	/* fVDI means graphic card, so no Xbios with it */
	if (Getcookie(C_fVDI, &cookie_fvdi) == C_FOUND) {
		if (!envr) {
			return 0;
		}
		if (SDL_strcmp(envr, XBIOS_VID_DRIVER_NAME)!=0) {
			return 0;
		}
		/* Except if we force Xbios usage, through env var */
	}

	/* Cookie _VDO present ? if not, assume ST machine */
	if (Getcookie(C__VDO, &cookie_vdo) != C_FOUND) {
		cookie_vdo = VDO_ST << 16;
	}

	/* Test if we have a monochrome monitor plugged in */
	switch( cookie_vdo >>16) {
		case VDO_ST:
		case VDO_STE:
			if ( Getrez() == (ST_HIGH>>8) )
				return 0;
			break;
		case VDO_TT:
			if ( (EgetShift() & ES_MODE) == TT_HIGH)
				return 0;
			break;
		case VDO_F30:
			if ( VgetMonitor() == MONITOR_MONO)
				return 0;
			if (Getcookie(C_SCPN, &cookie_scpn) == C_FOUND) {
				if (!SDL_XBIOS_SB3Usable((scpn_cookie_t *)cookie_scpn)) {
					return 0;
				}
			}
			break;
		case VDO_MILAN:
			break;
		default:
			return 0;
	}

	return 1;
}

static void XBIOS_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *XBIOS_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;
	long cookie_cvdo;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
		device->gl_data = (struct SDL_PrivateGLData *)
				SDL_malloc((sizeof *device->gl_data));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));
	SDL_memset(device->gl_data, 0, sizeof(*device->gl_data));

	/* Video functions */
	device->VideoInit = XBIOS_VideoInit;
	device->ListModes = XBIOS_ListModes;
	device->SetVideoMode = XBIOS_SetVideoMode;
	device->SetColors = NULL;	/* Defined by each device specific backend */
	device->UpdateRects = NULL;	/* Defined once video mode set */
	device->VideoQuit = XBIOS_VideoQuit;
	device->AllocHWSurface = XBIOS_AllocHWSurface;
	device->LockHWSurface = XBIOS_LockHWSurface;
	device->UnlockHWSurface = XBIOS_UnlockHWSurface;
	device->FlipHWSurface = XBIOS_FlipHWSurface;
	device->FreeHWSurface = XBIOS_FreeHWSurface;

#if SDL_VIDEO_OPENGL
	/* OpenGL functions */
	device->GL_LoadLibrary = SDL_AtariGL_LoadLibrary;
	device->GL_GetProcAddress = SDL_AtariGL_GetProcAddress;
	device->GL_GetAttribute = SDL_AtariGL_GetAttribute;
	device->GL_MakeCurrent = SDL_AtariGL_MakeCurrent;
	device->GL_SwapBuffers = XBIOS_GL_SwapBuffers;
#endif

	/* Events */
	device->InitOSKeymap = Atari_InitOSKeymap;
	device->PumpEvents = Atari_PumpEvents;

	device->free = XBIOS_DeleteDevice;

	/* Setup device specific functions, default to ST for everything */
	if (Getcookie(C__VDO, &cookie_cvdo) != C_FOUND) {
		cookie_cvdo = VDO_ST << 16;
	}
	SDL_XBIOS_VideoInit_ST(device, cookie_cvdo);
	device->hidden->updRects = XBIOS_UpdateRects;

	switch (cookie_cvdo>>16) {
		case VDO_ST:
		case VDO_STE:
			/* Already done as default */
			break;
		case VDO_TT:
			SDL_XBIOS_VideoInit_TT(device);
			break;
		case VDO_F30:
			SDL_XBIOS_VideoInit_F30(device);
			break;
		case VDO_MILAN:
			SDL_XBIOS_VideoInit_Milan(device);
			break;
	}

	return device;
}

VideoBootStrap XBIOS_bootstrap = {
	XBIOS_VID_DRIVER_NAME, "Atari Xbios driver",
	XBIOS_Available, XBIOS_CreateDevice
};

void SDL_XBIOS_AddMode(_THIS, int actually_add, const xbiosmode_t *modeinfo)
{
	int i = 0;

	switch(modeinfo->depth) {
		case 15:
		case 16:
			i = 1;
			break;
		case 24:
			i = 2;
			break;
		case 32:
			i = 3;
			break;
	}

	if ( actually_add ) {
		SDL_Rect saved_rect[2];
		xbiosmode_t saved_mode[2];
		int b, j;

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
			return;
		}
		/* Insert the new mode */
		saved_rect[b] = *SDL_modelist[i][j];
		SDL_memcpy(&saved_mode[b], SDL_xbiosmode[i][j], sizeof(xbiosmode_t));
		SDL_modelist[i][j]->w = modeinfo->width;
		SDL_modelist[i][j]->h = modeinfo->height;
		SDL_memcpy(SDL_xbiosmode[i][j], modeinfo, sizeof(xbiosmode_t));
		/* Everybody scoot down! */
		if ( saved_rect[b].w && saved_rect[b].h ) {
		    for ( ++j; SDL_modelist[i][j]->w; ++j ) {
			saved_rect[!b] = *SDL_modelist[i][j];
			SDL_memcpy(&saved_mode[!b], SDL_xbiosmode[i][j], sizeof(xbiosmode_t));
			*SDL_modelist[i][j] = saved_rect[b];
			SDL_memcpy(SDL_xbiosmode[i][j], &saved_mode[b], sizeof(xbiosmode_t));
			b = !b;
		    }
		    *SDL_modelist[i][j] = saved_rect[b];
		    SDL_memcpy(SDL_xbiosmode[i][j], &saved_mode[b], sizeof(xbiosmode_t));
		}
	} else {
		++SDL_nummodes[i];
	}
}

static int XBIOS_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int i;

	/* Initialize all variables that we clean on shutdown */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		SDL_nummodes[i] = 0;
		SDL_modelist[i] = NULL;
		SDL_xbiosmode[i] = NULL;
	}

	/* Allocate memory for old palette */
	XBIOS_oldpalette = (void *)SDL_malloc(256*sizeof(long));
	if ( !XBIOS_oldpalette ) {
		SDL_SetError("Unable to allocate memory for old palette\n");
		return(-1);
	}

	/* Determine the current screen size */
	this->info.current_w = 0;
	this->info.current_h = 0;

	/* Determine the screen depth (use default 8-bit depth) */
	vformat->BitsPerPixel = 8;

	/* Save current mode, may update current screen size or preferred depth */
	(*XBIOS_saveMode)(this, vformat);

	/* First allocate room for needed video modes */
	(*XBIOS_listModes)(this, 0);

	for ( i=0; i<NUM_MODELISTS; ++i ) {
		int j;

		SDL_xbiosmode[i] = (xbiosmode_t **)
			SDL_malloc((SDL_nummodes[i]+1)*sizeof(xbiosmode_t *));
		if ( SDL_xbiosmode[i] == NULL ) {
			SDL_OutOfMemory();
			return(-1);
		}
		for ( j=0; j<SDL_nummodes[i]; ++j ) {
			SDL_xbiosmode[i][j]=(xbiosmode_t *)SDL_malloc(sizeof(xbiosmode_t));
			if ( SDL_xbiosmode[i][j] == NULL ) {
				SDL_OutOfMemory();
				return(-1);
			}
			SDL_memset(SDL_xbiosmode[i][j], 0, sizeof(xbiosmode_t));
		}
		SDL_xbiosmode[i][j] = NULL;

		SDL_modelist[i] = (SDL_Rect **)
				SDL_malloc((SDL_nummodes[i]+1)*sizeof(SDL_Rect *));
		if ( SDL_modelist[i] == NULL ) {
			SDL_OutOfMemory();
			return(-1);
		}
		for ( j=0; j<SDL_nummodes[i]; ++j ) {
			SDL_modelist[i][j]=(SDL_Rect *)SDL_malloc(sizeof(SDL_Rect));
			if ( SDL_modelist[i][j] == NULL ) {
				SDL_OutOfMemory();
				return(-1);
			}
			SDL_memset(SDL_modelist[i][j], 0, sizeof(SDL_Rect));
		}
		SDL_modelist[i][j] = NULL;
	}

	/* Now fill the mode list */
	(*XBIOS_listModes)(this, 1);

	XBIOS_screens[0]=NULL;
	XBIOS_screens[1]=NULL;
	XBIOS_shadowscreen=NULL;

	/* Update hardware info */
	this->info.hw_available = 1;
	this->info.video_mem = (Uint32) Atari_SysMalloc(-1L, MX_STRAM);

	/* Init chunky to planar routine */
	SDL_Atari_C2pConvert = SDL_Atari_C2pConvert8;

#if SDL_VIDEO_OPENGL
	SDL_AtariGL_InitPointers(this);
#endif

	/* Disable screensavers */
	if (SDL_XBIOS_TveillePresent(this)) {
		SDL_XBIOS_TveilleDisable(this);
	}

	/* We're done! */
	return(0);
}

static SDL_Rect **XBIOS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	return(SDL_modelist[((format->BitsPerPixel+7)/8)-1]);
}

static void XBIOS_FreeBuffers(_THIS)
{
	(*XBIOS_freeVbuffers)(this);

	if (XBIOS_shadowscreen) {
		Mfree(XBIOS_shadowscreen);
		XBIOS_shadowscreen=NULL;
	}
}

static SDL_Surface *XBIOS_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	int mode, new_depth;
	int num_buffers;
	xbiosmode_t *new_video_mode;
	Uint32 new_screen_size;
	Uint32 modeflags;
	Uint32 lineWidth;

	/* Free current buffers */
	XBIOS_FreeBuffers(this);

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
	new_video_mode = SDL_xbiosmode[bpp][mode];

	modeflags = SDL_FULLSCREEN | SDL_PREALLOC | SDL_HWPALETTE | SDL_HWSURFACE;

	/* Allocate needed buffers: simple/double buffer and shadow surface */
	new_depth = new_video_mode->depth;
	if (new_depth == 4) {
		SDL_Atari_C2pConvert = SDL_Atari_C2pConvert4;
		new_depth=8;
	} else if (new_depth == 8) {
		SDL_Atari_C2pConvert = SDL_Atari_C2pConvert8;
	}

	lineWidth = (*XBIOS_getLineWidth)(this, new_video_mode, width, new_depth);

	new_screen_size = lineWidth * height;
	new_screen_size += 256; /* To align on a 256 byte adress */	

	if (new_video_mode->flags & XBIOSMODE_C2P) {
		XBIOS_shadowscreen = Atari_SysMalloc(new_screen_size, MX_PREFTTRAM);

		if (XBIOS_shadowscreen == NULL) {
			SDL_SetError("Can not allocate %d KB for shadow buffer", new_screen_size>>10);
			return (NULL);
		}
		SDL_memset(XBIOS_shadowscreen, 0, new_screen_size);
	}

	/* Output buffer needs to be twice in size for the software double-line mode */
	if (new_video_mode->flags & XBIOSMODE_DOUBLELINE) {
		new_screen_size <<= 1;
	}

	/* Double buffer ? */
	num_buffers = 1;

#if SDL_VIDEO_OPENGL
	if (flags & SDL_OPENGL) {
		if (this->gl_config.double_buffer) {
			flags |= SDL_DOUBLEBUF;
		}
	}
#endif
	if (flags & SDL_DOUBLEBUF) {
		num_buffers = 2;
		modeflags |= SDL_DOUBLEBUF;
	}

	/* Allocate buffers */
	if (!(*XBIOS_allocVbuffers)(this, num_buffers, new_screen_size)) {
		XBIOS_FreeBuffers(this);
		return (NULL);
	}

	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, new_depth, 0, 0, 0, 0) ) {
		XBIOS_FreeBuffers(this);
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	XBIOS_current = new_video_mode;
	current->w = width;
	current->h = height;
	current->pitch = lineWidth;

	/* this is for C2P conversion */
	XBIOS_pitch = (*XBIOS_getLineWidth)(this, new_video_mode, new_video_mode->width, new_video_mode->depth);

	if (XBIOS_shadowscreen)
		current->pixels = XBIOS_shadowscreen;
	else
		current->pixels = XBIOS_screens[0];

	XBIOS_fbnum = 0;

#if SDL_VIDEO_OPENGL
	if (flags & SDL_OPENGL) {
		if (!SDL_AtariGL_Init(this, current)) {
			XBIOS_FreeBuffers(this);
			SDL_SetError("Can not create OpenGL context");
			return NULL;
		}

		modeflags |= SDL_OPENGL;
	}
#endif

	current->flags = modeflags;

#ifndef DEBUG_VIDEO_XBIOS
	/* Now set the video mode */
	(*XBIOS_setMode)(this, new_video_mode);

	Vsync();
#endif

	this->UpdateRects = XBIOS_updRects;
	XBIOS_recoffset = 2;

	return (current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int XBIOS_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}

static void XBIOS_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static int XBIOS_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void XBIOS_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static void XBIOS_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	SDL_Surface *surface;

	surface = this->screen;

	if (XBIOS_current->flags & XBIOSMODE_C2P) {
		int i;
		int doubleline = (XBIOS_current->flags & XBIOSMODE_DOUBLELINE ? 1 : 0);

		for (i=0;i<numrects;i++) {
			Uint8 *source,*destination;
			int x1,x2;

			x1 = rects[i].x & ~15;
			x2 = rects[i].x+rects[i].w;
			if (x2 & 15) {
				x2 = (x2 | 15) +1;
			}

			source = surface->pixels;
			source += surface->pitch * rects[i].y;
			source += x1;

			destination = XBIOS_screens[XBIOS_fbnum];
			destination += XBIOS_pitch * rects[i].y;
			destination += x1;

			/* Convert chunky to planar screen */
			SDL_Atari_C2pConvert(
				source,
				destination,
				x2-x1,
				rects[i].h,
				doubleline,
				surface->pitch,
				XBIOS_pitch
			);
		}
	}

#ifndef DEBUG_VIDEO_XBIOS
	(*XBIOS_swapVbuffers)(this);

	Vsync();
#endif

	if ((surface->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
		XBIOS_fbnum ^= 1;
		if (!XBIOS_shadowscreen) {
			int src_offset = (surface->locked ? surface->offset : 0);
			surface->pixels=((Uint8 *) XBIOS_screens[XBIOS_fbnum]) + src_offset;
		}
	}
}

static int XBIOS_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	int src_offset;

	if (XBIOS_current->flags & XBIOSMODE_C2P) {
		int doubleline = (XBIOS_current->flags & XBIOSMODE_DOUBLELINE ? 1 : 0);
		int dst_offset;

		src_offset = (surface->locked ? 0 : surface->offset);
		dst_offset = this->offset_y * XBIOS_pitch +
				(this->offset_x & ~15) * this->screen->format->BytesPerPixel;

		/* Convert chunky to planar screen */
		SDL_Atari_C2pConvert(
			surface->pixels + src_offset,
			((Uint8 *) XBIOS_screens[XBIOS_fbnum]) + dst_offset,
			surface->w,
			surface->h,
			doubleline,
			surface->pitch,
			XBIOS_pitch
		);
	}

#ifndef DEBUG_VIDEO_XBIOS
	(*XBIOS_swapVbuffers)(this);

	Vsync();
#endif

	if ((surface->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
		XBIOS_fbnum ^= 1;
		if (!XBIOS_shadowscreen) {
			src_offset = (surface->locked ? surface->offset : 0);
			surface->pixels=((Uint8 *) XBIOS_screens[XBIOS_fbnum]) + src_offset;
		}
	}

	return(0);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
static void XBIOS_VideoQuit(_THIS)
{
	int i,j;

	Atari_ShutdownEvents();

	/* Restore video mode and palette */
#ifndef DEBUG_VIDEO_XBIOS
	(*XBIOS_restoreMode)(this);

	Vsync();
#endif

#if SDL_VIDEO_OPENGL
	if (gl_active) {
		SDL_AtariGL_Quit(this, SDL_TRUE);
	}
#endif

	if (XBIOS_oldpalette) {
		SDL_free(XBIOS_oldpalette);
		XBIOS_oldpalette=NULL;
	}
	XBIOS_FreeBuffers(this);

	/* Free mode list */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		if ( SDL_modelist[i] != NULL ) {
			for ( j=0; SDL_modelist[i][j]; ++j )
				SDL_free(SDL_modelist[i][j]);
			SDL_free(SDL_modelist[i]);
			SDL_modelist[i] = NULL;
		}
		if ( SDL_xbiosmode[i] != NULL ) {
			for ( j=0; SDL_xbiosmode[i][j]; ++j )
				SDL_free(SDL_xbiosmode[i][j]);
			SDL_free(SDL_xbiosmode[i]);
			SDL_xbiosmode[i] = NULL;
		}
	}

	this->screen->pixels = NULL;	

	/* Restore screensavers */
	if (SDL_XBIOS_TveillePresent(this)) {
		SDL_XBIOS_TveilleEnable(this);
	}
}

#if SDL_VIDEO_OPENGL

static void XBIOS_GL_SwapBuffers(_THIS)
{
	SDL_AtariGL_SwapBuffers(this);
	XBIOS_FlipHWSurface(this, this->screen);
	SDL_AtariGL_MakeCurrent(this);
}

#endif
