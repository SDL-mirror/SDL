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
	TT Xbios video functions

	Patrice Mandin
*/

#include <mint/cookie.h>
#include <mint/osbind.h>

#include "../SDL_sysvideo.h"

#include "SDL_xbios.h"

static const xbiosmode_t ttmodes[]={
	{TT_LOW,320,480,8, XBIOSMODE_C2P},
	{TT_LOW,320,240,8, XBIOSMODE_C2P|XBIOSMODE_DOUBLELINE}
};

static void listModes(_THIS, int actually_add);
static void saveMode(_THIS, SDL_PixelFormat *vformat);
static void setMode(_THIS, xbiosmode_t *new_video_mode);
static void restoreMode(_THIS);
static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);

void SDL_XBIOS_VideoInit_TT(_THIS)
{
	XBIOS_listModes = listModes;
	XBIOS_saveMode = saveMode;
	XBIOS_setMode = setMode;
	XBIOS_restoreMode = restoreMode;

	this->SetColors = setColors;
}

static void listModes(_THIS, int actually_add)
{
	int i;

	for (i=0; i<sizeof(ttmodes)/sizeof(xbiosmode_t); i++) {
		SDL_XBIOS_AddMode(this, actually_add, &ttmodes[i]);
	}
}

static void saveMode(_THIS, SDL_PixelFormat *vformat)
{
	XBIOS_oldvbase=Physbase();
	XBIOS_oldvmode=EgetShift();

	switch(XBIOS_oldvmode & ES_MODE) {
		case TT_LOW:
			XBIOS_oldnumcol=256;
			break;
		case ST_LOW:
		case TT_MED:
			XBIOS_oldnumcol=16;
			break;
		case ST_MED:
			XBIOS_oldnumcol=4;
			break;
		case ST_HIGH:
		case TT_HIGH:
			XBIOS_oldnumcol=2;
			break;
	}

	if (XBIOS_oldnumcol) {
		EgetPalette(0, XBIOS_oldnumcol, XBIOS_oldpalette);
	}
}

static void setMode(_THIS, xbiosmode_t *new_video_mode)
{
	Setscreen(-1,XBIOS_screens[0],-1);

	EsetShift(new_video_mode->number);
}

static void restoreMode(_THIS)
{
	Setscreen(-1,XBIOS_oldvbase,-1);

	EsetShift(XBIOS_oldvmode);
	if (XBIOS_oldnumcol) {
		EsetPalette(0, XBIOS_oldnumcol, XBIOS_oldpalette);
	}
}

static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	int	i, r,g,b;

	for(i = 0; i < ncolors; i++) {
		r = colors[i].r;	
		g = colors[i].g;
		b = colors[i].b;
					
		TT_palette[i]=((r>>4)<<8)|((g>>4)<<4)|(b>>4);
	}
	EsetPalette(firstcolor,ncolors,TT_palette);

	return(1);
}
