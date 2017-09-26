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
	Milan Xbios video functions

	Patrice Mandin
*/

#include <mint/cookie.h>
#include <mint/falcon.h>

#include "SDL_xbios.h"
#include "SDL_xbios_milan.h"

#define NUM_PREDEFINED_MODES 7

typedef struct {
	Uint16 width, height;
} predefined_mode_t;

static const predefined_mode_t mode_list[NUM_PREDEFINED_MODES]={
	{640,400},
	{640,480},
	{800,608},
	{1024,768},
	{1152,864},
	{1280,1024},
	{1600,1200}
};

static const Uint8 mode_bpp[4]={
	8, 15, 16, 32
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
static void swapVbuffers(_THIS);
static int allocVbuffers(_THIS, int num_buffers, int bufsize);
static void freeVbuffers(_THIS);
static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);

void SDL_XBIOS_VideoInit_Milan(_THIS)
{
	XBIOS_listModes = listModes;
	XBIOS_saveMode = saveMode;
	XBIOS_setMode = setMode;
	XBIOS_restoreMode = restoreMode;
	XBIOS_getLineWidth = getLineWidth;
	XBIOS_swapVbuffers = swapVbuffers;
	XBIOS_allocVbuffers = allocVbuffers;
	XBIOS_freeVbuffers = freeVbuffers;

	this->SetColors = setColors;
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
	int i;

	/* Read validated predefined modes */
	for (i=0; i<NUM_PREDEFINED_MODES; i++) {
		int j;
		Uint16 deviceid = 0x1000 + (i<<4);

		for (j=1; j<4; j++) {
			if (Validmode(deviceid + j)) {
				xbiosmode_t modeinfo;

				modeinfo.number = deviceid + j;
				modeinfo.width = mode_list[i].width;
				modeinfo.height = mode_list[i].height;
				modeinfo.depth = mode_bpp[j-1];
				modeinfo.flags = 0;

				SDL_XBIOS_AddMode(this, actually_add, &modeinfo);
			}
		}
	}

	/* Read custom created modes */
	enum_this = this;
	enum_actually_add = actually_add;
	VsetScreen(-1, &enumfunc, MI_MAGIC, CMD_ENUMMODES);
}

static void saveMode(_THIS, SDL_PixelFormat *vformat)
{
	SCREENINFO si;

	/* Read infos about current mode */
	VsetScreen(-1, &XBIOS_oldvmode, MI_MAGIC, CMD_GETMODE);

	si.size = sizeof(SCREENINFO);
	si.devID = XBIOS_oldvmode;
	si.scrFlags = 0;
	VsetScreen(-1, &si, MI_MAGIC, CMD_GETINFO);

	this->info.current_w = si.scrWidth;
	this->info.current_h = si.scrHeight;

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
	VsetScreen(-1, XBIOS_screens[0], MI_MAGIC, CMD_SETADR);

	VsetScreen(-1, new_video_mode->number, MI_MAGIC, CMD_SETMODE);

	/* Set hardware palette to black in True Colour */
	if (new_video_mode->depth > 8) {
		SDL_memset(F30_palette, 0, sizeof(F30_palette));
		VsetRGB(0,256,F30_palette);
	}
}

static void restoreMode(_THIS)
{
	VsetScreen(-1, &XBIOS_oldvbase, MI_MAGIC, CMD_SETADR);
	VsetScreen(-1, &XBIOS_oldvmode, MI_MAGIC, CMD_SETMODE);
	if (XBIOS_oldnumcol) {
		VsetRGB(0, XBIOS_oldnumcol, XBIOS_oldpalette);
	}
}

static void swapVbuffers(_THIS)
{
	VsetScreen(-1, XBIOS_screens[XBIOS_fbnum], MI_MAGIC, CMD_SETADR);
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
			VsetScreen(-1, &XBIOS_screensmem[i], MI_MAGIC, CMD_ALLOCPAGE);
		}

		if (!XBIOS_screensmem[i]) {
			SDL_SetError("Can not allocate %d KB for buffer %d", bufsize>>10, i);
			return (0);
		}
		SDL_memset(XBIOS_screensmem[i], 0, bufsize);

		XBIOS_screens[i]=XBIOS_screensmem[i];
	}

	return (1);
}

static void freeVbuffers(_THIS)
{
	int i;

	for (i=0;i<2;i++) {
		if (XBIOS_screensmem[i]) {
			if (i==1) {
				VsetScreen(-1, -1, MI_MAGIC, CMD_FREEPAGE);
			} else {
				/* Do not touch buffer 0 */
			}
			XBIOS_screensmem[i]=NULL;
		}
	}
}

static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	int i, r,g,b;

	for(i = 0; i < ncolors; i++) {
		r = colors[i].r;
		g = colors[i].g;
		b = colors[i].b;

		F30_palette[i]=(r<<16)|(g<<8)|b;
	}
	VsetRGB(firstcolor,ncolors,F30_palette);

	return (1);
}
