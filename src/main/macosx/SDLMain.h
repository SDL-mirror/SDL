/*   SDLMain.h - main entry point for our Cocoa-ized SDL app
       Darrell Walisser - dwaliss1@purdue.edu

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>

@interface SDLMain : NSObject
{
}
- (IBAction)quit:(id)sender;
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
@end
