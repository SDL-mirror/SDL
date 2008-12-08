/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

typedef struct SDL_VideoData
{
    long cookie_vdo;            /* _VDO cookie */

    Uint16 old_modecode;        /* Current video mode */
    void *old_vbase;            /* Current pointer to video RAM */
    int old_numcol;             /* Number of colors in saved palette */
    Uint32 old_palette[256];    /* Buffer to save current palette */

#if 0
    int old_video_mode;         /* Old video mode before entering SDL */
    void *old_video_base;       /* Old pointer to screen buffer */
    void *old_palette;          /* Old palette */
    Uint32 old_num_colors;      /* Nb of colors in saved palette */
    int num_modes;              /* Number of xbios video modes */
    xbiosmode_t *mode_list;     /* List of xbios video modes */

    void *screens[2];           /* Pointers to aligned screen buffer */
    void *screensmem[2];        /* Pointers to screen buffer */
    void *shadowscreen;         /* Shadow screen for c2p conversion */
    int doubleline;             /* Double line mode ? */
    int frame_number;           /* Number of frame for double buffer */
    int pitch;                  /* Destination line width for C2P */
    int width, height;          /* Screen size for centered C2P */

    SDL_bool centscreen;        /* Centscreen extension present ? */

    SDL_Rect *SDL_modelist[NUM_MODELISTS][SDL_NUMMODES + 1];
    xbiosmode_t *videomodes[NUM_MODELISTS][SDL_NUMMODES + 1];
#endif
} SDL_VideoData;

/* _VDO cookie values */
enum
{
    VDO_ST = 0,
    VDO_STE,
    VDO_TT,
    VDO_F30
};

/* Monitor types */
enum
{
    MONITOR_MONO = 0,
    MONITOR_TV,
    MONITOR_VGA,
    MONITOR_RGB
};

/* EgetShift masks */
#define ES_BANK		0x000f
#define ES_MODE		0x0700
#define ES_GRAY		0x1000
#define ES_SMEAR	0x8000

/* TT shifter modes */
#define ST_LOW	0x0000
#define ST_MED	0x0100
#define ST_HIGH	0x0200
#define TT_LOW	0x0700
#define TT_MED	0x0300
#define TT_HIGH	0x0600

#endif /* _SDL_xbios_h */
/* vi: set ts=4 sw=4 expandtab: */
