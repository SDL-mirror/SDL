/* Subclass of NSWindow to allow customization if we need it */

@interface SDL_QuartzWindow : NSWindow
{}
- (void)miniaturize:(id)sender;
- (void)deminiaturize:(id)sender;
- (void)display;
@end

@implementation SDL_QuartzWindow

/* These methods should be rewritten to fix the miniaturize bug */
- (void)miniaturize:(id)sender
{
    [ super miniaturize:sender ];
}

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
