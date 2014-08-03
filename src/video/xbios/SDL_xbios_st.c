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
	ST/STE Xbios video functions

	Patrice Mandin
*/

#include <mint/cookie.h>
#include <mint/osbind.h>

#include "../SDL_sysvideo.h"

#include "../ataricommon/SDL_atarimxalloc_c.h"
#include "../ataricommon/SDL_ataric2p_s.h"
#include "SDL_xbios.h"

static const xbiosmode_t stmodes[]={
	{ST_LOW>>8,320,200,4, XBIOSMODE_C2P}
};

static void listModes(_THIS, int actually_add);
static void saveMode(_THIS, SDL_PixelFormat *vformat);
static void setMode_ST(_THIS, xbiosmode_t *new_video_mode);
static void setMode_STE(_THIS, xbiosmode_t *new_video_mode);
static void restoreMode(_THIS);
static int getLineWidth(_THIS, xbiosmode_t *new_video_mode, int width, int bpp);
static void swapVbuffers(_THIS);
static int allocVbuffers(_THIS, int num_buffers, int bufsize);
static void freeVbuffers(_THIS);
static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);

void SDL_XBIOS_VideoInit_ST(_THIS, unsigned long cookie_cvdo)
{
	XBIOS_listModes = listModes;
	XBIOS_saveMode = saveMode;
	XBIOS_setMode = setMode_ST;
	XBIOS_restoreMode = restoreMode;
	XBIOS_getLineWidth = getLineWidth;
	XBIOS_swapVbuffers = swapVbuffers;
	XBIOS_allocVbuffers = allocVbuffers;
	XBIOS_freeVbuffers = freeVbuffers;

	this->SetColors = setColors;

	if ((cookie_cvdo>>16) == VDO_STE) {
		XBIOS_setMode = setMode_STE;
	}
}

static void listModes(_THIS, int actually_add)
{
	SDL_XBIOS_AddMode(this, actually_add, &stmodes[0]);
}

static void saveMode(_THIS, SDL_PixelFormat *vformat)
{
	short *oldpalette;
	int i;
		
	XBIOS_oldvbase=Physbase();
	XBIOS_oldvmode=Getrez();

	switch(XBIOS_oldvmode << 8) {
		case ST_LOW:
			XBIOS_oldnumcol=16;
			break;
		case ST_MED:
			XBIOS_oldnumcol=4;
			break;
		case ST_HIGH:
			XBIOS_oldnumcol=2;
		break;
	}

	oldpalette= (short *) XBIOS_oldpalette;
	for (i=0;i<XBIOS_oldnumcol;i++) {
		*oldpalette++=Setcolor(i,-1);
	}
}

static void setMode_ST(_THIS, xbiosmode_t *new_video_mode)
{
	int i;

	Setscreen(-1,XBIOS_screens[0],-1);

	Setscreen(-1,-1,new_video_mode->number);

	/* Reset palette, 8 shades of gray with a bit of green interleaved */
	for (i=0;i<16;i++) {
		TT_palette[i]= ((i>>1)<<8) | (((i*8)/17)<<4) | (i>>1);
	}
	Setpalette(TT_palette);
}

static void setMode_STE(_THIS, xbiosmode_t *new_video_mode)
{
	int i;

	Setscreen(-1,XBIOS_screens[0],-1);

	Setscreen(-1,-1,new_video_mode->number);

	/* Reset palette, 16 shades of gray */
	for (i=0;i<16;i++) {
		int c;

		c=((i&1)<<3)|((i>>1)&7);
		TT_palette[i]=(c<<8)|(c<<4)|c;
	}
	Setpalette(TT_palette);
}

static void restoreMode(_THIS)
{
	Setscreen(-1,XBIOS_oldvbase,XBIOS_oldvmode);

	if (XBIOS_oldnumcol) {
		Setpalette(XBIOS_oldpalette);
	}
}

static int getLineWidth(_THIS, xbiosmode_t *new_video_mode, int width, int bpp)
{
	return (width * (((bpp==15) ? 16 : bpp)>>3));
}

static void swapVbuffers(_THIS)
{
	Setscreen(-1,XBIOS_screens[XBIOS_fbnum],-1);
}

static int allocVbuffers(_THIS, int num_buffers, int bufsize)
{
	int i;

	for (i=0; i<num_buffers; i++) {
		XBIOS_screensmem[i] = Atari_SysMalloc(bufsize, MX_STRAM);

		if (XBIOS_screensmem[i]==NULL) {
			SDL_SetError("Can not allocate %d KB for buffer %d", bufsize>>10, i);
			return (0);
		}
		SDL_memset(XBIOS_screensmem[i], 0, bufsize);

		XBIOS_screens[i]=(void *) (( (long) XBIOS_screensmem[i]+256) & 0xFFFFFF00UL);
	}

	return (1);
}

static void freeVbuffers(_THIS)
{
	int i;

	for (i=0;i<2;i++) {
		if (XBIOS_screensmem[i]) {
			Mfree(XBIOS_screensmem[i]);
		}
		XBIOS_screensmem[i]=NULL;
	}
}

static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	int	i, r,g,b;

 	for (i=0;i<ncolors;i++) {
		r = colors[i].r;	
		g = colors[i].g;
		b = colors[i].b;

		TT_palette[firstcolor+i]=((r*30)+(g*59)+(b*11))/100;
	}

	SDL_Atari_C2pConvert4_pal(TT_palette); /* convert the lighting */

	return(1);
}
