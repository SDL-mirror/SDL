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

struct WMcursor {
    Cursor curs;
};

static void QZ_FreeWMCursor     (_THIS, WMcursor *cursor) { 

    if ( cursor != NULL )
        free (cursor);
}

/* Use the Carbon cursor routines for now */
static WMcursor*    QZ_CreateWMCursor   (_THIS, Uint8 *data, Uint8 *mask, 
                                          int w, int h, int hot_x, int hot_y) { 
	WMcursor *cursor;
	int row, bytes;
	cursor = (WMcursor *)malloc(sizeof(WMcursor));
	if ( cursor == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}
	memset(cursor, 0, sizeof(*cursor));
		
	bytes = (w/8);
	if ( bytes > 2 ) {
		bytes = 2;
	}
	for ( row=0; row<h && (row < 16); ++row ) {
		memcpy(&cursor->curs.data[row], data, bytes);
		data += w/8;
	}
	for ( row=0; row<h && (row < 16); ++row ) {
		memcpy(&cursor->curs.mask[row], mask, bytes);
		mask += w/8;
	}
	cursor->curs.hotSpot.h = hot_x;
	cursor->curs.hotSpot.v = hot_y;
	
        return(cursor);
}

static int QZ_ShowWMCursor (_THIS, WMcursor *cursor) { 

    static int visible = 1;
    
    if ( cursor == NULL) {
        if ( visible ) {
            HideCursor ();
            visible = 0;
        }
    }
    else {
        SetCursor(&cursor->curs);
        if ( ! visible ) {
            ShowCursor ();
            visible = 1;
        }
    }

    return 1;
}

static void  QZ_PrivateWarpCursor (_THIS, int fullscreen, int h, int x, int y) {

    CGPoint p;
    
    /* We require absolute screen coordiates for our warp */
    p.x = x;
    p.y = h - y;
        
    if ( fullscreen )
        /* Already absolute coordinates */
        CGDisplayMoveCursorToPoint(display_id, p);
    else {
        /* Convert to absolute screen coordinates */
        NSPoint base, screen;
        base = NSMakePoint (p.x, p.y);
        screen = [ window convertBaseToScreen:base ];
        p.x = screen.x;
        p.y = device_height - screen.y;
        CGDisplayMoveCursorToPoint (display_id, p);
    }
}

static void QZ_WarpWMCursor     (_THIS, Uint16 x, Uint16 y) {
    
    /* Only allow warping when in foreground */
    if ( ! inForeground )
        return;
            
    /* Do the actual warp */
    QZ_PrivateWarpCursor (this, SDL_VideoSurface->flags & SDL_FULLSCREEN, 
        SDL_VideoSurface->h, x, y);
    
    /* Generate mouse moved event */
    SDL_PrivateMouseMotion (SDL_RELEASED, 0, x, y);
}

static void QZ_MoveWMCursor     (_THIS, int x, int y) { }
static void QZ_CheckMouseMode   (_THIS) { }

static void QZ_SetCaption    (_THIS, const char *title, const char *icon) {

    if ( window != nil ) {
        NSString *string;
        if ( title != NULL ) {
            string = [ [ NSString alloc ] initWithCString:title ];
            [ window setTitle:string ];
            [ string release ];
        }
        if ( icon != NULL ) {
            string = [ [ NSString alloc ] initWithCString:icon ];
            [ window setMiniwindowTitle:string ];
            [ string release ];
        }
    }
}

static void QZ_SetIcon       (_THIS, SDL_Surface *icon, Uint8 *mask) {
/* Convert icon/mask to NSImage, assign with NSWindow's setMiniwindowImage method */
}

static int  QZ_IconifyWindow (_THIS) { 

    /* Bug! minimize erases the framebuffer */
    if ( ! [ window isMiniaturized ] ) {
        [ window miniaturize:nil ];
        return 1;
    }
    else {
        SDL_SetError ("window already iconified");
        return 0;
    }
}

/*
static int  QZ_GetWMInfo  (_THIS, SDL_SysWMinfo *info) { 
    info->nsWindowPtr = window;
    return 0; 
}*/

static SDL_GrabMode QZ_GrabInput (_THIS, SDL_GrabMode grab_mode) {

    switch (grab_mode) {
	case SDL_GRAB_QUERY:
            break;
	case SDL_GRAB_OFF:
            CGAssociateMouseAndMouseCursorPosition (1);
            currentGrabMode = SDL_GRAB_OFF;
            break;
	case SDL_GRAB_ON:
            QZ_WarpWMCursor (this, SDL_VideoSurface->w / 2, SDL_VideoSurface->h / 2);
            CGAssociateMouseAndMouseCursorPosition (0);
            currentGrabMode = SDL_GRAB_ON;
            break;
        case SDL_GRAB_FULLSCREEN:
            break;
    }
        
    return currentGrabMode;
}
