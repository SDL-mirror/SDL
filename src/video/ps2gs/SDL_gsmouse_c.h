/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga

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

#include "SDL_gsvideo.h"

/* This is the maximum size of the cursor sprite */
#define CURSOR_W	32
#define CURSOR_H	32
#define CURSOR_W_POW	5	/* 32 = 2^5 */
#define CURSOR_H_POW	5	/* 32 = 2^5 */

/* Functions to be exported */
extern void GS_FreeWMCursor(_THIS, WMcursor *cursor);
extern WMcursor *GS_CreateWMCursor(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);
extern void GS_MoveWMCursor(_THIS, int x, int y);
extern int GS_ShowWMCursor(_THIS, WMcursor *cursor);
