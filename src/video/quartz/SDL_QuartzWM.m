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

static int QZ_cursor_visible = 1;
    
static int QZ_ShowWMCursor (_THIS, WMcursor *cursor) { 

    if ( cursor == NULL) {
        if ( QZ_cursor_visible ) {
            HideCursor ();
            QZ_cursor_visible = 0;
        }
    }
    else {
        SetCursor(&cursor->curs);
        if ( ! QZ_cursor_visible ) {
            ShowCursor ();
            QZ_cursor_visible = 1;
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
static void QZ_PrivateGlobalToLocal (_THIS, NSPoint *p) {

    *p = [ qz_window convertScreenToBase:*p ];
}


/* Convert Cocoa window coordinate to Cocoa screen coordinate */
static void QZ_PrivateLocalToGlobal (_THIS, NSPoint *p) {

    *p = [ qz_window convertBaseToScreen:*p ];
}

/* Convert SDL coordinate to Cocoa coordinate */
static void QZ_PrivateSDLToCocoa (_THIS, NSPoint *p) {

    int height;
    
    if ( CGDisplayIsCaptured (display_id) ) { /* capture signals fullscreen */
    
        height = CGDisplayPixelsHigh (display_id);
    }
    else {
        
        height = NSHeight ( [ qz_window frame ] );
        if ( [ qz_window styleMask ] & NSTitledWindowMask ) {
        
            height -= 22;
        }
    }
    
    p->y = height - p->y;
}

/* Convert Cocoa coordinate to SDL coordinate */
static void QZ_PrivateCocoaToSDL (_THIS, NSPoint *p) {

    QZ_PrivateSDLToCocoa (this, p);
}

/* Convert SDL coordinate to window server (CoreGraphics) coordinate */
static CGPoint QZ_PrivateSDLToCG (_THIS, NSPoint *p) {
    
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

/* Convert window server (CoreGraphics) coordinate to SDL coordinate */
static void QZ_PrivateCGToSDL (_THIS, NSPoint *p) {
            
    if ( ! CGDisplayIsCaptured (display_id) ) { /* not captured => not fullscreen => local coord */
    
        int height;

        /* Convert CG Global to Cocoa Global */
        height = CGDisplayPixelsHigh (display_id);
        p->y = height - p->y;

        QZ_PrivateGlobalToLocal (this, p);
        QZ_PrivateCocoaToSDL (this, p);
    }
}

static void  QZ_PrivateWarpCursor (_THIS, int x, int y) {
    
    NSPoint p;
    CGPoint cgp;
    
    p = NSMakePoint (x, y);
    cgp = QZ_PrivateSDLToCG (this, &p);   
    CGDisplayMoveCursorToPoint (display_id, cgp);
    warp_ticks = SDL_GetTicks();
    warp_flag = 1;

    SDL_PrivateMouseMotion(0, 0, x, y);
}

static void QZ_WarpWMCursor (_THIS, Uint16 x, Uint16 y) {

    /* Only allow warping when in foreground */
    if ( ! in_foreground )
        return;
            
    /* Do the actual warp */
    QZ_PrivateWarpCursor (this, x, y);
}

static void QZ_MoveWMCursor     (_THIS, int x, int y) { }
static void QZ_CheckMouseMode   (_THIS) { }

static void QZ_SetCaption    (_THIS, const char *title, const char *icon) {

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

static void QZ_SetIcon       (_THIS, SDL_Surface *icon, Uint8 *mask)
{
    NSBitmapImageRep *imgrep;
    NSImage *img;
    SDL_Surface *mergedSurface;
    Uint8 *surfPtr;
    int i,j,masksize;
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
    
    if (SDL_BlitSurface(icon,&rrect,mergedSurface,&rrect)) {
        NSLog(@"Error blitting to mergedSurface");
        goto freePool;
    }
    
    if (mask) {
        masksize=icon->w*icon->h;
        surfPtr = (Uint8 *)mergedSurface->pixels;
        #define ALPHASHIFT 3
        for (i=0;i<masksize;i+=8)
            for (j=0;j<8;j++) 
                surfPtr[ALPHASHIFT+((i+j)<<2)]=(mask[i>>3]&(1<<(7-j)))?0xFF:0x00;
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

static int  QZ_IconifyWindow (_THIS) { 

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
static int  QZ_GetWMInfo  (_THIS, SDL_SysWMinfo *info) { 
    info->nsWindowPtr = qz_window;
    return 0; 
}*/

static SDL_GrabMode QZ_GrabInput (_THIS, SDL_GrabMode grab_mode) {

    switch (grab_mode) {
    case SDL_GRAB_QUERY:
            break;
    case SDL_GRAB_OFF:
            CGAssociateMouseAndMouseCursorPosition (1);
            current_grab_mode = SDL_GRAB_OFF;
            break;
    case SDL_GRAB_ON:
            QZ_WarpWMCursor (this, SDL_VideoSurface->w / 2, SDL_VideoSurface->h / 2);
            CGAssociateMouseAndMouseCursorPosition (0);
            current_grab_mode = SDL_GRAB_ON;
            break;
    case SDL_GRAB_FULLSCREEN:        
            break;
    }
        
    return current_grab_mode;
}

/* Resize icon, BMP format */
static unsigned char QZ_ResizeIcon[] = {
    0x42,0x4d,0x31,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x0d,0x00,0x00,0x00,0x0d,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
    0x00,0x00,0xfb,0x01,0x00,0x00,0x13,0x0b,0x00,0x00,0x13,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0b,0xff,0xff,
    0xff,0xda,0xda,0xda,0x87,0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xda,0xda,0xda,
    0x87,0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xda,0xda,0xda,0x87,0x87,0x87,0xe8,
    0xe8,0xe8,0xff,0xff,0xff,0x0b,0xff,0xff,0xff,0xff,0xff,0xff,0xda,0xda,0xda,0x87,
    0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xda,0xda,0xda,0x87,0x87,0x87,0xe8,0xe8,
    0xe8,0xff,0xff,0xff,0xda,0xda,0xda,0x87,0x87,0x87,0xff,0xff,0xff,0x0b,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xd5,0xd5,0xd5,0x87,0x87,0x87,0xe8,0xe8,0xe8,
    0xff,0xff,0xff,0xda,0xda,0xda,0x87,0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xda,
    0xda,0xda,0xff,0xff,0xff,0x0b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xd7,0xd7,0xd7,0x87,0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xda,0xda,
    0xda,0x87,0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xff,0xff,0xff,0x0b,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xd7,0xd7,0xd7,
    0x87,0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xda,0xda,0xda,0x87,0x87,0x87,0xe8,
    0xe8,0xe8,0xff,0xff,0xff,0x0b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xd7,0xd7,0xd7,0x87,0x87,0x87,0xe8,0xe8,
    0xe8,0xff,0xff,0xff,0xdc,0xdc,0xdc,0x87,0x87,0x87,0xff,0xff,0xff,0x0b,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xd9,0xd9,0xd9,0x87,0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xdc,
    0xdc,0xdc,0xff,0xff,0xff,0x0b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xdb,0xdb,
    0xdb,0x87,0x87,0x87,0xe8,0xe8,0xe8,0xff,0xff,0xff,0xff,0xff,0xff,0x0b,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xdb,0xdb,0xdb,0x87,0x87,0x87,0xe8,
    0xe8,0xe8,0xff,0xff,0xff,0x0b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xdc,0xdc,0xdc,0x87,0x87,0x87,0xff,0xff,0xff,0x0b,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xdc,
    0xdc,0xdc,0xff,0xff,0xff,0x0b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0b
};

static void QZ_DrawResizeIcon (_THIS, RgnHandle dirtyRegion) {

    /* Check if we should draw the resize icon */
    if (SDL_VideoSurface->flags & SDL_RESIZABLE) {
    
        Rect	icon;
        SetRect (&icon, SDL_VideoSurface->w - 13, SDL_VideoSurface->h - 13, 
                    SDL_VideoSurface->w, SDL_VideoSurface->h);
                    
        if (RectInRgn (&icon, dirtyRegion)) {
        
            SDL_Rect icon_rect;
            
            /* Create the icon image */
            if (resize_icon == NULL) {
            
                SDL_RWops *rw;
                SDL_Surface *tmp;
                
                rw = SDL_RWFromMem (QZ_ResizeIcon, sizeof(QZ_ResizeIcon));
                tmp = SDL_LoadBMP_RW (rw, SDL_TRUE);
                                                                
                resize_icon = SDL_ConvertSurface (tmp, SDL_VideoSurface->format, SDL_SRCCOLORKEY);								
                SDL_SetColorKey (resize_icon, SDL_SRCCOLORKEY, 0xFFFFFF);
                
                SDL_FreeSurface (tmp);
            }
            
            icon_rect.x = SDL_VideoSurface->w - 13;
            icon_rect.y = SDL_VideoSurface->h - 13;
            icon_rect.w = 13;
            icon_rect.h = 13;
            
            SDL_BlitSurface (resize_icon, NULL, SDL_VideoSurface, &icon_rect);
        }
    }
}