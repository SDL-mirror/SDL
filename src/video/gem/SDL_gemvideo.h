/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

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

#ifndef _SDL_gemvideo_h
#define _SDL_gemvideo_h

#include "SDL_sysvideo.h"
#include "SDL_mutex.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

/* Functions prototypes */
void GEM_wind_redraw(_THIS, int winhandle, short *inside);

/* Private display data */

#define SDL_NUMMODES	1		/* Fullscreen */

struct SDL_PrivateVideoData {
	/* VDI infos */
	short vdi_handle;			/* VDI handle */
	short full_w, full_h;		/* Fullscreen size */
    int bpp;					/* Colour depth */
	int pixelsize;				/* Bytes per pixel */
	Uint16 old_numcolors;		/* Number of colors in saved palette */
	Uint16 old_palette[256][3];	/* Saved current palette */
	Uint16 pitch;				/* Line length */
	int format;					/* Screen format */
	void *screen;				/* Screen address */
	Uint32 red, green, blue, alpha;	/* Screen components */
	Uint32 screensize;
	MFDB	src_mfdb, dst_mfdb;	/* VDI MFDB for bitblt */
	short	blit_coords[8];		/* Coordinates for bitblt */
	/* Gem infos */
	short ap_id;				/* AES handle */
	short desk_x, desk_y;		/* Desktop properties */
	short desk_w, desk_h;
	short win_handle;			/* Our window handle */
    void *buffer;				/* Our shadow buffer */
	int window_type;			/* Window type */
	const char *title_name;		/* Window title */
	const char *icon_name;		/* Icon title */
	short version;				/* AES version */
	short wfeatures;			/* AES window features */
	SDL_bool window_fulled;		/* Window maximized ? */
	SDL_bool mouse_relative;	/* Report relative mouse movement */
	SDL_bool locked;			/* AES locked for fullscreen ? */
	SDL_Rect *SDL_modelist[SDL_NUMMODES+1];	/* Mode list */
};

/* Hidden structure -> variables names */
#define VDI_handle			(this->hidden->vdi_handle)
#define VDI_w				(this->hidden->full_w)
#define VDI_h				(this->hidden->full_h)
#define VDI_bpp				(this->hidden->bpp)
#define VDI_pixelsize		(this->hidden->pixelsize)
#define VDI_oldnumcolors	(this->hidden->old_numcolors)
#define VDI_oldpalette		(this->hidden->old_palette)
#define VDI_pitch			(this->hidden->pitch)
#define VDI_format			(this->hidden->format)
#define VDI_screen			(this->hidden->screen)
#define VDI_redmask			(this->hidden->red)
#define VDI_greenmask		(this->hidden->green)
#define VDI_bluemask		(this->hidden->blue)
#define VDI_alphamask		(this->hidden->alpha)
#define VDI_screensize		(this->hidden->screensize)
#define VDI_src_mfdb		(this->hidden->src_mfdb)
#define VDI_dst_mfdb		(this->hidden->dst_mfdb)
#define VDI_blit_coords		(this->hidden->blit_coords)
#define GEM_ap_id			(this->hidden->ap_id)
#define GEM_desk_x			(this->hidden->desk_x)
#define GEM_desk_y			(this->hidden->desk_y)
#define GEM_desk_w			(this->hidden->desk_w)
#define GEM_desk_h			(this->hidden->desk_h)
#define GEM_handle			(this->hidden->win_handle)
#define GEM_buffer			(this->hidden->buffer)
#define GEM_win_type		(this->hidden->window_type)
#define GEM_title_name		(this->hidden->title_name)
#define GEM_icon_name		(this->hidden->icon_name)
#define GEM_version			(this->hidden->version)
#define GEM_wfeatures		(this->hidden->wfeatures)
#define GEM_win_fulled		(this->hidden->window_fulled)
#define GEM_mouse_relative	(this->hidden->mouse_relative)
#define GEM_locked			(this->hidden->locked)
#define SDL_modelist		(this->hidden->SDL_modelist)

#endif /* _SDL_gemvideo_h */
