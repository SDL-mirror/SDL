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
	Centscreen extension definitions

	Patrice Mandin
*/

#include <mint/cookie.h>
#include <mint/falcon.h>

#include "SDL_xbios.h"
#include "SDL_xbios_centscreen.h"

static void listModes(_THIS, int actually_add);
static void saveMode(_THIS, SDL_PixelFormat *vformat);
static void setMode(_THIS, xbiosmode_t *new_video_mode);
static void restoreMode(_THIS);

void SDL_XBIOS_VideoInit_Centscreen(_THIS)
{
	XBIOS_listModes = listModes;
	XBIOS_saveMode = saveMode;
	XBIOS_setMode = setMode;
	XBIOS_restoreMode = restoreMode;
}

static void listModes(_THIS, int actually_add)
{
	centscreen_mode_t curmode, listedmode;
	unsigned long result;

	/* Add Centscreen modes */
	Vread(&curmode);
	curmode.mode = curmode.physx = curmode.physy = curmode.plan =
		curmode.logx = curmode.logy = -1;

	result = Vfirst(&curmode, &listedmode);
	while (result==0) {
		/* Don't add modes with virtual screen */
		if ((listedmode.mode & CSCREEN_VIRTUAL)==0) {
			/* Don't add modes with bpp<8 */
			if (listedmode.plan>=8) {
				xbiosmode_t modeinfo;

				modeinfo.number = listedmode.mode;
				modeinfo.width = listedmode.physx;
				modeinfo.height = listedmode.physy;
				modeinfo.depth = listedmode.plan;
				modeinfo.flags = (modeinfo.depth == 8 ? XBIOSMODE_C2P : 0);

				SDL_XBIOS_AddMode(this, actually_add, &modeinfo);
			}
		}
		SDL_memcpy(&curmode, &listedmode, sizeof(centscreen_mode_t));
		curmode.mode = curmode.physx = curmode.physy = curmode.plan =
			curmode.logx = curmode.logy = -1;
		result = Vnext(&curmode, &listedmode);
	}
}

static void saveMode(_THIS, SDL_PixelFormat *vformat)
{
	centscreen_mode_t curmode;

	XBIOS_oldvbase=Physbase();

	Vread(&curmode);
	XBIOS_oldvmode = curmode.handle;

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
	centscreen_mode_t newmode, curmode;

	Setscreen(-1,XBIOS_screens[0],-1);

	newmode.handle = newmode.mode = newmode.logx = newmode.logy = -1;
	newmode.physx = new_video_mode->width;
	newmode.physy = new_video_mode->height;
	newmode.plan = new_video_mode->depth;
	Vwrite(0, &newmode, &curmode);

#ifdef SDL_VIDEO_DISABLE_SCREENSAVER
	/* Disable screensaver */
	Vread(&newmode);
	newmode.mode &= ~(CSCREEN_SAVER|CSCREEN_ENERGYSTAR);
	Vwrite(0, &newmode, &curmode);
#endif /* SDL_VIDEO_DISABLE_SCREENSAVER */

	/* Set hardware palette to black in True Colour */
	if (new_video_mode->depth > 8) {
		SDL_memset(F30_palette, 0, sizeof(F30_palette));
		VsetRGB(0,256,F30_palette);
	}
}

static void restoreMode(_THIS)
{
	centscreen_mode_t newmode, curmode;

	Setscreen(-1,XBIOS_oldvbase,-1);

	newmode.handle = XBIOS_oldvmode;
	newmode.mode = newmode.physx = newmode.physy = newmode.plan =
		newmode.logx = newmode.logy = -1;
	Vwrite(0, &newmode, &curmode);

	if (XBIOS_oldnumcol) {
		VsetRGB(0, XBIOS_oldnumcol, XBIOS_oldpalette);
	}
}
