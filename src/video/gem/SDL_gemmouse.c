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
 *	GEM Mouse manager
 *
 *	Patrice Mandin
 */

#include <stdlib.h>

#include <gem.h>

#include "SDL_error.h"
#include "SDL_mouse.h"
#include "SDL_events_c.h"
#include "SDL_cursor_c.h"
#include "SDL_gemmouse_c.h"

/* Defines */

/*#define DEBUG_VIDEO_GEM 1*/

#define MAXCURWIDTH 16
#define MAXCURHEIGHT 16

/* The implementation dependent data for the window manager cursor */
struct WMcursor {
	MFORM *mform_p;
};


void GEM_FreeWMCursor(_THIS, WMcursor *cursor)
{
	if (cursor == NULL)
		return;
	
	graf_mouse(ARROW, NULL);

	if (cursor->mform_p != NULL)
		free(cursor->mform_p);

	free(cursor);
}

WMcursor *GEM_CreateWMCursor(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
	WMcursor *cursor;
	MFORM *new_mform;
	int i;

	/* Check the size */
	if ( (w > MAXCURWIDTH) || (h > MAXCURHEIGHT) ) {
		SDL_SetError("Only cursors of dimension (%dx%d) are allowed",
							MAXCURWIDTH, MAXCURHEIGHT);
		return(NULL);
	}

	/* Allocate the cursor memory */
	cursor = (WMcursor *)malloc(sizeof(WMcursor));
	if ( cursor == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}

	/* Allocate mform */
	new_mform = (MFORM *)malloc(sizeof(MFORM));		
	if (new_mform == NULL) {
		free(cursor);
		SDL_OutOfMemory();
		return(NULL);
	}

	cursor->mform_p = new_mform;

	new_mform->mf_xhot = hot_x;
	new_mform->mf_yhot = hot_y;
	new_mform->mf_nplanes = 1;
	new_mform->mf_fg = 0;
	new_mform->mf_bg = 1;

	for (i=0;i<MAXCURHEIGHT;i++) {
		new_mform->mf_mask[i]=0;
		new_mform->mf_data[i]=0;
	}

	if (w<=8) {
		for (i=0;i<h;i++) {
			new_mform->mf_mask[i]= mask[i]<<8;
			new_mform->mf_data[i]= data[i]<<8;
		}
	} else {
		for (i=0;i<h;i++) {
			new_mform->mf_mask[i]= mask[i<<1]<<8 | mask[(i<<1)+1];
			new_mform->mf_data[i]= data[i<<1]<<8 | data[(i<<1)+1];
		}
	}

#ifdef DEBUG_VIDEO_GEM
	for (i=0; i<h ;i++) {
		printf("sdl:video:gem: cursor, line %d = 0x%04x\n", i, new_mform->mf_mask[i]);
	}

	printf("sdl:video:gem: CreateWMCursor(): done\n");
#endif

	return cursor;
}

int GEM_ShowWMCursor(_THIS, WMcursor *cursor)
{
/*
	if (cursor == NULL) {
		graf_mouse(M_OFF, NULL);
	} else if (cursor->mform_p) {
		graf_mouse(USER_DEF, cursor->mform_p);
	}
*/
#ifdef DEBUG_VIDEO_GEM
	printf("sdl:video:gem: ShowWMCursor(0x%08x)\n", (long) cursor);
#endif

	return 1;
}

#if 0
void GEM_WarpWMCursor(_THIS, Uint16 x, Uint16 y)
{
	/* This seems to work only on AES 3.4 (Falcon) */

	EVNTREC	warpevent;
	
	warpevent.ap_event = APPEVNT_MOUSE; 
	warpevent.ap_value = (x << 16) | y;

	appl_tplay(&warpevent, 1, 1000);
}
#endif

void GEM_CheckMouseMode(_THIS)
{
	/* If the mouse is hidden and input is grabbed, we use relative mode */
	if ( !(SDL_cursorstate & CURSOR_VISIBLE) &&
		(this->input_grab != SDL_GRAB_OFF) &&
             (SDL_GetAppState() & SDL_APPACTIVE) ) {
		GEM_mouse_relative = SDL_TRUE;
	} else {
		GEM_mouse_relative = SDL_FALSE;
	}
}
