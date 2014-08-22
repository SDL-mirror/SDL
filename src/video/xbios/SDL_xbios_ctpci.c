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
	CTPCI Xbios video functions

	Pawel Goralski
	Patrice Mandin
*/

#include <mint/cookie.h>
#include <mint/osbind.h>
#include <mint/falcon.h>

#include "../SDL_sysvideo.h"

#include "../ataricommon/SDL_atarimxalloc_c.h"

#include "SDL_xbios.h"
#include "SDL_xbios_milan.h"

/* use predefined, hardcoded table if CTPCI_USE_TABLE == 1
   else enumerate all non virtual video modes */
#define CTPCI_USE_TABLE 1

typedef struct {
	Uint16 modecode, width, height;
} predefined_mode_t;

static const predefined_mode_t mode_list[]={
	{0x4260,320,200},
	{0x4270,320,240},
	{0x4138,640,480},
	{0x4160,800,600},
	{0x4188,1024,768},
	{0x42c8,1280,800},
	{0x41e0,1280,1024},
	{0x4210,1600,1200}	
};

static const Uint8 mode_bpp[]={
	8, 16, 32
};

/*--- Variables ---*/

static int enum_actually_add;
static SDL_VideoDevice *enum_this;

/*--- Functions ---*/

static void listModes(_THIS, int actually_add);
static void saveMode(_THIS, SDL_PixelFormat *vformat);
static void setMode(_THIS, xbiosmode_t *new_video_mode);
static void restoreMode(_THIS);
static int getLineWidth(_THIS, xbiosmode_t *new_video_mode, int width, int bpp);
static int allocVbuffers(_THIS, int num_buffers, int bufsize);
static void freeVbuffers(_THIS);
static void updateRects(_THIS, int numrects, SDL_Rect *rects);
static int flipHWSurface(_THIS, SDL_Surface *surface);

void SDL_XBIOS_VideoInit_Ctpci(_THIS)
{
	XBIOS_listModes = listModes;
	XBIOS_saveMode = saveMode;
	XBIOS_setMode = setMode;
	XBIOS_restoreMode = restoreMode;
	XBIOS_getLineWidth = getLineWidth;
	XBIOS_allocVbuffers = allocVbuffers;
	XBIOS_freeVbuffers = freeVbuffers;

	XBIOS_updRects = updateRects;
	this->FlipHWSurface = flipHWSurface;
}

static unsigned long /*cdecl*/ enumfunc(SCREENINFO *inf, unsigned long flag)
{
	xbiosmode_t modeinfo;

	modeinfo.number = inf->devID;
	modeinfo.width = inf->scrWidth;
	modeinfo.height = inf->scrHeight;
	modeinfo.depth = inf->scrPlanes;
	modeinfo.flags = 0;

	SDL_XBIOS_AddMode(enum_this, enum_actually_add, &modeinfo);

	return ENUMMODE_CONT; 
} 

static void listModes(_THIS, int actually_add)
{
#ifdef CTPCI_USE_TABLE
	int i;

	/* Read validated predefined modes */
	for (i=0; i<sizeof(mode_list)/sizeof(predefined_mode_t); i++) {
		int j;
		Uint16 deviceid = mode_list[i].modecode;

		for (j=3; j<5; j++) {
			if (Validmode(deviceid + j)) {
				xbiosmode_t modeinfo;
				
				modeinfo.number = deviceid + j;
				modeinfo.width = mode_list[i].width;
				modeinfo.height = mode_list[i].height;
				modeinfo.depth = mode_bpp[j-3];
				modeinfo.flags = 0;

				SDL_XBIOS_AddMode(this, actually_add, &modeinfo);
			}
		}
	}
#else
	/* Read custom created modes */
	enum_this = this;
	enum_actually_add = actually_add;
	VsetScreen(-1, &enumfunc, VN_MAGIC, CMD_ENUMMODES);
#endif
}

static void saveMode(_THIS, SDL_PixelFormat *vformat)
{
	SCREENINFO si;

	/* Read infos about current mode */ 
	VsetScreen(-1, &XBIOS_oldvmode, VN_MAGIC, CMD_GETMODE);

	si.size = sizeof(SCREENINFO);
	si.devID = XBIOS_oldvmode;
	si.scrFlags = 0;
	VsetScreen(-1, &si, VN_MAGIC, CMD_GETINFO);

	this->info.current_w = si.scrWidth;
	this->info.current_h = si.scrHeight;

	vformat->BitsPerPixel = si.scrPlanes;

	XBIOS_oldnumcol = 0;
	if (si.scrFlags & SCRINFO_OK) {
		if (si.scrPlanes <= 8) {
			XBIOS_oldnumcol = 1<<si.scrPlanes;
		}
	}
	if (XBIOS_oldnumcol) {
		VgetRGB(0, XBIOS_oldnumcol, XBIOS_oldpalette);
	}
}

