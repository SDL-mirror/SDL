
/*
    This function makes the *SDL region* of the window 100% opaque. 
    The genie effect uses the alpha component. Otherwise,
    it doesn't seem to matter what value it has.
*/
static void QZ_SetPortAlphaOpaque () {
    
    SDL_Surface *surface = current_video->screen;
    int bpp;
    
    bpp = surface->format->BitsPerPixel;
    
    if (bpp == 32) {
    
        Uint32    *pixels = (Uint32*) surface->pixels;
        Uint32    rowPixels = surface->pitch / 4;
        Uint32    i, j;
        
        for (i = 0; i < surface->h; i++)
            for (j = 0; j < surface->w; j++) {
        
                pixels[ (i * rowPixels) + j ] |= 0xFF000000;
            }
    }
}

/* Subclass of NSWindow to fix genie effect and support resize events  */
@interface SDL_QuartzWindow : NSWindow
{}
- (void)miniaturize:(id)sender;
- (void)display;
- (void)setFrame:(NSRect)frameRect display:(BOOL)flag;
- (void)appDidHide:(NSNotification*)note;
- (void)appWillUnhide:(NSNotification*)note;
- (void)appDidUnhide:(NSNotification*)note;
- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int)styleMask backing:(NSBackingStoreType)backingType defer:(BOOL)flag;
@end

@implementation SDL_QuartzWindow

/* we override these methods to fix the miniaturize animation/dock icon bug */
- (void)miniaturize:(id)sender
{
    if (SDL_VideoSurface->flags & SDL_OPENGL) {
    
        /* 
            Future: Grab framebuffer and put into NSImage
            [ qz_window setMiniwindowImage:image ];
        */
    }
    else {
        
        /* make the alpha channel opaque so anim won't have holes in it */
        QZ_SetPortAlphaOpaque ();
    }
    
    /* window is hidden now */
    SDL_PrivateAppActive (0, SDL_APPACTIVE);
    
    [ super miniaturize:sender ];
}

- (void)display
{    
    /* 
        This method fires just before the window deminaturizes from the Dock.
        
        We'll save the current visible surface, let the window manager redraw any
        UI elements, and restore the SDL surface. This way, no expose event 
        is required, and the deminiaturize works perfectly.
    */
     SDL_VideoDevice *this = (SDL_VideoDevice*)current_video;
    
    /* make sure pixels are fully opaque */
    if (! ( SDL_VideoSurface->flags & SDL_OPENGL ) )
        QZ_SetPortAlphaOpaque ();
    
    /* save current visible SDL surface */
    [ self cacheImageInRect:[ window_view frame ] ];
    
    /* let the window manager redraw controls, border, etc */
    [ super display ];
    
    /* restore visible SDL surface */
    [ self restoreCachedImage ];
    
    /* window is visible again */
    SDL_PrivateAppActive (1, SDL_APPACTIVE);
}

- (void)setFrame:(NSRect)frameRect display:(BOOL)flag
{

    /*
        If the video surface is NULL, this originated from QZ_SetVideoMode,
        so don't send the resize event. 
    */
    SDL_VideoDevice *this = (SDL_VideoDevice*)current_video;
    
    if (this && SDL_VideoSurface == NULL) {

        [ super setFrame:frameRect display:flag ];
    }
    else if (this && qz_window) {

        NSRect newViewFrame;
        
        [ super setFrame:frameRect display:flag ];
        
        newViewFrame = [ window_view frame ];
        
        SDL_PrivateResize (newViewFrame.size.width, newViewFrame.size.height);

        /* If not OpenGL, we have to update the pixels and pitch */
        if ( ! ( SDL_VideoSurface->flags & SDL_OPENGL ) ) {
            
            CGrafPtr thePort = [ window_view qdPort ];
            LockPortBits ( thePort );
            
            SDL_VideoSurface->pixels = GetPixBaseAddr ( GetPortPixMap ( thePort ) );
            SDL_VideoSurface->pitch  = GetPixRowBytes ( GetPortPixMap ( thePort ) );
                        
            /* 
                SDL_VideoSurface->pixels now points to the window's pixels
                We want it to point to the *view's* pixels 
            */
            { 
                int vOffset = [ qz_window frame ].size.height - 
                    newViewFrame.size.height - newViewFrame.origin.y;
                
                int hOffset = newViewFrame.origin.x;
                        
                SDL_VideoSurface->pixels += (vOffset * SDL_VideoSurface->pitch) + hOffset * (device_bpp/8);
            }
            
            UnlockPortBits ( thePort );
        }
    }
}

- (void)appDidHide:(NSNotification*)note
{
    SDL_PrivateAppActive (0, SDL_APPACTIVE);
}

- (void)appWillUnhide:(NSNotification*)note
{
    SDL_VideoDevice *this = (SDL_VideoDevice*)current_video;
    
    if ( this ) {
    
        /* make sure pixels are fully opaque */
        if (! ( SDL_VideoSurface->flags & SDL_OPENGL ) )
            QZ_SetPortAlphaOpaque ();
          
        /* save current visible SDL surface */
        [ self cacheImageInRect:[ window_view frame ] ];
    }
}

- (void)appDidUnhide:(NSNotification*)note
{
    /* restore cached image, since it may not be current, post expose event too */
    [ self restoreCachedImage ];
    
    //SDL_PrivateExpose ();
    
    SDL_PrivateAppActive (1, SDL_APPACTIVE);
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int)styleMask backing:(NSBackingStoreType)backingType defer:(BOOL)flag
{
    /* Make our window subclass receive these application notifications */
    [ [ NSNotificationCenter defaultCenter ] addObserver:self
        selector:@selector(appDidHide:) name:NSApplicationDidHideNotification object:NSApp ];
    
    [ [ NSNotificationCenter defaultCenter ] addObserver:self
        selector:@selector(appDidUnhide:) name:NSApplicationDidUnhideNotification object:NSApp ];
   
    [ [ NSNotificationCenter defaultCenter ] addObserver:self
        selector:@selector(appWillUnhide:) name:NSApplicationWillUnhideNotification object:NSApp ];
        
    return [ super initWithContentRect:contentRect styleMask:styleMask backing:backingType defer:flag ];
}

@end

/* Delegate for our NSWindow to send SDLQuit() on close */
@interface SDL_QuartzWindowDelegate : NSObject
{}
- (BOOL)windowShouldClose:(id)sender;
@end

@implementation SDL_QuartzWindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
    SDL_PrivateQuit();
    return NO;
}
@end

/* Subclass of NSQuickDrawView for the window's subview */
@interface SDL_QuartzWindowView : NSQuickDrawView
{}
@end

@implementation SDL_QuartzWindowView
@end
