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

#ifndef _SDL_os4video_h
#define _SDL_os4video_h

#include <exec/types.h>
#include <intuition/intuition.h>
#include <libraries/Picasso96.h>
#include <workbench/icon.h>

#include "../SDL_sysvideo.h"

#define _THIS SDL_VideoDevice *_this

/*
 * Data associated with the off-screen buffer for
 * software display surfaces
 */
struct OffScreenBuffer
{
	struct BitMap 		   *bitmap;							/* P96 bitmap holding the actual pixels */
	uint32					width;
	uint32					height;
	SDL_PixelFormat			format;							/* SDL pixel format of this surface */
	void				   *pixels;
	uint32					pitch;
	uint32 					palette[256];					/* Palette for palette-mapped surfaces. Colours are in
															 * the screen's pixel format */
};

/*
 * Resources needed for double-buffering
 */
struct DoubleBufferData
{
	struct ScreenBuffer *	sb[2];							/* Screen buffers when double-buffering */
	uint32 					currentSB;						/* Index of current off-screen buffer */
	BOOL					SafeToFlip;						/* Do we need to wait before flipping screen buffers? */
	BOOL					SafeToWrite;					/* Do we need to wait before writing to the off-screen buffer */
	struct MsgPort		   *SafeToFlip_MsgPort;				/* For receiving SafeToFlip msgs from gfx.lib */
	struct MsgPort		   *SafeToWrite_MsgPort;			/* For receiving SafeToWrite msgs from gfx.lib */
};


typedef enum
{
	pointer_dont_know = -1,
	pointer_inside_window,
	pointer_outside_window
} pointer_state;

/*
 * Identifier describing an SDL surface's P96 bitmap
 */
typedef enum en_hwdata_type
{
	hwdata_other,
	hwdata_display_hw,		/* Screen bitmap used for display surfaces of type SDL_HWSURFACE */
	hwdata_display_sw,		/* Off-screen bitmap used for display surfaces of type SDL_SWSURFACE */
	hwdata_bitmap			/* Non-displayable surface of type SDL_HWSURFACE */
} hwdata_type;

/*
 * Private surface data for SDL surfaces
 */
struct private_hwdata
{
	hwdata_type        type;
	struct BitMap     *bm;
	struct RenderInfo  ri;
	LONG               lock;
};

/*
 * Data private to the OS4 video driver
 */
struct SDL_PrivateVideoData
{
	struct Screen *			publicScreen;
	SDL_Rect **				Modes[RGBFB_MaxFormats];
	struct MsgPort *		userPort;
	struct Screen *			scr;

	struct Window *			win;
	BOOL					windowActive;					/* Are we the active intuition window? */

	APTR 					pool;
	struct SignalSemaphore *poolSemaphore;

	struct private_hwdata	screenHWData;					/* Private surface data for the display surface */

	SDL_Color				currentPalette[256];			/* Actual palette */

	RGBFTYPE				screenP96Format;				/* P96 pixel format of screen */
	SDL_PixelFormat			screenFormat;					/* SDL pixel format of screen */

	struct OffScreenBuffer  offScreenBuffer;				/* Offscreen buffer for software surfaces */

	struct DoubleBufferData	dbData;							/* Resources for double-buffering */

	uint16 *				mouse;
	void *					currentCursor;
	uint32					mouseColors[15];
	uint32					mouseColorsValid;
	BOOL					isMouseRelative;				/* If true, we report delta mouse movements */
	pointer_state			pointerState;
	uint32					pointerGrabTicks;
	char					currentCaption[128];
	char					currentIconCaption[128];
	struct DiskObject *		currentIcon;
	struct AppIcon *        currentAppIcon; 				/* Only valid when iconified */
	struct MsgPort *		appPort;
	BOOL					OpenGL;

	/* This are new to allow the new MiniGL buffer mode */
	BOOL dontdeletecontext;

	struct GLContextIFace *IGL;
	struct BitMap *m_frontBuffer;
	struct BitMap *m_backBuffer;
};

#endif
