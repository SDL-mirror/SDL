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

/*
 * Xbios SDL video driver
 * 
 * Patrice Mandin
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Mint includes */
#include <mint/cookie.h>
#include <mint/osbind.h>
#include <mint/falcon.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"

#include "SDL_ataric2p_s.h"
#include "SDL_atarievents_c.h"
#include "SDL_atarimxalloc_c.h"
#include "SDL_xbios.h"

#define XBIOS_VID_DRIVER_NAME "xbios"

/*#define DEBUG_VIDEO_XBIOS 1*/

/* Initialization/Query functions */
static int XBIOS_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **XBIOS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *XBIOS_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int XBIOS_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void XBIOS_VideoQuit(_THIS);

/* Hardware surface functions */
static int XBIOS_AllocHWSurface(_THIS, SDL_Surface *surface);
static int XBIOS_LockHWSurface(_THIS, SDL_Surface *surface);
static int XBIOS_FlipHWSurface(_THIS, SDL_Surface *surface);
static void XBIOS_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void XBIOS_FreeHWSurface(_THIS, SDL_Surface *surface);
static void XBIOS_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

/* List of video modes */

/* ST modes */
static int xbiosnummodes_st=1;
static xbiosmode_t xbiosmodelist_st[]={
	{ST_LOW>>8,320,200,4,SDL_FALSE}
};

/* TT modes */
static int xbiosnummodes_tt=2;
static xbiosmode_t xbiosmodelist_tt[]={
	{TT_LOW,320,480,8,SDL_FALSE},
	{TT_LOW,320,240,8,SDL_TRUE}	/* Software double-lined mode */
};

/* Falcon RVB modes */
static int xbiosnummodes_f30rvb=16;
static xbiosmode_t xbiosmodelist_f30rvb[]={
	{BPS16|COL80|OVERSCAN|VERTFLAG,768,480,16,SDL_FALSE},
	{BPS16|OVERSCAN|VERTFLAG,384,480,16,SDL_FALSE},
	{BPS16|COL80|VERTFLAG,640,400,16,SDL_FALSE},
	{BPS16|VERTFLAG,320,400,16,SDL_FALSE},
	{BPS16|COL80|OVERSCAN,768,240,16,SDL_FALSE},
	{BPS16|OVERSCAN,384,240,16,SDL_FALSE},
	{BPS16|COL80,640,200,16,SDL_FALSE},
	{BPS16,320,200,16,SDL_FALSE},

	{BPS8|COL80|OVERSCAN|VERTFLAG,768,480,8,SDL_FALSE},
	{BPS8|OVERSCAN|VERTFLAG,384,480,8,SDL_FALSE},
	{BPS8|COL80|VERTFLAG,640,400,8,SDL_FALSE},
	{BPS8|VERTFLAG,320,400,8,SDL_FALSE},
	{BPS8|COL80|OVERSCAN,768,240,8,SDL_FALSE},
	{BPS8|OVERSCAN,384,240,8,SDL_FALSE},
	{BPS8|COL80,640,200,8,SDL_FALSE},
	{BPS8,320,200,8,SDL_FALSE}
};

/* Falcon VGA modes */
static int xbiosnummodes_f30vga=6;
static xbiosmode_t xbiosmodelist_f30vga[]={
	{BPS16,320,480,16,SDL_FALSE},
	{BPS16|VERTFLAG,320,240,16,SDL_FALSE},

	{BPS8|COL80,640,480,8,SDL_FALSE},	
	{BPS8,320,480,8,SDL_FALSE},
	{BPS8|COL80|VERTFLAG,640,240,8,SDL_FALSE},
	{BPS8|VERTFLAG,320,240,8,SDL_FALSE}
};

/* To setup palette */

static unsigned short	TT_palette[256];
static unsigned long	F30_palette[256];

/* Xbios driver bootstrap functions */

static int XBIOS_Available(void)
{
	unsigned long cookie_vdo, cookie_mil, cookie_hade;

	/* Milan/Hades Atari clones do not have an Atari video chip */
	if ( (Getcookie(C__MIL, &cookie_mil) == C_FOUND) ||
		(Getcookie(C_hade, &cookie_hade) == C_FOUND) ) {
		return 0;
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
			if ( Montype() == MONITOR_MONO)
				return 0;
			break;
		default:
			return 0;
	}

	return 1;
}

