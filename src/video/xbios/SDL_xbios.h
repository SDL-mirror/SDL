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

#ifndef _SDL_xbios_h
#define _SDL_xbios_h

#include "SDL_stdinc.h"
#include "../SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

#define XBIOSMODE_DOUBLELINE (1<<0)
#define XBIOSMODE_C2P (1<<1)

typedef struct
{
	Uint16 number;		/* Video mode number */
	Uint16 width;		/* Size */	
	Uint16 height;
	Uint16 depth;		/* bits per plane */
	Uint16 flags;
} xbiosmode_t;

/* Private display data */
#define NUM_MODELISTS	4		/* 8, 16, 24, and 32 bits-per-pixel */

struct SDL_PrivateVideoData {
	long old_video_mode;				/* Old video mode before entering SDL */
	void *old_video_base;			/* Old pointer to screen buffer */
	void *old_palette;				/* Old palette */
	Uint32 old_num_colors;			/* Nb of colors in saved palette */

	void *screens[2];		/* Pointers to aligned screen buffer */
	void *screensmem[2];	/* Pointers to screen buffer */
	void *shadowscreen;		/* Shadow screen for c2p conversion */
	int frame_number;		/* Number of frame for double buffer */
	int pitch;				/* Destination line width for C2P */
	int recalc_offset;		/* Recalculate SDL_Surface offset for C2P */

	xbiosmode_t *current;	/* Current set mode */
	int SDL_nummodes[NUM_MODELISTS];
	SDL_Rect **SDL_modelist[NUM_MODELISTS];
	xbiosmode_t **SDL_xbiosmode[NUM_MODELISTS];

	union {
		Uint16 pal16[16];
		Uint32 pal32[256];
	} palette;

	void (*listModes)(_THIS, int actually_add);	/* List video modes */
	void (*saveMode)(_THIS, SDL_PixelFormat *vformat);	/* Save mode,palette,vbase change format if needed */
	void (*setMode)(_THIS, xbiosmode_t *new_video_mode);	/* Set mode */
	void (*restoreMode)(_THIS);	/* Restore system mode */
	int (*getLineWidth)(_THIS, xbiosmode_t *new_video_mode, int width, int bpp);	/* Return video mode pitch */
	void (*swapVbuffers)(_THIS);	/* Swap video buffers */
	int (*allocVbuffers)(_THIS, int num_buffers, int bufsize);	/* Allocate video buffers */
	void (*freeVbuffers)(_THIS);	/* Free video buffers */

	void (*updRects)(_THIS, int numrects, SDL_Rect *rects);	/* updateRects to use when video ready */
};

/* _VDO cookie values */
enum {
	VDO_ST=0,
	VDO_STE,
	VDO_TT,
	VDO_F30,
	VDO_MILAN
};

/* Monitor types */
enum {
	MONITOR_MONO=0,
	MONITOR_TV,
	MONITOR_VGA,
	MONITOR_RGB
};

/* EgetShift masks */
#define ES_MODE		0x0700

/* Hidden structure -> variables names */
#define SDL_nummodes		(this->hidden->SDL_nummodes)
#define SDL_modelist		(this->hidden->SDL_modelist)
#define SDL_xbiosmode		(this->hidden->SDL_xbiosmode)
#define XBIOS_oldpalette	(this->hidden->old_palette)
#define XBIOS_oldnumcol		(this->hidden->old_num_colors)
#define XBIOS_oldvbase		(this->hidden->old_video_base)
#define XBIOS_oldvmode		(this->hidden->old_video_mode)
#define XBIOS_screens		(this->hidden->screens)
#define XBIOS_screensmem	(this->hidden->screensmem)
#define XBIOS_shadowscreen	(this->hidden->shadowscreen)
#define XBIOS_fbnum			(this->hidden->frame_number)
#define XBIOS_pitch			(this->hidden->pitch)
#define XBIOS_current		(this->hidden->current)
#define XBIOS_recoffset		(this->hidden->recalc_offset)

#define TT_palette		(this->hidden->palette.pal16)
#define F30_palette		(this->hidden->palette.pal32)

#define XBIOS_listModes		(this->hidden->listModes)
#define XBIOS_saveMode		(this->hidden->saveMode)
#define XBIOS_setMode		(this->hidden->setMode)
#define XBIOS_restoreMode	(this->hidden->restoreMode)
#define XBIOS_getLineWidth	(this->hidden->getLineWidth)
#define XBIOS_swapVbuffers	(this->hidden->swapVbuffers)
#define XBIOS_allocVbuffers	(this->hidden->allocVbuffers)
#define XBIOS_freeVbuffers	(this->hidden->freeVbuffers)

#define XBIOS_updRects		(this->hidden->updRects)

/*--- Functions prototypes ---*/

void SDL_XBIOS_AddMode(_THIS, int actually_add, const xbiosmode_t *modeinfo);

/* SDL_xbios_st.c */
void SDL_XBIOS_VideoInit_ST(_THIS, unsigned long cookie_cvdo);

/* SDL_xbios_tt.c */
void SDL_XBIOS_VideoInit_TT(_THIS);

/* SDL_xbios_f30.c */
void SDL_XBIOS_VideoInit_F30(_THIS);

/* SDL_xbios_milan.c */
void SDL_XBIOS_VideoInit_Milan(_THIS);

/* SDL_xbios_ctpci.c */
void SDL_XBIOS_VideoInit_Ctpci(_THIS);

#endif /* _SDL_xbios_h */
