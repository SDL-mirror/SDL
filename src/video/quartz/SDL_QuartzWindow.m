/* Subclass of NSWindow to allow customization if we need it */

@interface SDL_QuartzWindow : NSWindow
{}
- (void)miniaturize:(id)sender;
- (void)deminiaturize:(id)sender;
@end

@implementation SDL_QuartzWindow

/* These methods should be rewritten to fix the miniaturize bug */
- (void)miniaturize:(id)sender
{
    [ super miniaturize:sender ];
}

- (void)deminiaturize:(id)sender
{
    /* Let the app know they have to redraw everything */
    SDL_PrivateExpose ();
    
    [ super deminiaturize:sender ];
}

@end
