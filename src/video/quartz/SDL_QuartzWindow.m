/* Subclass of NSWindow to allow customization if we need it */

@interface SDL_QuartzWindow : NSWindow
{}
- (void)miniaturize:(id)sender;
- (void)deminiaturize:(id)sender;
- (void)display;
@end

/**
 * Function to set the opacity of window's pixels to 100% 
 * The opacity is only used by the window server code that does the minimize effect
 **/
static void QZ_SetPortAlphaOpaque (CGrafPtr port, Uint32 noTitleBar) {
    
    Uint32 *pixels;
    Uint32  rowPixels;
    Uint32  width, height;
    Uint32  bpp;
    PixMapHandle pixMap;
    Rect bounds;
    int i, j;
    
    pixMap = GetPortPixMap ( port );
    bpp = GetPixDepth ( pixMap );
    
    if (bpp == 32) {
    
        GetPortBounds ( port, &bounds );
        width = bounds.right - bounds.left;
        height = bounds.bottom - bounds.top;
        
        LockPortBits (port);
        
        pixels = (Uint32*) GetPixBaseAddr ( pixMap );
        rowPixels = GetPixRowBytes ( pixMap ) / 4;
        
        if (! noTitleBar) {
        
            /* offset for title bar */
            pixels += rowPixels * 22;
        }
            
        for (i = 0; i < height; i++)
            for (j = 0; j < width; j++) {
        
                pixels[ (i * rowPixels) + j ] |= 0xFF000000;
            }
            
        UnlockPortBits (port);
    }
}

@implementation SDL_QuartzWindow

/* override these methods to fix the miniaturize animation/dock icon bug */
- (void)miniaturize:(id)sender
{
    
    if (SDL_VideoSurface->flags & SDL_OPENGL) {
    
        /* Grab framebuffer and put into NSImage */
        /* [ qz_window setMiniwindowImage:image ]; */
    }
    else {
        
        QZ_SetPortAlphaOpaque ([ [ self contentView ] qdPort ], 
                               [ self styleMask ] & NSBorderlessWindowMask);
    }
    
    [ super miniaturize:sender ];
}

/* this routine fires *after* deminiaturizing, so it might be useless to us */
- (void)deminiaturize:(id)sender
{
   [ super deminiaturize:sender ];
}

- (void)display
{
    /* Do nothing to keep pinstripe pattern from drawing */
}
@end

/* Delegate for our NSWindow to send SDLQuit() on close */
@interface SDL_QuartzWindowDelegate : NSObject
{}
- (BOOL)windowShouldClose:(id)sender;
@end

@implementation SDL_QuartzWindowDelegate
- (BOOL)windowShouldClose:(id)sender {

    SDL_PrivateQuit();
    return NO;
}

@end

/* empty class; probably could be used to fix bugs in the future */
@interface SDL_QuartzWindowView : NSQuickDrawView
{}
@end

@implementation SDL_QuartzWindowView

@end