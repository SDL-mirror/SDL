
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
    
    [ super miniaturize:sender ];
}

- (void)display
{    
    /* 
        This method fires just before the window deminaturizes.
        So, it's just the right place to fixup the alpha channel - which
        makes the deminiaturize animation look right.
    */
    if ( (SDL_VideoSurface->flags & SDL_OPENGL) == 0)
        QZ_SetPortAlphaOpaque ();
}

- (void)setFrame:(NSRect)frameRect display:(BOOL)flag
{

    /*
        If the video surface is NULL, this originated from QZ_SetVideoMode,
        so don't send the resize event. 
    */
    if (SDL_VideoSurface == NULL) {

        [ super setFrame:frameRect display:flag ];
    }
    else {

        SDL_VideoDevice *this = (SDL_VideoDevice*)current_video;
        
        NSRect sdlRect = [ NSWindow contentRectForFrameRect:frameRect styleMask:[self styleMask] ];

        [ super setFrame:frameRect display:flag ];
        SDL_PrivateResize (sdlRect.size.width, sdlRect.size.height);

        /* If not OpenGL, we have to update the pixels and pitch */
        if ( ! this->screen->flags & SDL_OPENGL ) {
            
            LockPortBits ( [ window_view qdPort ] );
            
            SDL_VideoSurface->pixels = GetPixBaseAddr ( GetPortPixMap ( [ window_view qdPort ] ) );
            SDL_VideoSurface->pitch  = GetPixRowBytes ( GetPortPixMap ( [ window_view qdPort ] ) );
            
            SDL_VideoSurface->pixels += ((int)[ self frame ].size.height - (int)sdlRect.size.height) * SDL_VideoSurface->pitch;
    
            UnlockPortBits ( [ window_view qdPort ] );
        }
    }
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