static void setMode(_THIS, xbiosmode_t *new_video_mode)
{
	VsetScreen(-1, XBIOS_screens[0], VN_MAGIC, CMD_SETADR);

	VsetScreen(-1, new_video_mode->number, VN_MAGIC, CMD_SETMODE);

	/* Set hardware palette to black in True Colour */
	if (new_video_mode->depth > 8) {
		SDL_memset(F30_palette, 0, sizeof(F30_palette));
		VsetRGB(0,256,F30_palette);
	}
}

static void restoreMode(_THIS)
{
	VsetScreen(-1, &XBIOS_oldvbase, VN_MAGIC, CMD_SETADR);
	VsetScreen(-1, &XBIOS_oldvmode, VN_MAGIC, CMD_SETMODE);
	if (XBIOS_oldnumcol) {
		VsetRGB(0, XBIOS_oldnumcol, XBIOS_oldpalette);
	}
}

static int getLineWidth(_THIS, xbiosmode_t *new_video_mode, int width, int bpp)
{
	SCREENINFO si;
	int retvalue = width * (((bpp==15) ? 16 : bpp)>>3);

	/* Set pitch of new mode */
	si.size = sizeof(SCREENINFO);
	si.devID = new_video_mode->number;
	si.scrFlags = 0;
	VsetScreen(-1, &si, VN_MAGIC, CMD_GETINFO);
	if (si.scrFlags & SCRINFO_OK) {
		retvalue = si.lineWrap;
	}

	return (retvalue);
}

static int allocVbuffers(_THIS, int num_buffers, int bufsize)
{
	int i;

	for (i=0; i<num_buffers; i++) {
		if (i==0) {
			/* Buffer 0 is current screen */
			XBIOS_screensmem[i] = XBIOS_oldvbase;
		} else {
			VsetScreen(-1, &XBIOS_screensmem[i], VN_MAGIC, CMD_ALLOCPAGE);
		}

		if (!XBIOS_screensmem[i]) {
			SDL_SetError("Can not allocate %d KB for buffer %d", bufsize>>10, i);
			return (0);
		}
		SDL_memset(XBIOS_screensmem[i], 0, bufsize);

		XBIOS_screens[i]=XBIOS_screensmem[i];
	}

	/*--- Always use shadow buffer ---*/
	if (XBIOS_shadowscreen) {
		Mfree(XBIOS_shadowscreen);
		XBIOS_shadowscreen=NULL;
	}

	/* allocate shadow buffer in TT-RAM, blitting directly to screen is
	   damn slow, send postcards to R.Czuba for fixing TT-RAM->CTPCI burst
	   mode */
	XBIOS_shadowscreen = Atari_SysMalloc(bufsize, MX_PREFTTRAM);

	if (XBIOS_shadowscreen == NULL) {
		SDL_SetError("Can not allocate %d KB for shadow buffer", bufsize>>10);
		return (0);
	}
	SDL_memset(XBIOS_shadowscreen, 0, bufsize);

	return (1);
}

static void freeVbuffers(_THIS)
{
	int i;

	for (i=0;i<2;i++) {
		if (XBIOS_screensmem[i]) {
			if (i==1) {
				VsetScreen(-1, -1, VN_MAGIC, CMD_FREEPAGE);
			} else {
				/* Do not touch buffer 0 */
			}
			XBIOS_screensmem[i]=NULL;
		}
	}
}

static void updateRects(_THIS, int numrects, SDL_Rect *rects)
{
	SDL_Surface *surface;
	int i;
	  
	surface = this->screen;

	for (i=0;i<numrects;i++) {
		Uint8 *blockSrcStart, *blockDstStart;
		int y;
		
		blockSrcStart = (Uint8 *) surface->pixels;
		blockSrcStart += surface->pitch*rects[i].y;
		blockSrcStart += surface->format->BytesPerPixel*rects[i].x;

		blockDstStart = ((Uint8 *) XBIOS_screens[XBIOS_fbnum]) + surface->offset;
		blockDstStart += XBIOS_pitch*rects[i].y;
		blockDstStart += surface->format->BytesPerPixel*rects[i].x;

		for(y=0;y<rects[i].h;y++){
			SDL_memcpy(blockDstStart,blockSrcStart,surface->pitch);

			blockSrcStart += surface->pitch;
			blockDstStart += XBIOS_pitch;
		}
	}

	VsetScreen(-1L, -1L, VN_MAGIC, CMD_FLIPPAGE);
	Vsync();

	if ((surface->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
		XBIOS_fbnum ^= 1;
	}
}

static int flipHWSurface(_THIS, SDL_Surface *surface)
{
	int i;
	Uint8 *src, *dst;

	src = surface->pixels;
	dst = ((Uint8 *) XBIOS_screens[XBIOS_fbnum]) + surface->offset;

	for (i=0; i<surface->h; i++) {
		SDL_memcpy(dst, src, surface->w * surface->format->BytesPerPixel);
		src += surface->pitch;
		dst += XBIOS_pitch;
		
	}

	VsetScreen(-1L, -1L, VN_MAGIC, CMD_FLIPPAGE);
	Vsync();

	if ((surface->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
		XBIOS_fbnum ^= 1;
	}

	return(0);
}
