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

#ifndef _SDL_riscosvideo_h
#define _SDL_riscosvideo_h

#include "SDL_mouse.h"
#include "SDL_mutex.h"
#include "../SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

/* Information that RISC OS uses to identify/describe pixel formats */
typedef struct {
	int ncolour,modeflags,log2bpp;
	Uint32 sprite_mode_word;
} RISCOS_PixelFormat;

/* RISC OS information combined with SDL information */
typedef struct {
	RISCOS_PixelFormat ro;
	int sdl_bpp;
	Uint32 rmask;
	Uint32 gmask;
	Uint32 bmask;
} RISCOS_SDL_PixelFormat;

extern const RISCOS_SDL_PixelFormat *RISCOS_SDL_PixelFormats; /* Table of known formats */


/* Private display data */

struct SDL_PrivateVideoData {
    unsigned char *bank[2];
    int current_bank;
	unsigned char *alloc_bank;
    int height;
    int xeig;
    int yeig;
	int screen_width;
	int screen_height;
	char *pixtrans;
	int *scale;

	/* Wimp variables */
	unsigned int window_handle;
	char title[256];

    const RISCOS_SDL_PixelFormat *format; /* Format of current SDL display */

#define NUM_MODELISTS	4		/* 8, 16, 24, and 32 bits-per-pixel */
    int SDL_nummodes[NUM_MODELISTS];
    SDL_Rect **SDL_modelist[NUM_MODELISTS];
};

/* Old variable names */
#define SDL_nummodes		(this->hidden->SDL_nummodes)
#define SDL_modelist		(this->hidden->SDL_modelist)

/* SDL_riscosvideo.c */
extern const RISCOS_SDL_PixelFormat *RISCOS_CurrentPixelFormat(); /* Find the current format */
extern int RISCOS_ToggleFullScreen(_THIS, int fullscreen);
extern int RISCOS_GetWmInfo(_THIS, SDL_SysWMinfo *info);

/* SDL_riscossprite.c */
extern int WIMP_IsSupportedSpriteFormat(const RISCOS_PixelFormat *fmt);
extern const RISCOS_SDL_PixelFormat *WIMP_FindSupportedSpriteFormat(int bpp);
extern unsigned char *WIMP_CreateBuffer(int width, int height, const RISCOS_PixelFormat *format);
extern void WIMP_SetupPlotInfo(_THIS);
extern void WIMP_PlotSprite(_THIS, int x, int y);

/* SDL_wimpvideo.c */
extern void WIMP_PumpEvents(_THIS);
extern void WIMP_SetFocus(int win);
extern void WIMP_ReadModeInfo(_THIS);
extern void WIMP_DeleteWindow(_THIS);
extern SDL_Surface *WIMP_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
extern int WIMP_ToggleFromFullScreen(_THIS);

/* SDL_riscosFullScreenVideo.c */
extern void FULLSCREEN_SetDeviceMode(_THIS);
extern const RISCOS_SDL_PixelFormat *FULLSCREEN_SetMode(int width, int height, int bpp);
extern void FULLSCREEN_BuildModeList(_THIS);
extern SDL_Surface *FULLSCREEN_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
extern int FULLSCREEN_ToggleFromWimp(_THIS);

/* SDL_riscosASM.S */
extern void RISCOS_Put32(void *to, int pixels, int pitch, int rows, void *from, int src_skip_bytes); /* Fast assembler copy */

/* SDL_wimppoll.c */
extern int mouseInWindow; /* Mouse is in WIMP window */
extern int hasFocus;
extern void WIMP_Poll(_THIS, int waitTime);


#endif /* _SDL_risosvideo_h */
