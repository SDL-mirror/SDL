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

#include "SDL_error.h"
#include "SDL_mouse.h"
#include "SDL_events_c.h"
#include "SDL_cursor_c.h"
#include "SDL_amigamouse_c.h"


/* The implementation dependent data for the window manager cursor */

typedef void * WMCursor;

void amiga_FreeWMCursor(_THIS, WMcursor *cursor)
{
}

WMcursor *amiga_CreateWMCursor(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
	return (WMcursor *)1; // Amiga has an Hardware cursor, so it's ok to return something unuseful but true
}

int amiga_ShowWMCursor(_THIS, WMcursor *cursor)
{
	/* Don't do anything if the display is gone */
	if ( SDL_Display == NULL) {
		return(0);
	}

	/* Set the Amiga prefs cursor cursor, or blank if cursor is NULL */

	if ( SDL_Window ) {
		SDL_Lock_EventThread();
		if ( cursor == NULL ) {
			if ( SDL_BlankCursor != NULL ) {
// Hide cursor HERE
				SetPointer(SDL_Window,(UWORD *)SDL_BlankCursor,1,1,0,0);
			}
		} else {
// Show cursor
			ClearPointer(SDL_Window);
		}
		SDL_Unlock_EventThread();
	}
	return(1);
}

void amiga_WarpWMCursor(_THIS, Uint16 x, Uint16 y)
{
/* FIXME: Not implemented */
}

/* Check to see if we need to enter or leave mouse relative mode */
void amiga_CheckMouseMode(_THIS)
{
	/* If the mouse is hidden and input is grabbed, we use relative mode */
#if 0
	SDL_Lock_EventThread();
	if ( !(SDL_cursorstate & CURSOR_VISIBLE) &&
	     (this->input_grab != SDL_GRAB_OFF) ) {
		mouse_relative = 1;
		X11_EnableDGAMouse(this);
		if ( ! (using_dga & DGA_MOUSE) ) {
			char *use_mouse_accel;

			SDL_GetMouseState(&mouse_last.x, &mouse_last.y);
			/* Use as raw mouse mickeys as possible */
			XGetPointerControl(SDL_Display,
						&mouse_accel.numerator, 
						&mouse_accel.denominator,
						&mouse_accel.threshold);
			use_mouse_accel = getenv("SDL_VIDEO_X11_MOUSEACCEL");
			if ( use_mouse_accel ) {
				SetMouseAccel(this, use_mouse_accel);
			}
		}
	} else {
		if ( mouse_relative ) {
			if ( using_dga & DGA_MOUSE ) {
				X11_DisableDGAMouse(this);
			} else {
				XChangePointerControl(SDL_Display, True, True,
						mouse_accel.numerator, 
						mouse_accel.denominator,
						mouse_accel.threshold);
			}
			mouse_relative = 0;
		}
	}
	SDL_Unlock_EventThread();
#endif
}