static void XBIOS_DeleteDevice(SDL_VideoDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_VideoDevice *XBIOS_CreateDevice(int devindex)
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

	/* Video functions */
	device->VideoInit = XBIOS_VideoInit;
	device->ListModes = XBIOS_ListModes;
	device->SetVideoMode = XBIOS_SetVideoMode;
	device->SetColors = XBIOS_SetColors;
	device->UpdateRects = NULL;
	device->VideoQuit = XBIOS_VideoQuit;
	device->AllocHWSurface = XBIOS_AllocHWSurface;
	device->LockHWSurface = XBIOS_LockHWSurface;
	device->UnlockHWSurface = XBIOS_UnlockHWSurface;
	device->FlipHWSurface = XBIOS_FlipHWSurface;
	device->FreeHWSurface = XBIOS_FreeHWSurface;

	/* Events */
	device->InitOSKeymap = Atari_InitOSKeymap;
	device->PumpEvents = Atari_PumpEvents;

	device->free = XBIOS_DeleteDevice;

	return device;
}

VideoBootStrap XBIOS_bootstrap = {
	XBIOS_VID_DRIVER_NAME, "Atari Xbios driver",
	XBIOS_Available, XBIOS_CreateDevice
};

static int XBIOS_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int i,j8,j16;
	xbiosmode_t *current_mode;

	/* Initialize all variables that we clean on shutdown */
	memset (SDL_modelist, 0, sizeof(SDL_modelist));

	/* Cookie _VDO present ? if not, assume ST machine */
	if (Getcookie(C__VDO, &XBIOS_cvdo) != C_FOUND) {
		XBIOS_cvdo = VDO_ST << 16;
	}

	/* Allocate memory for old palette */
	XBIOS_oldpalette = (void *)malloc(256*sizeof(long));
	if ( !XBIOS_oldpalette ) {
		SDL_SetError("Unable to allocate memory for old palette\n");
		return(-1);
	}

	/* Initialize video mode list */
	/* and save current screen status (palette, screen address, video mode) */

	switch (XBIOS_cvdo >>16) {
		case VDO_ST:
		case VDO_STE:
			{
				short *oldpalette;

				XBIOS_nummodes=xbiosnummodes_st;
				XBIOS_modelist=xbiosmodelist_st;
			
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
					default:
						XBIOS_oldnumcol=0;
						break;
				}

				oldpalette= (short *) XBIOS_oldpalette;
				for (i=0;i<XBIOS_oldnumcol;i++) {
					*oldpalette++=Setcolor(i,-1);
				}

				vformat->BitsPerPixel = 8;
			}
			break;
		case VDO_TT:
			XBIOS_nummodes=xbiosnummodes_tt;
			XBIOS_modelist=xbiosmodelist_tt;

			XBIOS_oldvbase=Logbase();
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
				default:
					XBIOS_oldnumcol=0;
					break;
			}
			if (XBIOS_oldnumcol) {
				EgetPalette(0, XBIOS_oldnumcol, XBIOS_oldpalette);
			}

			vformat->BitsPerPixel = 8;
			break;
		case VDO_F30:
			switch (Montype())
			{
				case MONITOR_MONO:
					/* Not usable */
					break;
				case MONITOR_RGB:
				case MONITOR_TV:
					XBIOS_nummodes = xbiosnummodes_f30rvb;
					XBIOS_modelist = xbiosmodelist_f30rvb;
					break;
				case MONITOR_VGA:
					XBIOS_nummodes = xbiosnummodes_f30vga;
					XBIOS_modelist = xbiosmodelist_f30vga;
					break;
			}
			XBIOS_oldvbase=Logbase();
			XBIOS_oldvmode=Vsetmode(-1);

			XBIOS_oldnumcol= 1<< (1 << (XBIOS_oldvmode & NUMCOLS));
			if (XBIOS_oldnumcol > 256) {
				XBIOS_oldnumcol = 0;
			}
			if (XBIOS_oldnumcol) {
				VgetRGB(0, XBIOS_oldnumcol, XBIOS_oldpalette);
			}

			vformat->BitsPerPixel = 16;

			/* Keep vga/rvb, and pal/ntsc bits */
			current_mode = XBIOS_modelist;
			for (i=0;i<XBIOS_nummodes;i++) {
				Uint16 newvmode;			

				newvmode = current_mode->number;
				newvmode &= ~(VGA|PAL);
				newvmode |= XBIOS_oldvmode & (VGA|PAL);
				current_mode->number = newvmode;
				
				current_mode++;
			}

			break;
	}
	
	current_mode = XBIOS_modelist;
	j8 = j16 = 0;
	for (i=0;i<XBIOS_nummodes;i++) {
		switch (current_mode->depth) {
			case 4:
			case 8:
				SDL_modelist[0][j8] = malloc(sizeof(SDL_Rect));
				SDL_modelist[0][j8]->x = SDL_modelist[0][j8]->y = 0;
				SDL_modelist[0][j8]->w = current_mode->width;
				SDL_modelist[0][j8]->h = current_mode->height;
				XBIOS_videomodes[0][j8]=current_mode;
				current_mode++;
				j8++;
				break;
			case 16:
				SDL_modelist[1][j16] = malloc(sizeof(SDL_Rect));
				SDL_modelist[1][j16]->x = SDL_modelist[1][j16]->y = 0;
				SDL_modelist[1][j16]->w = current_mode->width;
				SDL_modelist[1][j16]->h = current_mode->height;
				XBIOS_videomodes[1][j16]=current_mode;
				current_mode++;
				j16++;
				break;
		}		
	}
	SDL_modelist[0][j8] = NULL;
	SDL_modelist[1][j16] = NULL;

	XBIOS_screens[0]=NULL;
	XBIOS_screens[1]=NULL;
	XBIOS_shadowscreen=NULL;

	/* Update hardware info */
	this->info.hw_available = 1;
	this->info.video_mem = (Uint32) Atari_SysMalloc(-1L, MX_STRAM);

	/* Init chunky to planar routine */
	SDL_Atari_C2pConvert = SDL_Atari_C2pConvert8;

	/* We're done! */
	return(0);
}

