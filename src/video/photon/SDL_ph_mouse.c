/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SDL_error.h"
#include "SDL_mouse.h"
#include "SDL_events_c.h"
#include "SDL_cursor_c.h"
#include "SDL_ph_mouse_c.h"

struct  WMcursor {
	PhCursorDef_t *ph_cursor ;
};


void ph_FreeWMCursor(_THIS, WMcursor *cursor)
{

    if ( window != NULL ) {
		SDL_Lock_EventThread();
		
		if (PtSetResource( window, Pt_ARG_CURSOR_TYPE, Ph_CURSOR_INHERIT, 0 ) < 0)
		{
			//TODO: output error msg
		}
		
		SDL_Unlock_EventThread();
	}	
	//free(cursor->ph_cursor.images);
	free(cursor);
}

WMcursor *ph_CreateWMCursor(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
	WMcursor* cursor;
	int clen, i;

	/* Allocate and initialize the cursor memory */
	if ((cursor = (WMcursor*)malloc(sizeof(WMcursor))) == NULL)
	{
        SDL_OutOfMemory();
        return(NULL);
	}
	memset(cursor,0,sizeof(WMcursor));
	
	cursor->ph_cursor = (PhCursorDef_t *) malloc(sizeof(PhCursorDef_t) + 32*4*2);
	if(cursor->ph_cursor == NULL)
	   printf("cursor malloc failed\n");

	memset(cursor->ph_cursor,0,(sizeof(PhCursorDef_t) + 32*4*2));
	   
	cursor->ph_cursor->hdr.type =Ph_RDATA_CURSOR;   
	cursor->ph_cursor->size1.x = (short)w;
	cursor->ph_cursor->size1.y = (short)h;
	cursor->ph_cursor->offset1.x = (short)hot_x;
	cursor->ph_cursor->offset1.y = (short)hot_y;
	cursor->ph_cursor->bytesperline1 = (char)w/8;
	cursor->ph_cursor->color1 = Pg_WHITE;
	cursor->ph_cursor->size2.x = (short)w;
    cursor->ph_cursor->size2.y = (short)h;
    cursor->ph_cursor->offset2.x = (short)hot_x;
    cursor->ph_cursor->offset2.y = (short)hot_y;
    cursor->ph_cursor->bytesperline2 = (char)w/8;
    cursor->ph_cursor->color2 = Pg_BLACK;
      
	clen = (w/8)*h;

	/* Copy the mask and the data to different 
	   bitmap planes */
	for ( i=0; i<clen; ++i ) {
        cursor->ph_cursor->images[i] = data[i];
        cursor->ph_cursor->images[i+clen] = mask[i];
    }
    
    //#bytes following the hdr struct
	cursor->ph_cursor->hdr.len =sizeof(PhCursorDef_t) + clen*2 - sizeof(PhRegionDataHdr_t); 

	return (cursor);
}


PhCursorDef_t ph_GetWMPhCursor(WMcursor *cursor)
{

	return(*cursor->ph_cursor);
}


int ph_ShowWMCursor(_THIS, WMcursor *cursor)
{
	PtArg_t args[3];
	int nargs = 0;
	short cursor_is_defined = 0;



	/* Don't do anything if the display is gone */
 	if ( window == NULL ) {
    	 return(0);
 	}

	/* Set the photon cursor cursor, or blank if cursor is NULL */
	if ( window ) {
		
		if ( cursor != NULL ) {
			PtSetArg( &args[0], Pt_ARG_CURSOR_TYPE, Ph_CURSOR_BITMAP, 0 );
			// Could set next to any PgColor_t value
			PtSetArg( &args[1], Pt_ARG_CURSOR_COLOR,Ph_CURSOR_DEFAULT_COLOR , 0 );
			PtSetArg( &args[2], Pt_ARG_BITMAP_CURSOR, cursor->ph_cursor, (cursor->ph_cursor->hdr.len + sizeof(PhRegionDataHdr_t)) );
			nargs = 3;
			cursor_is_defined = 1;
		}
		else // Ph_CURSOR_NONE
		{
			PtSetArg( &args[0], Pt_ARG_CURSOR_TYPE,Ph_CURSOR_NONE, 0);
			nargs = 1;
			cursor_is_defined = 1;
		}
		if (cursor_is_defined)
		 {
    	    SDL_Lock_EventThread();
			
			if (PtSetResources( window, nargs, args ) < 0 )
			{
			    return(0);
			}	
						
			SDL_Unlock_EventThread();
		}
		else
			return(0);
	}
	return(1);
}

void ph_WarpWMCursor(_THIS, Uint16 x, Uint16 y)
{

	SDL_Lock_EventThread();
	PhMoveCursorRel( PhInputGroup(NULL), x, y );	
	SDL_Unlock_EventThread();
}


void ph_CheckMouseMode(_THIS)
{

	mouse_relative = 1;
}
