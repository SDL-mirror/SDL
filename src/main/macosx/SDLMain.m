/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Darrell Walisser - dwaliss1@purdue.edu

    Feel free to customize this file to suit your needs
*/

#import "SDL.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

static int    gArgc;
static char  **gArgv;

/* The main class of the application, the application's delegate */
@implementation SDLMain

/* Invoked from the Quit menu item */
- (void) quit:(id)sender
{
    	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

/* Invoked from the "Make fulllscreen" menu item */
- (void) makeFullscreen:(id)sender
{
    
}

/* Set the working directory to the .app's parent directory */
- (void) setupWorkingDirectory
{
    char parentdir[MAXPATHLEN];
    char *c;
    
    strncpy ( parentdir, gArgv[0], MAXPATHLEN );
    c = (char*) parentdir;
    
    while (*c != '\0')     /* go to end */
        c++;
    
    while (*c != '/')      /* back up to parent */
        c--;
    
    *c = '\0';             /* cut off last part (binary name) */
    
    assert ( chdir (parentdir) == 0 );   /* chdir to the binary app's parent */
    assert ( chdir ("../../../") == 0 ); /* chdir to the .app's parent */
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    /* Set the working directory to the .app's parent directory */
    [ self setupWorkingDirectory ];
    
    /* This is passed if we are launched by double-clicking */
    if ( gArgc >= 2 && strncmp (gArgv[1], "-psn", 4) == 0 )
        gArgc = 1;
    
    /* Hand off to main application code */
    SDL_main (gArgc, gArgv);
    exit(0);
}
@end

#ifdef main
#  undef main
#endif

/* Main entry point to executible - should *not* be SDL_main! */
int main (int argc, char **argv) {

    /* Copy the arguments into a global variable */
    int i;
    
    gArgc = argc;
    gArgv = (char**) malloc (sizeof(*gArgv) * gArgc);
    assert (gArgv != NULL);
    for (i = 0; i < gArgc; i++) {
        gArgv[i] = strdup (argv[i]);
    }
    
    NSApplicationMain (argc, argv);
    return 0;
}