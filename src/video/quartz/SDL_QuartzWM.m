/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2003  Sam Lantinga

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

#include "SDL_QuartzVideo.h"


struct WMcursor {
    Cursor curs;
};

void QZ_FreeWMCursor     (_THIS, WMcursor *cursor) { 

    if ( cursor != NULL )
        free (cursor);
}

/* Use the Carbon cursor routines for now */
WMcursor*    QZ_CreateWMCursor   (_THIS, Uint8 *data, Uint8 *mask, 
                                         int w, int h, int hot_x, int hot_y) { 
    WMcursor *cursor;
    int row, bytes;
        
    /* Allocate the cursor memory */
    cursor = (WMcursor *)malloc(sizeof(WMcursor));
    if ( cursor == NULL ) {
        SDL_OutOfMemory();
        return(NULL);
    }
    memset(cursor, 0, sizeof(*cursor));
    
    if (w > 16)
        w = 16;
    
    if (h > 16)
        h = 16;
    
    bytes = (w+7)/8;

    for ( row=0; row<h; ++row ) {
        memcpy(&cursor->curs.data[row], data, bytes);
        data += bytes;
    }
    for ( row=0; row<h; ++row ) {
        memcpy(&cursor->curs.mask[row], mask, bytes);
        mask += bytes;
    }
    cursor->curs.hotSpot.h = hot_x;
    cursor->curs.hotSpot.v = hot_y;
    
    return(cursor);
}

void QZ_ShowMouse (_THIS) {
    if (!cursor_visible) {
        [ NSCursor unhide ];
        cursor_visible = YES;
    }
}

void QZ_HideMouse (_THIS) {
    BOOL isInGameWin = QZ_IsMouseInWindow (this);
    if (isInGameWin && cursor_visible) {
        [ NSCursor hide ];
        cursor_visible = NO;
    }
}

BOOL QZ_IsMouseInWindow (_THIS) {
    return (mode_flags & SDL_FULLSCREEN) ? true : NSPointInRect([ qz_window mouseLocationOutsideOfEventStream ], [ window_view frame ]);
}

int QZ_ShowWMCursor (_THIS, WMcursor *cursor) { 

    if ( cursor == NULL) {
        if ( cursor_should_be_visible ) {
            QZ_HideMouse (this);
            cursor_should_be_visible = NO;
            QZ_ChangeGrabState (this, QZ_HIDECURSOR);
        }
    }
    else {
        SetCursor(&cursor->curs);
        if ( ! cursor_should_be_visible ) {
            QZ_ShowMouse (this);
            cursor_should_be_visible = YES;
            QZ_ChangeGrabState (this, QZ_SHOWCURSOR);
        }
    }

    return 1;
}

/*
    Coordinate conversion functions, for convenience
    Cocoa sets the origin at the lower left corner of the window/screen
    SDL, CoreGraphics/WindowServer, and QuickDraw use the origin at the upper left corner
    The routines were written so they could be called before SetVideoMode() has finished;
    this might have limited usefulness at the moment, but the extra cost is trivial.
*/

/* Convert Cocoa screen coordinate to Cocoa window coordinate */
void QZ_PrivateGlobalToLocal (_THIS, NSPoint *p) {

    *p = [ qz_window convertScreenToBase:*p ];
}


/* Convert Cocoa window coordinate to Cocoa screen coordinate */
void QZ_PrivateLocalToGlobal (_THIS, NSPoint *p) {

    *p = [ qz_window convertBaseToScreen:*p ];
}

/* Convert SDL coordinate to Cocoa coordinate */
void QZ_PrivateSDLToCocoa (_THIS, NSPoint *p) {

    if ( CGDisplayIsCaptured (display_id) ) { /* capture signals fullscreen */
    
        p->y = CGDisplayPixelsHigh (display_id) - p->y - 1;
    }
    else {
        
        NSPoint newPoint;
        
        newPoint = [ window_view convertPoint:*p toView:[ qz_window contentView ] ];
        
        *p = newPoint;
    }
}

/* Convert Cocoa coordinate to SDL coordinate */
void QZ_PrivateCocoaToSDL (_THIS, NSPoint *p) {

    if ( CGDisplayIsCaptured (display_id) ) { /* capture signals fullscreen */
    
        p->y = CGDisplayPixelsHigh (display_id) - p->y - 1;
    }
    else {
        
        NSPoint newPoint;
        
        newPoint = [ window_view convertPoint:*p fromView:[ qz_window contentView ] ];
        
        *p = newPoint;
    }
}

