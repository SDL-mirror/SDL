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
	Falcon Xbios video functions

	Patrice Mandin
*/

#include <mint/cookie.h>
#include <mint/osbind.h>
#include <mint/falcon.h>

#include "SDL_xbios.h"
#include "SDL_xbios_blowup.h"
#include "SDL_xbios_sb3.h"
#include "SDL_xbios_centscreen.h"

static const xbiosmode_t rgb_modes[]={
	{BPS16|COL80|OVERSCAN|VERTFLAG,768,480,16,0},
	{BPS16|COL80|OVERSCAN,768,240,16,0},
	{BPS16|COL80|VERTFLAG,640,400,16,0},
	{BPS16|COL80,640,200,16,0},
	{BPS16|OVERSCAN|VERTFLAG,384,480,16,0},
	{BPS16|OVERSCAN,384,240,16,0},
	{BPS16|VERTFLAG,320,400,16,0},
	{BPS16,320,200,16,0},
	{BPS8|COL80|OVERSCAN|VERTFLAG,768,480,8,XBIOSMODE_C2P},
	{BPS8|COL80|OVERSCAN,768,240,8,XBIOSMODE_C2P},
	{BPS8|COL80|VERTFLAG,640,400,8,XBIOSMODE_C2P},
	{BPS8|COL80,640,200,8,XBIOSMODE_C2P},
	{BPS8|OVERSCAN|VERTFLAG,384,480,8,XBIOSMODE_C2P},
	{BPS8|OVERSCAN,384,240,8,XBIOSMODE_C2P},
	{BPS8|VERTFLAG,320,400,8,XBIOSMODE_C2P},
	{BPS8,320,200,8,XBIOSMODE_C2P}
};

static const xbiosmode_t vga_modes[]={
	{BPS16,320,480,16,0},
	{BPS16|VERTFLAG,320,240,16,0},
	{BPS8|COL80,640,480,8,XBIOSMODE_C2P},
	{BPS8|COL80|VERTFLAG,640,240,8,XBIOSMODE_C2P},
	{BPS8,320,480,8,XBIOSMODE_C2P},
	{BPS8|VERTFLAG,320,240,8,XBIOSMODE_C2P}
};

static void listModes(_THIS, int actually_add);
static void saveMode(_THIS, SDL_PixelFormat *vformat);
static void setMode(_THIS, xbiosmode_t *new_video_mode);
static void restoreMode(_THIS);
static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);

void SDL_XBIOS_VideoInit_F30(_THIS)
{
	long cookie_cnts, cookie_scpn;

	XBIOS_listModes = listModes;
	XBIOS_saveMode = saveMode;
	XBIOS_setMode = setMode;
	XBIOS_restoreMode = restoreMode;

	this->SetColors = setColors;

	/* ScreenBlaster 3 ? */
	if (Getcookie(C_SCPN, &cookie_scpn) == C_FOUND) {
		SDL_XBIOS_VideoInit_SB3(this);
	} else
	/* Centscreen ? */
	if (Getcookie(C_CNTS, &cookie_cnts) == C_FOUND) {
		SDL_XBIOS_VideoInit_Centscreen(this);
	}
}

static void listModes(_THIS, int actually_add)
{
	long cookie_blow;
	int i, max_modes = 0;
	const xbiosmode_t *f30_modes = NULL;
	xbiosmode_t modeinfo;

	switch (VgetMonitor()) {
		case MONITOR_RGB:
		case MONITOR_TV:
			max_modes = sizeof(rgb_modes)/sizeof(xbiosmode_t);
			f30_modes = rgb_modes;
			break;
		case MONITOR_VGA:
			max_modes = sizeof(vga_modes)/sizeof(xbiosmode_t);
			f30_modes = vga_modes;
			break;
	}

	for (i=0; i<max_modes; i++) {
		SDL_memcpy(&modeinfo, &f30_modes[i], sizeof(xbiosmode_t));
		modeinfo.number &= ~(VGA|PAL);
		modeinfo.number |= XBIOS_oldvmode & (VGA|PAL);

		SDL_XBIOS_AddMode(this, actually_add, &modeinfo);
	}

	if (Getcookie(C_BLOW, &cookie_blow) == C_FOUND) {
		SDL_XBIOS_ListBlowupModes(this, actually_add, (blow_cookie_t *)cookie_blow);
	}
}

static void saveMode(_THIS, SDL_PixelFormat *vformat)
{
	XBIOS_oldvbase=Physbase();

	XBIOS_oldvmode=VsetMode(-1);

	XBIOS_oldnumcol= 1<< (1 << (XBIOS_oldvmode & NUMCOLS));
	if (XBIOS_oldnumcol > 256) {
		XBIOS_oldnumcol = 256;
	}
	if (XBIOS_oldnumcol) {
		VgetRGB(0, XBIOS_oldnumcol, XBIOS_oldpalette);
	}
}

static void setMode(_THIS, xbiosmode_t *new_video_mode)
{
	Setscreen(-1,XBIOS_screens[0],-1);

	VsetMode(new_video_mode->number);

	/* Set hardware palette to black in True Colour */
	if (new_video_mode->depth > 8) {
		SDL_memset(F30_palette, 0, sizeof(F30_palette));
		VsetRGB(0,256,F30_palette);
	}
}

static void restoreMode(_THIS)
{
	Setscreen(-1,XBIOS_oldvbase,-1);

	VsetMode(XBIOS_oldvmode);

	if (XBIOS_oldnumcol) {
		VsetRGB(0, XBIOS_oldnumcol, XBIOS_oldpalette);
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

	return 1;
}