static SDL_Rect **XBIOS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	/* 8 bits -> list 0 */
	/* 16 bits -> list 1 */
	if ((format->BitsPerPixel != 8) && (format->BitsPerPixel !=16)) {
		return NULL;
	}

	return(SDL_modelist[(format->BitsPerPixel)>>4]);
}

static void XBIOS_FreeBuffers(_THIS)
{
	int i;

	for (i=0;i<2;i++) {
		if (XBIOS_screensmem[i]!=NULL) {
			Mfree(XBIOS_screensmem[i]);
			XBIOS_screensmem[i]=NULL;
		}
	}

	if (XBIOS_shadowscreen!=NULL) {
		Mfree(XBIOS_shadowscreen);
		XBIOS_shadowscreen=NULL;
	}
}

static SDL_Surface *XBIOS_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	int mode, new_depth;
	int i;
	xbiosmode_t *new_video_mode;
	Uint32 new_screen_size;
	Uint32 modeflags;

	/* Free current buffers */
	XBIOS_FreeBuffers(this);

	/* Limit bpp */
	if (bpp>16) {
		bpp = 16;
	}
	bpp >>= 4;

	/* Search if the mode exists (width, height, bpp) */
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

	modeflags = SDL_FULLSCREEN;

	/* Allocate needed buffers: simple/double buffer and shadow surface */
	new_video_mode = XBIOS_videomodes[bpp][mode];
	new_depth = new_video_mode->depth;
	if (new_depth == 4) {
		SDL_Atari_C2pConvert = SDL_Atari_C2pConvert4;
		new_depth=8;
		modeflags |= SDL_SWSURFACE;
	} else if (new_depth == 8) {
		SDL_Atari_C2pConvert = SDL_Atari_C2pConvert8;
		modeflags |= SDL_SWSURFACE|SDL_HWPALETTE;
	} else {
		modeflags |= SDL_HWSURFACE;
	}

	new_screen_size = width * height * ((new_depth)>>3);
	new_screen_size += 256; /* To align on a 256 byte adress */	

	if (new_depth == 8) {
		XBIOS_shadowscreen = Atari_SysMalloc(new_screen_size, MX_PREFTTRAM);

		if (XBIOS_shadowscreen == NULL) {
			SDL_SetError("XBIOS_SetVideoMode: Not enough memory for shadow surface");
			return (NULL);
		}
		memset(XBIOS_shadowscreen, 0, new_screen_size);
	}

	/* Output buffer needs to be twice in size for the software double-line mode */
	XBIOS_doubleline = SDL_FALSE;
	if (new_video_mode->doubleline) {
		new_screen_size <<= 1;
		XBIOS_doubleline = SDL_TRUE;
	}

	XBIOS_screensmem[0] = Atari_SysMalloc(new_screen_size, MX_STRAM);

	if (XBIOS_screensmem[0]==NULL) {
		XBIOS_FreeBuffers(this);
		SDL_SetError("XBIOS_SetVideoMode: Not enough memory for video buffer");
		return (NULL);
	}
	memset(XBIOS_screensmem[0], 0, new_screen_size);

	XBIOS_screens[0]=(void *) (( (long) XBIOS_screensmem[0]+256) & 0xFFFFFF00UL);

	/* Double buffer ? */
	if (flags & SDL_DOUBLEBUF) {
		XBIOS_screensmem[1] = Atari_SysMalloc(new_screen_size, MX_STRAM);

		if (XBIOS_screensmem[1]==NULL) {
			XBIOS_FreeBuffers(this);
			SDL_SetError("XBIOS_SetVideoMode: Not enough memory for double buffer");
			return (NULL);
		}
		memset(XBIOS_screensmem[1], 0, new_screen_size);

		XBIOS_screens[1]=(void *) (( (long) XBIOS_screensmem[1]+256) & 0xFFFFFF00UL);
		modeflags |= SDL_DOUBLEBUF;
	}
	
	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, new_depth, 0, 0, 0, 0) ) {
		XBIOS_FreeBuffers(this);
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	current->flags = modeflags;
	current->w = XBIOS_width = width;
	current->h = XBIOS_height = height;
	current->pitch = (width * new_depth)>>3;

	/* this is for C2P conversion */
	XBIOS_pitch = (new_video_mode->width * new_video_mode->depth)>>3;

	if (new_depth == 8)
		current->pixels = XBIOS_shadowscreen;
	else
		current->pixels = XBIOS_screens[0];

	XBIOS_fbnum = 0;

	/* Now set the video mode */
#ifndef DEBUG_VIDEO_XBIOS
	Setscreen(-1,XBIOS_screens[0],-1);
#endif

	switch(XBIOS_cvdo >> 16) {
		case VDO_ST:
#ifndef DEBUG_VIDEO_XBIOS
			Setscreen(-1,-1,new_video_mode->number);
#endif
			/* Reset palette */
			for (i=0;i<16;i++) {
				int c;

				c = ((i>>1)<<8) | ((i>>1)<<4) | (i>>1);
				if ((i & 1) && (i<15))
					c += (1<<4);
				if (i==14)
					c -= 1<<8;

				TT_palette[i]= c;
			}
#ifndef DEBUG_VIDEO_XBIOS
			Setpalette(TT_palette);
#endif
			break;
		case VDO_STE:
#ifndef DEBUG_VIDEO_XBIOS
			Setscreen(-1,-1,new_video_mode->number);
#endif
			/* Reset palette */
			for (i=0;i<16;i++)
			{
				int c;

				c=((i&1)<<3)|((i>>1)&7);
				TT_palette[i]=(c<<8)|(c<<4)|c;
			}
#ifndef DEBUG_VIDEO_XBIOS
			Setpalette(TT_palette);
#endif
			break;
		case VDO_TT:
#ifndef DEBUG_VIDEO_XBIOS
			EsetShift(new_video_mode->number);
#endif
			break;
		case VDO_F30:
#ifndef DEBUG_VIDEO_XBIOS
			Vsetmode(new_video_mode->number);
#endif
			break;
	}

	Vsync();

	this->UpdateRects = XBIOS_UpdateRects;

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

	if ((surface->format->BitsPerPixel) == 8) {
		void *destscr;
		int destx;
		int i;

		/* Center on destination screen */
		destscr = XBIOS_screens[XBIOS_fbnum];
		destscr += XBIOS_pitch * ((XBIOS_height - surface->h) >> 1);
		destx = (XBIOS_width - surface->w) >> 1;
		destx &= ~15;
		destscr += destx;

		for (i=0;i<numrects;i++) {
			void *source,*destination;
			int x1,x2;

			x1 = rects[i].x & ~15;
			x2 = rects[i].x+rects[i].w;
			if (x2 & 15) {
				x2 = (x2 | 15) +1;
			}

			source = surface->pixels;
			source += surface->pitch * rects[i].y;
			source += x1;

			destination = destscr;
			destination += XBIOS_pitch * rects[i].y;
			destination += x1;

			/* Convert chunky to planar screen */
			SDL_Atari_C2pConvert(
				source,
				destination,
				x2-x1,
				rects[i].h,
				XBIOS_doubleline,
				surface->pitch,
				XBIOS_pitch
			);
		}
	}

#ifndef DEBUG_VIDEO_XBIOS
	Setscreen(-1,XBIOS_screens[XBIOS_fbnum],-1);
#endif
	Vsync();

	if ((surface->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
		XBIOS_fbnum ^= 1;
		if ((surface->format->BitsPerPixel) > 8) {
			surface->pixels=XBIOS_screens[XBIOS_fbnum];
		}
	}
}

static int XBIOS_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	if ((surface->format->BitsPerPixel) == 8) {
		void *destscr;
		int destx;
			
		/* Center on destination screen */
		destscr = XBIOS_screens[XBIOS_fbnum];
		destscr += XBIOS_pitch * ((XBIOS_height - surface->h) >> 1);
		destx = (XBIOS_width - surface->w) >> 1;
		destx &= ~15;
		destscr += destx;

		/* Convert chunky to planar screen */
#ifdef DEBUG_VIDEO_XBIOS
		printf("C2p:\n");
		printf(" Source: Adr=0x%08x, Pitch=%d\n", surface->pixels, surface->pitch);
		printf(" Dest: Adr=0x%08x, Pitch=%d\n", destscr, XBIOS_pitch);
		printf(" Size: %dx%d, dblline=%d\n", surface->w, surface->h, XBIOS_doubleline);
		fflush(stdout);
#endif
		SDL_Atari_C2pConvert(
			surface->pixels,
			destscr,
			surface->w,
			surface->h,
			XBIOS_doubleline,
			surface->pitch,
			XBIOS_pitch
		);
	}

#ifndef DEBUG_VIDEO_XBIOS
	Setscreen(-1,XBIOS_screens[XBIOS_fbnum],-1);
#endif
	Vsync();

	if ((surface->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
		XBIOS_fbnum ^= 1;
		if ((surface->format->BitsPerPixel) > 8) {
			surface->pixels=XBIOS_screens[XBIOS_fbnum];
		}
	}

	return(0);
}

static int XBIOS_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	int		i;
	int		r,v,b;

	switch( XBIOS_cvdo >> 16) {
		case VDO_ST:
		case VDO_STE:
		 	for (i=0;i<ncolors;i++)
			{
				r = colors[i].r;	
				v = colors[i].g;
				b = colors[i].b;

				TT_palette[firstcolor+i]=((r*30)+(v*59)+(b*11))/100;
			}
			SDL_Atari_C2pConvert4_pal(TT_palette); /* convert the lighting */
			break;
		case VDO_TT:
			for(i = 0; i < ncolors; i++)
			{
				r = colors[i].r;	
				v = colors[i].g;
				b = colors[i].b;
					
				TT_palette[i]=((r>>4)<<8)|((v>>4)<<4)|(b>>4);
			}
#ifndef DEBUG_VIDEO_XBIOS
			EsetPalette(firstcolor,ncolors,TT_palette);
#endif
			break;
		case VDO_F30:
			for(i = 0; i < ncolors; i++)
			{
				r = colors[i].r;	
				v = colors[i].g;
				b = colors[i].b;

				F30_palette[i]=(r<<16)|(v<<8)|b;
			}
#ifndef DEBUG_VIDEO_XBIOS
			VsetRGB(firstcolor,ncolors,F30_palette);
#endif
			break;
	}

	return(1);
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
	switch(XBIOS_cvdo >> 16) {
		case VDO_ST:
		case VDO_STE:
			Setscreen(-1,XBIOS_oldvbase,XBIOS_oldvmode);
			if (XBIOS_oldnumcol) {
				Setpalette(XBIOS_oldpalette);
			}
			break;
		case VDO_TT:
			Setscreen(-1,XBIOS_oldvbase,-1);
			EsetShift(XBIOS_oldvmode);
			if (XBIOS_oldnumcol) {
				EsetPalette(0, XBIOS_oldnumcol, XBIOS_oldpalette);
			}
			break;
		case VDO_F30:
			Setscreen(-1, XBIOS_oldvbase, -1);
			Vsetmode(XBIOS_oldvmode);
			if (XBIOS_oldnumcol) {
				VsetRGB(0, XBIOS_oldnumcol, XBIOS_oldpalette);
			}
			break;
	}
	Vsync();
#endif

	if (XBIOS_oldpalette) {
		free(XBIOS_oldpalette);
		XBIOS_oldpalette=NULL;
	}
	XBIOS_FreeBuffers(this);

	/* Free mode list */
	for (j=0;j<NUM_MODELISTS;j++) {
		for (i=0;i<SDL_NUMMODES;i++) {
			if (SDL_modelist[j][i]!=NULL) {
				free(SDL_modelist[j][i]);
				SDL_modelist[j][i]=NULL;
			}
		}
	}

	this->screen->pixels = NULL;	
}