/* Convert SDL coordinate to window server (CoreGraphics) coordinate */
CGPoint QZ_PrivateSDLToCG (_THIS, NSPoint *p) {
    
    CGPoint cgp;
    
    if ( ! CGDisplayIsCaptured (display_id) ) { /* not captured => not fullscreen => local coord */
    
        int height;
        
        QZ_PrivateSDLToCocoa (this, p);
        QZ_PrivateLocalToGlobal (this, p);
        
        height = CGDisplayPixelsHigh (display_id);
        p->y = height - p->y;
    }
    
    cgp.x = p->x;
    cgp.y = p->y;
    
    return cgp;
}

#if 0 /* Dead code */
/* Convert window server (CoreGraphics) coordinate to SDL coordinate */
void QZ_PrivateCGToSDL (_THIS, NSPoint *p) {
            
    if ( ! CGDisplayIsCaptured (display_id) ) { /* not captured => not fullscreen => local coord */
    
        int height;

        /* Convert CG Global to Cocoa Global */
        height = CGDisplayPixelsHigh (display_id);
        p->y = height - p->y;

        QZ_PrivateGlobalToLocal (this, p);
        QZ_PrivateCocoaToSDL (this, p);
    }
}
#endif /* Dead code */

void  QZ_PrivateWarpCursor (_THIS, int x, int y) {
    
    NSPoint p;
    CGPoint cgp;
    
    p = NSMakePoint (x, y);
    cgp = QZ_PrivateSDLToCG (this, &p);
    
    /* this is the magic call that fixes cursor "freezing" after warp */
    CGSetLocalEventsSuppressionInterval (0.0);
    CGWarpMouseCursorPosition (cgp);
}

void QZ_WarpWMCursor (_THIS, Uint16 x, Uint16 y) {

    /* Only allow warping when in foreground */
    if ( ! [ NSApp isActive ] )
        return;
            
    /* Do the actual warp */
    QZ_PrivateWarpCursor (this, x, y);

    /* Generate the mouse moved event */
    SDL_PrivateMouseMotion (0, 0, x, y);
}

void QZ_MoveWMCursor     (_THIS, int x, int y) { }
void QZ_CheckMouseMode   (_THIS) { }

void QZ_SetCaption    (_THIS, const char *title, const char *icon) {

    if ( qz_window != nil ) {
        NSString *string;
        if ( title != NULL ) {
            string = [ [ NSString alloc ] initWithCString:title ];
            [ qz_window setTitle:string ];
            [ string release ];
        }
        if ( icon != NULL ) {
            string = [ [ NSString alloc ] initWithCString:icon ];
            [ qz_window setMiniwindowTitle:string ];
            [ string release ];
        }
    }
}

void QZ_SetIcon       (_THIS, SDL_Surface *icon, Uint8 *mask)
{
    NSBitmapImageRep *imgrep;
    NSImage *img;
    SDL_Surface *mergedSurface;
    int i,j;
    NSAutoreleasePool *pool;
    SDL_Rect rrect;
    NSSize imgSize = {icon->w, icon->h};
    
    pool = [ [ NSAutoreleasePool alloc ] init ];
    SDL_GetClipRect(icon, &rrect);
    
    /* create a big endian RGBA surface */
    mergedSurface = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA, 
                    icon->w, icon->h, 32, 0xff<<24, 0xff<<16, 0xff<<8, 0xff<<0);
    if (mergedSurface==NULL) {
        NSLog(@"Error creating surface for merge");
        goto freePool;
    }
    
    if (mergedSurface->pitch != 
        mergedSurface->format->BytesPerPixel * mergedSurface->w) {
        SDL_SetError ("merged surface has wrong format");
        SDL_FreeSurface (mergedSurface);
        goto freePool;
    }
    
    if (SDL_BlitSurface(icon,&rrect,mergedSurface,&rrect)) {
        NSLog(@"Error blitting to mergedSurface");
        goto freePool;
    }
    
    if (mask) {

        Uint32 *pixels = mergedSurface->pixels;
        for (i = 0; i < mergedSurface->h; i++) {
            for (j = 0; j < mergedSurface->w; j++) {
                
                int index = i * mergedSurface->w + j;
                int mindex = index >> 3;
                int bindex = 7 - (index & 0x7);
                
                if (mask[mindex] & (1 << bindex))
                    pixels[index] |= 0x000000FF;
                else
                    pixels[index] &= 0xFFFFFF00;
            }
        }
    }
    
    imgrep = [ [ NSBitmapImageRep alloc] 
                    initWithBitmapDataPlanes:(unsigned char **)&mergedSurface->pixels 
                        pixelsWide:icon->w pixelsHigh:icon->h bitsPerSample:8 samplesPerPixel:4 
                        hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace 
                        bytesPerRow:icon->w<<2 bitsPerPixel:32 ];
    
    img = [ [ NSImage alloc ] initWithSize:imgSize ];
    
    [ img addRepresentation: imgrep ];
    [ NSApp setApplicationIconImage:img ];
    
    [ img release ];
    [ imgrep release ];
    SDL_FreeSurface(mergedSurface);
