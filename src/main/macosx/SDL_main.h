/*   SDL_main.h - main entry point for our Cocoa-ized SDL app
       Darrell Walisser - dwaliss1@purdue.edu

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>

@interface SDL_main : NSObject
{
}
- (IBAction)quit:(id)sender;
- (IBAction)makeFullscreen:(id)sender;
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
@end