freePool:
    [pool release];
}

int  QZ_IconifyWindow (_THIS) { 

    if ( ! [ qz_window isMiniaturized ] ) {
        [ qz_window miniaturize:nil ];
        return 1;
    }
    else {
        SDL_SetError ("window already iconified");
        return 0;
    }
}

/*
int  QZ_GetWMInfo  (_THIS, SDL_SysWMinfo *info) { 
    info->nsWindowPtr = qz_window;
    return 0; 
}*/

void QZ_ChangeGrabState (_THIS, int action) {

    /* 
        Figure out what the next state should be based on the action.
        Ignore actions that can't change the current state.
    */
    if ( grab_state == QZ_UNGRABBED ) {
        if ( action == QZ_ENABLE_GRAB ) {
            if ( cursor_should_be_visible )
                grab_state = QZ_VISIBLE_GRAB;
            else
                grab_state = QZ_INVISIBLE_GRAB;
        }
    }
    else if ( grab_state == QZ_VISIBLE_GRAB ) {
        if ( action == QZ_DISABLE_GRAB )
            grab_state = QZ_UNGRABBED;
        else if ( action == QZ_HIDECURSOR )
            grab_state = QZ_INVISIBLE_GRAB;
    }
    else {
        assert( grab_state == QZ_INVISIBLE_GRAB );
        
        if ( action == QZ_DISABLE_GRAB )
            grab_state = QZ_UNGRABBED;
        else if ( action == QZ_SHOWCURSOR )
            grab_state = QZ_VISIBLE_GRAB;
    }
    
    /* now apply the new state */
    if (grab_state == QZ_UNGRABBED) {
    
        CGAssociateMouseAndMouseCursorPosition (1);
    }
    else if (grab_state == QZ_VISIBLE_GRAB) {
    
        CGAssociateMouseAndMouseCursorPosition (1);
    }
    else {
        assert( grab_state == QZ_INVISIBLE_GRAB );

        QZ_PrivateWarpCursor (this, SDL_VideoSurface->w / 2, SDL_VideoSurface->h / 2);
        CGAssociateMouseAndMouseCursorPosition (0);
    }
}

SDL_GrabMode QZ_GrabInput (_THIS, SDL_GrabMode grab_mode) {

    int doGrab = grab_mode & SDL_GRAB_ON;
    /*int fullscreen = grab_mode & SDL_GRAB_FULLSCREEN;*/

    if ( this->screen == NULL ) {
        SDL_SetError ("QZ_GrabInput: screen is NULL");
        return SDL_GRAB_OFF;
    }
        
    if ( ! video_set ) {
        /*SDL_SetError ("QZ_GrabInput: video is not set, grab will take effect on mode switch"); */
        current_grab_mode = grab_mode;
        return grab_mode;       /* Will be set later on mode switch */
    }

    if ( grab_mode != SDL_GRAB_QUERY ) {
        if ( doGrab )
            QZ_ChangeGrabState (this, QZ_ENABLE_GRAB);
        else
            QZ_ChangeGrabState (this, QZ_DISABLE_GRAB);
        
        current_grab_mode = doGrab ? SDL_GRAB_ON : SDL_GRAB_OFF;
    }

    return current_grab_mode;
}
