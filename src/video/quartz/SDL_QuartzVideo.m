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

#include "SDL_QuartzVideo.h"

/* Some variables to share among files, put in device structure eventually */
static SDL_GrabMode currentGrabMode = SDL_GRAB_OFF;
static BOOL   inForeground = YES;
static char QZ_Error[255]; /* Global error buffer to temporarily store more informative error messages */

/* Include files into one compile unit...break apart eventually */
#include "SDL_QuartzWM.m"
#include "SDL_QuartzEvents.m"
#include "SDL_QuartzWindow.m"


/* Bootstrap binding, enables entry point into the driver */
VideoBootStrap QZ_bootstrap = {
    "Quartz", "Mac OS X CoreGraphics", QZ_Available, QZ_CreateDevice
};

/* Bootstrap functions */
static int QZ_Available () {
    return 1;
}

static SDL_VideoDevice* QZ_CreateDevice (int device_index) {

#pragma unused (device_index)

    SDL_VideoDevice *device;
    SDL_PrivateVideoData *hidden;

    device = (SDL_VideoDevice*) malloc (sizeof (*device) );
    hidden = (SDL_PrivateVideoData*) malloc (sizeof (*hidden) );

    if (device == NULL || hidden == NULL)
        SDL_OutOfMemory ();

    memset (device, 0, sizeof (*device) );
    memset (hidden, 0, sizeof (*hidden) );

    device->hidden = hidden;

    device->VideoInit        = QZ_VideoInit;
    device->ListModes        = QZ_ListModes;
    device->SetVideoMode     = QZ_SetVideoMode;
    device->ToggleFullScreen = QZ_ToggleFullScreen;
    device->SetColors        = QZ_SetColors;
    /* device->UpdateRects      = QZ_UpdateRects; this is determined by SetVideoMode() */
    device->VideoQuit        = QZ_VideoQuit;

    device->LockHWSurface   = QZ_LockHWSurface;
    device->UnlockHWSurface = QZ_UnlockHWSurface;
    device->FreeHWSurface   = QZ_FreeHWSurface;
    /* device->FlipHWSurface   = QZ_FlipHWSurface */;

    device->SetGamma     = QZ_SetGamma;
    device->GetGamma     = QZ_GetGamma;
    device->SetGammaRamp = QZ_SetGammaRamp;
    device->GetGammaRamp = QZ_GetGammaRamp;

    device->GL_GetProcAddress = QZ_GL_GetProcAddress;
    device->GL_GetAttribute   = QZ_GL_GetAttribute;
    device->GL_MakeCurrent    = QZ_GL_MakeCurrent;
    device->GL_SwapBuffers    = QZ_GL_SwapBuffers;
    device->GL_LoadLibrary    = QZ_GL_LoadLibrary;

    device->FreeWMCursor   = QZ_FreeWMCursor;
    device->CreateWMCursor = QZ_CreateWMCursor;
    device->ShowWMCursor   = QZ_ShowWMCursor;
    device->WarpWMCursor   = QZ_WarpWMCursor;
    device->MoveWMCursor   = QZ_MoveWMCursor;
    device->CheckMouseMode = QZ_CheckMouseMode;
    device->InitOSKeymap   = QZ_InitOSKeymap;
    device->PumpEvents     = QZ_PumpEvents;

    device->SetCaption    = QZ_SetCaption;
    device->SetIcon       = QZ_SetIcon;
    device->IconifyWindow = QZ_IconifyWindow;
    /*device->GetWMInfo     = QZ_GetWMInfo;*/
    device->GrabInput     = QZ_GrabInput;

    device->free = QZ_DeleteDevice;

    return device;
}

static void QZ_DeleteDevice (SDL_VideoDevice *device) {

    free (device->hidden);
    free (device);
}

static int QZ_VideoInit (_THIS, SDL_PixelFormat *video_format) {

    /* Initialize the video settings; this data persists between mode switches */
    display_id = kCGDirectMainDisplay;
    save_mode  = CGDisplayCurrentMode    (display_id);
    mode_list  = CGDisplayAvailableModes (display_id);
    palette    = CGPaletteCreateDefaultColorPalette ();

    /* Gather some information that is useful to know about the display */
    CFNumberGetValue (CFDictionaryGetValue (save_mode, kCGDisplayBitsPerPixel),
                      kCFNumberSInt32Type, &device_bpp);

    CFNumberGetValue (CFDictionaryGetValue (save_mode, kCGDisplayWidth),
                      kCFNumberSInt32Type, &device_width);

    CFNumberGetValue (CFDictionaryGetValue (save_mode, kCGDisplayHeight),
                      kCFNumberSInt32Type, &device_height);

    video_format->BitsPerPixel = device_bpp;

    return 0;
}

static SDL_Rect** QZ_ListModes (_THIS, SDL_PixelFormat *format, Uint32 flags) {
    
    CFIndex num_modes;
    CFIndex i;

    static SDL_Rect **list = NULL;
    int list_size = 0;
    
    /* Any windowed mode is acceptable */
    if ( (flags & SDL_FULLSCREEN) == 0 )
        return (SDL_Rect**)-1;
        
    /* Free memory from previous call, if any */
    if ( list != NULL ) {

      int i;

      for (i = 0; list[i] != NULL; i++)
	free (list[i]);

      free (list);
      list = NULL;
    }
    
    num_modes = CFArrayGetCount (mode_list);

    /* Build list of modes with the requested bpp */
    for (i = 0; i < num_modes; i++) {
   
        CFDictionaryRef onemode;
        CFNumberRef     number;
	int bpp;
	
        onemode = CFArrayGetValueAtIndex (mode_list, i); 
	number = CFDictionaryGetValue (onemode, kCGDisplayBitsPerPixel);
	CFNumberGetValue (number, kCFNumberSInt32Type, &bpp);

	if (bpp == format->BitsPerPixel) {
	  
	  int intvalue;
          int hasMode;
          int width, height;
          
	  number = CFDictionaryGetValue (onemode, kCGDisplayWidth);
	  CFNumberGetValue (number, kCFNumberSInt32Type, &intvalue);
	  width = (Uint16) intvalue;
	  
	  number = CFDictionaryGetValue (onemode, kCGDisplayHeight);
	  CFNumberGetValue (number, kCFNumberSInt32Type, &intvalue);
	  height = (Uint16) intvalue;
          
          /* Check if mode is already in the list */
          {
            int i;
            hasMode = SDL_FALSE;
            for (i = 0; i < list_size; i++) {
                if (list[i]->w == width && list[i]->h == height) {
                    hasMode = SDL_TRUE;
                    break;
                }
            }
          }
          
          /* Grow the list and add mode to the list */
          if ( ! hasMode ) {
	  
            SDL_Rect *rect;
            
            list_size++;

            if (list == NULL)
                list = (SDL_Rect**) malloc (sizeof(*list) * (list_size+1) );
            else
                list = (SDL_Rect**) realloc (list, sizeof(*list) * (list_size+1));
            
            rect = (SDL_Rect*) malloc (sizeof(**list));
            
            if (list == NULL || rect == NULL) {
                SDL_OutOfMemory ();
                return NULL;
            }
            
            rect->w = width;
            rect->h = height;
    
            list[list_size-1] = rect;
            list[list_size]   = NULL;
        }
      }
    }
    
    /* Sort list largest to smallest (by area) */
    {
        int i, j;
        for (i = 0; i < list_size; i++) {
            for (j = 0; j < list_size-1; j++) {
            
                int area1, area2;
                area1 = list[j]->w * list[j]->h;
                area2 = list[j+1]->w * list[j+1]->h;
                
                if (area1 < area2) {
                    SDL_Rect *tmp = list[j];
                    list[j] = list[j+1];
                    list[j+1] = tmp;
                }
            }
        }
    }
    return list;
}

/* Gamma functions to try to hide the flash from a rez switch */
/* Fade the display from normal to black */
/* Save gamma tables for fade back to normal */
static UInt32 QZ_FadeGammaOut (_THIS, SDL_QuartzGammaTable *table) {

    CGGammaValue redTable[QZ_GAMMA_TABLE_SIZE],
    greenTable[QZ_GAMMA_TABLE_SIZE],
    blueTable[QZ_GAMMA_TABLE_SIZE];

    float percent;
    int j;
    int actual;

    if ( (CGDisplayNoErr != CGGetDisplayTransferByTable
          (display_id, QZ_GAMMA_TABLE_SIZE,
           table->red, table->green, table->blue, &actual)) ||
         actual != QZ_GAMMA_TABLE_SIZE) {

        return 1;
    }

    memcpy (redTable, table->red, sizeof(redTable));
    memcpy (greenTable, table->green, sizeof(greenTable));
    memcpy (blueTable, table->blue, sizeof(greenTable));

    for (percent = 1.0; percent >= 0.0; percent -= 0.01) {

        for (j = 0; j < QZ_GAMMA_TABLE_SIZE; j++) {

            redTable[j]   = redTable[j]   * percent;
            greenTable[j] = greenTable[j] * percent;
            blueTable[j]  = blueTable[j]  * percent;
        }

        if (CGDisplayNoErr != CGSetDisplayTransferByTable
            (display_id, QZ_GAMMA_TABLE_SIZE,
             redTable, greenTable, blueTable)) {

            CGDisplayRestoreColorSyncSettings();
            return 1;
        }

        SDL_Delay (10);
    }

    return 0;
}

/* Fade the display from black to normal */
/* Restore previously saved gamma values */
static UInt32 QZ_FadeGammaIn (_THIS, SDL_QuartzGammaTable *table) {

    CGGammaValue redTable[QZ_GAMMA_TABLE_SIZE],
        greenTable[QZ_GAMMA_TABLE_SIZE],
        blueTable[QZ_GAMMA_TABLE_SIZE];

    float percent;
    int j;

    memset (redTable, 0, sizeof(redTable));
    memset (greenTable, 0, sizeof(greenTable));
    memset (blueTable, 0, sizeof(greenTable));
    
    for (percent = 0.0; percent <= 1.0; percent += 0.01) {

        for (j = 0; j < QZ_GAMMA_TABLE_SIZE; j++) {

            redTable[j]   = table->red[j]   * percent;
            greenTable[j] = table->green[j] * percent;
            blueTable[j]  = table->blue[j]  * percent;
        }

        if (CGDisplayNoErr != CGSetDisplayTransferByTable
            (display_id, QZ_GAMMA_TABLE_SIZE,
             redTable, greenTable, blueTable)) {

            CGDisplayRestoreColorSyncSettings();
            return 1;
        }

        SDL_Delay (10);
    }

    return 0;
}

static void QZ_UnsetVideoMode (_THIS) {

    /* Reset values that may change between switches */
    this->info.blit_fill = 0;
    this->FillHWRect     = NULL;
    this->UpdateRects    = NULL;
   
    /* Release fullscreen resources */
    if ( mode_flags & SDL_FULLSCREEN ) {

        SDL_QuartzGammaTable gamma_table;
        int gamma_error;

        gamma_error = QZ_FadeGammaOut (this, &gamma_table);

        /* Release the OpenGL context */
        /* Do this first to avoid trash on the display before fade */
        if ( mode_flags & SDL_OPENGL )
            QZ_TearDownOpenGL (this);
        
        if (mode_flags & SDL_OPENGL)
            CGLSetFullScreen(NULL);

        /* Restore original screen resolution/bpp */
        CGDisplaySwitchToMode (display_id, save_mode);
        CGDisplayRelease (display_id);
        ShowMenuBar ();
        
        if (! gamma_error)
            QZ_FadeGammaIn (this, &gamma_table);
    }
    /* Release window mode resources */
    else { 
        if ( (mode_flags & SDL_OPENGL) == 0 ) {
            UnlockPortBits ( [ window_view qdPort ] );
            [ window_view release  ];
        }
        [ qz_window setContentView:nil ];
        [ qz_window setDelegate:nil ];
        [ qz_window close ];
        [ qz_window release ];

        /* Release the OpenGL context */
        if ( mode_flags & SDL_OPENGL )
            QZ_TearDownOpenGL (this);
    }

    /* Restore gamma settings */
    CGDisplayRestoreColorSyncSettings ();
    
    /* Set pixels to null (so other code doesn't try to free it) */
    if (this->screen != NULL)
        this->screen->pixels = NULL;
        
    /* Ensure the cursor will be visible and working when we quit */
    CGDisplayShowCursor (display_id);
    CGAssociateMouseAndMouseCursorPosition (1);
    
    /* Signal successful teardown */
    video_set = SDL_FALSE;
}

static SDL_Surface* QZ_SetVideoFullScreen (_THIS, SDL_Surface *current, int width,
                                           int height, int bpp, Uint32 flags) {
    int exact_match;
    int gamma_error;
    SDL_QuartzGammaTable gamma_table;
    
    /* See if requested mode exists */
    mode = CGDisplayBestModeForParameters (display_id, bpp, width, 
					   height, &exact_match);
    
    /* Require an exact match to the requested mode */
    if ( ! exact_match ) {
        sprintf (QZ_Error, "Failed to find display resolution: %dx%dx%d", width, height, bpp);
        SDL_SetError (QZ_Error);
        goto ERR_NO_MATCH;
    }

    /* Fade display to zero gamma */
    gamma_error = QZ_FadeGammaOut (this, &gamma_table);

    /* Put up the blanking window (a window above all other windows) */
    if ( CGDisplayNoErr != CGDisplayCapture (display_id) ) {
        SDL_SetError ("Failed capturing display");
        goto ERR_NO_CAPTURE;
    }

    
    /* Do the physical switch */
    if ( CGDisplayNoErr != CGDisplaySwitchToMode (display_id, mode) ) {
        SDL_SetError ("Failed switching display resolution");
        goto ERR_NO_SWITCH;
    }

    current->pixels = (Uint32*) CGDisplayBaseAddress (display_id);
    current->pitch  = CGDisplayBytesPerRow (display_id);

    current->flags = 0;
    current->w = width;
    current->h = height;
    current->flags |= SDL_FULLSCREEN;  
    current->flags |= SDL_HWSURFACE;
   
    this->UpdateRects = QZ_DirectUpdate;
    
    /* Setup some mode-dependant info */
    if ( CGSDisplayCanHWFill (display_id) ) {
         this->info.blit_fill = 1;
         this->FillHWRect = QZ_FillHWRect;
    }
        
    if ( CGDisplayCanSetPalette (display_id) )
        current->flags |= SDL_HWPALETTE;
    
    /* Setup OpenGL for a fullscreen context */
    if (flags & SDL_OPENGL) {

        CGLError err;
        CGLContextObj ctx;
        
        if ( ! QZ_SetupOpenGL (this, bpp, flags) ) {
            goto ERR_NO_GL;
        }
       
        ctx = [ gl_context cglContext ];
        err = CGLSetFullScreen (ctx);
        
        if (err) {
            sprintf (QZ_Error, "Error setting OpenGL fullscreen: %s", CGLErrorString(err));
            SDL_SetError (QZ_Error);
            goto ERR_NO_GL;
        }
         
        [ gl_context makeCurrentContext];

        glClear (GL_COLOR_BUFFER_BIT);

        [ gl_context flushBuffer ];
        
        current->flags |= SDL_OPENGL;
    }

    /* If we don't hide menu bar, it will get events and interrupt the program */
    HideMenuBar ();

    /* Fade the display to original gamma */
    if (! gamma_error )
        QZ_FadeGammaIn (this, &gamma_table);
    
    /* Save the flags to ensure correct tear-down */
    mode_flags = current->flags;
    
    return current;

 /* Since the blanking window covers *all* windows (even force quit) correct recovery is crucial */
 ERR_NO_GL:      CGDisplaySwitchToMode (display_id, save_mode);
 ERR_NO_SWITCH:  CGDisplayRelease (display_id);
 ERR_NO_CAPTURE: if (!gamma_error) { QZ_FadeGammaIn (this, &gamma_table); }
 ERR_NO_MATCH:	return NULL;
}

static SDL_Surface* QZ_SetVideoWindowed (_THIS, SDL_Surface *current, int width,
                                           int height, int bpp, Uint32 flags) {    
    unsigned int style;
    NSRect rect;    
    rect = NSMakeRect (0, 0, width, height);

#if 1 // FIXME - the resize button doesn't show?  Also need resize events...
    flags &= ~SDL_RESIZABLE;
#endif
    /* Set the window style based on input flags */
    if ( flags & SDL_NOFRAME ) {
        style = NSBorderlessWindowMask;
    } else {
        style = NSTitledWindowMask;
        style |= (NSMiniaturizableWindowMask | NSClosableWindowMask);
        if ( flags & SDL_RESIZABLE )
            style |= NSResizableWindowMask;
    }

    /* Manually create a window, avoids having a nib file resource */
    qz_window = [ [ SDL_QuartzWindow alloc ] initWithContentRect:rect 
        styleMask:style backing:NSBackingStoreBuffered defer:NO ];
    if (qz_window == nil) {
        SDL_SetError ("Could not create the Cocoa window");
        return NULL;
    }
    
    current->flags = 0;
    current->w = width;
    current->h = height;
    
    [ qz_window setReleasedWhenClosed:YES ];
    QZ_SetCaption(this, this->wm_title, this->wm_icon);
    [ qz_window setAcceptsMouseMovedEvents:YES ];
    [ qz_window setViewsNeedDisplay:NO ];
    [ qz_window center ];
    [ qz_window setDelegate:
        [ [ [ SDL_QuartzWindowDelegate alloc ] init ] autorelease ] ];
    
    /* For OpenGL, we set the content view to a NSOpenGLView */
    if ( flags & SDL_OPENGL ) {
    
        if ( ! QZ_SetupOpenGL (this, bpp, flags) ) {
            return NULL;
        }
        
        [ gl_context setView: [ qz_window contentView ] ];
        [ gl_context makeCurrentContext];
        [ qz_window makeKeyAndOrderFront:nil ];
        current->flags |= SDL_OPENGL;
    }
    /* For 2D, we set the content view to a NSQuickDrawView */
    else {
    
        window_view = [ [ SDL_QuartzWindowView alloc ] init ];
        [ qz_window setContentView:window_view ];
        [ qz_window makeKeyAndOrderFront:nil ];    
        
        LockPortBits ( [ window_view qdPort ] );
        current->pixels = GetPixBaseAddr ( GetPortPixMap ( [ window_view qdPort ] ) );
        current->pitch  = GetPixRowBytes ( GetPortPixMap ( [ window_view qdPort ] ) );
        
        current->flags |= SDL_SWSURFACE;
        current->flags |= SDL_PREALLOC;
        
	if ( flags & SDL_NOFRAME )
        	current->flags |= SDL_NOFRAME;
	if ( flags & SDL_RESIZABLE )
        	current->flags |= SDL_RESIZABLE;

        /* Offset 22 pixels down to fill the full content region */
	if ( ! (current->flags & SDL_NOFRAME) ) {
        	current->pixels += 22 * current->pitch;
	}

        this->UpdateRects = QZ_UpdateRects;
    }
    
    /* Save flags to ensure correct teardown */
    mode_flags = current->flags;
      
    return current;
}

static SDL_Surface* QZ_SetVideoMode (_THIS, SDL_Surface *current, int width, 
				     int height, int bpp, Uint32 flags) {

    if (video_set == SDL_TRUE)
        QZ_UnsetVideoMode (this);
       
    current->flags = 0;
    
    /* Setup full screen video */
    if ( flags & SDL_FULLSCREEN ) {
        current = QZ_SetVideoFullScreen (this, current, width, height, bpp, flags );
        if (current == NULL)
            return NULL;
    }
    /* Setup windowed video */
    else {
        /* Force bpp to the device's bpp */
        bpp = device_bpp;
        current = QZ_SetVideoWindowed (this, current, width, height, bpp, flags);
        if (current == NULL)
            return NULL;
    }
    
    /* Setup the new pixel format */
    {
        int amask = 0, 
            rmask = 0, 
            gmask = 0,
            bmask = 0;
            
        switch (bpp) {
            case 16:   /* (1)-5-5-5 RGB */
                amask = 0; 
                rmask = 0x7C00;
                gmask = 0x03E0;
                bmask = 0x001F;
                break;
            case 24:
                SDL_SetError ("24bpp is not available");
                return NULL;
            case 32:   /* (8)-8-8-8 ARGB */
                amask = 0x00000000;
                rmask = 0x00FF0000;
                gmask = 0x0000FF00;
                bmask = 0x000000FF;
                break;
        }
        
        if ( ! SDL_ReallocFormat (current, bpp,
                                  rmask, gmask, bmask, amask ) ) {
       	   SDL_SetError ("Couldn't reallocate pixel format");
           return NULL;
       	}
    }
    
    /* Signal successful completion (used internally) */
    video_set = SDL_TRUE;
    
    return current;
}

static int QZ_ToggleFullScreen (_THIS, int on) { 
    return -1;
}

static int QZ_SetColors (_THIS, int first_color, int num_colors, 
			 SDL_Color *colors) {

    CGTableCount  index;
    CGDeviceColor color;
    
    for (index = first_color; index < first_color+num_colors; index++) {
    
        /* Clamp colors between 0.0 and 1.0 */
        color.red   = colors->r / 255.0;
        color.blue  = colors->b / 255.0;
        color.green = colors->g / 255.0;
        
        colors++;
        
        CGPaletteSetColorAtIndex (palette, color, index);
    }
     
    if ( CGDisplayNoErr != CGDisplaySetPalette (display_id, palette) )
        return 0;
        
    return 1;
}

static void QZ_DirectUpdate (_THIS, int num_rects, SDL_Rect *rects) {
    #pragma unused(this,num_rects,rects)
}

/** 
 *  The obscured code is based on work by Matt Slot fprefect@ambrosiasw.com,
 *  who supplied sample code for Carbon.
 **/
static int QZ_IsWindowObscured (NSWindow *window) {

//#define TEST_OBSCURED 1

#if TEST_OBSCURED
    
    /*  In order to determine if a direct copy to the screen is possible,
       we must figure out if there are any windows covering ours (including shadows).
       This can be done by querying the window server about the on screen            
       windows for their screen rectangle and window level.                          
       The procedure used below is puts accuracy before speed; however, it aims to call 
       the window server the fewest number of times possible to keep things reasonable.
       In my testing on a 300mhz G3, this routine typically takes < 2 ms. -DW
    
        Notes:
            -Calls into the Window Server involve IPC which is slow.
            -Getting a rectangle seems slower than getting the window level
            -The window list we get back is in sorted order, top to bottom
            -On average, I suspect, most windows above ours are dock icon windows (hence optimization)
            -Some windows above ours are always there, and cannot move or obscure us (menu bar)
            
        Bugs:
            -no way (yet) to deactivate direct drawing when a window is dragged, 
               or suddenly obscured, so drawing continues and can produce garbage
               We need some kind of locking mechanism on window movement to prevent this
               
            -deactivated normal windows use activated normal 
               window shadows (slight inaccuraccy)
    */
    
    /* Cache the connection to the window server */
    static CGSConnectionID	cgsConnection = (CGSConnectionID) -1;
    
    /* Cache the dock icon windows */
    static CGSWindowID          dockIcons[kMaxWindows];
    static int                  numCachedDockIcons = 0;
    
    CGSWindowID		        windows[kMaxWindows];
    CGSWindowCount	        i, count;
    CGSWindowLevel	        winLevel;
    CGSRect			winRect;

    CGSRect contentRect;
    int     windowNumber;
    //int     isMainWindow;
    int     firstDockIcon;    
    int     dockIconCacheMiss;
    int     windowContentOffset;
    
    int     obscured = SDL_TRUE;
        
    if ( [ window isVisible ] ) {
        
        /*  walk the window list looking for windows over top of 
                (or casting a shadow on) ours */
       
        /* Get a connection to the window server */
        /* Should probably be moved out into SetVideoMode() or InitVideo() */
        if (cgsConnection == (CGSConnectionID) -1) {
            cgsConnection = (CGSConnectionID) 0;
            cgsConnection = _CGSDefaultConnection ();
        }
        
        if (cgsConnection) { 
        
            if ( ! [ window styleMask ] & NSBorderlessWindowMask )
                windowContentOffset = 22;
            else
                windowContentOffset = 0;
                
            windowNumber = [ window windowNumber ];
            //isMainWindow = [ window isMainWindow ];
            
            /* The window list is sorted according to order on the screen */
            count = 0;
            CGSGetOnScreenWindowList (cgsConnection, 0, kMaxWindows, windows, &count);
            CGSGetScreenRectForWindow (cgsConnection, windowNumber, &contentRect);
    
            /* adjust rect for window title bar (if present) */
            contentRect.origin.y    += windowContentOffset;
            contentRect.size.height -= windowContentOffset;
            
            firstDockIcon = -1;
            dockIconCacheMiss = SDL_FALSE;
            
            /* The first window is always an empty window with level kCGSWindowLevelTop 
               so start at index 1 */
            for (i = 1; i < count; i++) {
                
                /* If we reach our window in the list, it cannot be obscured */
                if (windows[i] == windowNumber) {
                    
                    obscured = SDL_FALSE;
                    break;
                }
                else {
                    
                    float shadowSide;
                    float shadowTop;
                    float shadowBottom;

                    CGSGetWindowLevel (cgsConnection, windows[i], &winLevel);
                    
                    if (winLevel == kCGSWindowLevelDockIcon) {
                    
                        int j;
                        
                        if (firstDockIcon < 0) {
                            
                            firstDockIcon = i;
                        
                            if (numCachedDockIcons > 0) {
                            
                                for (j = 0; j < numCachedDockIcons; j++) {
                            
                                    if (windows[i] == dockIcons[j])
                                        i++;
                                    else
                                        break;
                                }
                            
                                if (j != 0) {
                                 
                                    i--;
                                                                    
                                    if (j < numCachedDockIcons) {
                                        
                                        dockIconCacheMiss = SDL_TRUE;
                                    }
                                }

                            }
                        }
                                               
                        continue;
                    }
                    else if (winLevel == kCGSWindowLevelMenuIgnore
                             /* winLevel == kCGSWindowLevelTop */) {
                     
                        continue; /* cannot obscure window */
                    }
                    else if (winLevel == kCGSWindowLevelDockMenu ||
                             winLevel == kCGSWindowLevelMenu) {
                     
                        shadowSide = 18;
                        shadowTop = 4;
                        shadowBottom = 22;   
                    }
                    else if (winLevel == kCGSWindowLevelUtility) {
                    
                        shadowSide = 8;
                        shadowTop = 4;
                        shadowBottom = 12;
                    }
                    else if (winLevel == kCGSWindowLevelNormal) {
                    
                        /* These numbers are for foreground windows,
                           they are too big (but will work) for background windows */
                        shadowSide = 20;
                        shadowTop = 10;
                        shadowBottom = 24;
                    }
                    else if (winLevel == kCGSWindowLevelDock) {
                    
                        /* Create dock icon cache */
                        if (numCachedDockIcons != (i-firstDockIcon) ||
                            dockIconCacheMiss) {
                        
                            numCachedDockIcons = i - firstDockIcon;
                            memcpy (dockIcons, &(windows[firstDockIcon]), 
                                    numCachedDockIcons * sizeof(*windows));
                        }
                        
                        /* no shadow */
                        shadowSide = 0;
                        shadowTop = 0;
                        shadowBottom = 0;
                    }
                    else {
                    
                        /*   kCGSWindowLevelDockLabel,
                             kCGSWindowLevelDock,
                             kOther??? */
                        
                        /* no shadow */
                        shadowSide = 0;
                        shadowTop = 0;
                        shadowBottom = 0;
                    }
                    
                    CGSGetScreenRectForWindow (cgsConnection, windows[i], &winRect);
                    
                    winRect.origin.x -= shadowSide;
                    winRect.origin.y -= shadowTop;
                    winRect.size.width += shadowSide;
                    winRect.size.height += shadowBottom;
                    
                    if (NSIntersectsRect (contentRect, winRect)) {
                    
                        obscured = SDL_TRUE;
                        break;
                    }
               
               } /* window was not our window */
            
            } /* iterate over windows */
            
        } /* get cgsConnection */
    
    } /* window is visible */
    
    return obscured;
#else
    return SDL_TRUE;
#endif
}

static void QZ_UpdateRects (_THIS, int numRects, SDL_Rect *rects) { 

    if (SDL_VideoSurface->flags & SDL_OPENGLBLIT) {
        QZ_GL_SwapBuffers (this);
    }
    else if ( [ qz_window isMiniaturized ] && 
              ! (SDL_VideoSurface->flags & SDL_OPENGL)) {
    
        /** 
         * Set port alpha opaque so deminiaturize looks right
         * This isn't so nice, but there is no 
         * initial deminatureize notification (before demini starts)
         **/
        QZ_SetPortAlphaOpaque ([ [ qz_window contentView ] qdPort],
                                [ qz_window styleMask ] & NSBorderlessWindowMask);
    }
    else if ( ! QZ_IsWindowObscured (qz_window) ) {
        
        /* Use direct copy to flush contents to the display */
        CGrafPtr savePort;
        CGrafPtr dstPort, srcPort;
        const BitMap  *dstBits, *srcBits;
        Rect     dstRect, srcRect;      
        Point    offset;
        int i;
        
        GetPort (&savePort);
        
        dstPort = CreateNewPortForCGDisplayID ((UInt32)display_id);
        srcPort = [ window_view qdPort ];
        
        offset.h = 0;
        offset.v = 0;
        SetPort (srcPort);
        LocalToGlobal (&offset);
        
        SetPort (dstPort);
        
        LockPortBits (dstPort);
        LockPortBits (srcPort);
        
        dstBits = GetPortBitMapForCopyBits (dstPort);
        srcBits = GetPortBitMapForCopyBits (srcPort);
        
        for (i = 0; i < numRects; i++) {
            
            SetRect (&srcRect, rects[i].x, rects[i].y,
                     rects[i].x + rects[i].w,
                     rects[i].y + rects[i].h);
            
            SetRect (&dstRect,
                     rects[i].x + offset.h, 
                     rects[i].y + offset.v,
                     rects[i].x + rects[i].w + offset.h,
                     rects[i].y + rects[i].h + offset.v);
                        
            CopyBits (srcBits, dstBits,
                      &srcRect, &dstRect, srcCopy, NULL);
                        
        }
        
        SetPort (savePort);
    }
    else {
    
        /* Use QDFlushPortBuffer() to flush content to display */
        int i;
        RgnHandle dirty = NewRgn ();
        RgnHandle temp  = NewRgn ();
        
        SetEmptyRgn (dirty);
        
        /* Build the region of dirty rectangles */
        for (i = 0; i < numRects; i++) {
        
            MacSetRectRgn (temp, rects[i].x, rects[i].y, 
                rects[i].x + rects[i].w, rects[i].y + rects[i].h);
            MacUnionRgn (dirty, temp, dirty);
        }
        
        /* Flush the dirty region */
        QDFlushPortBuffer ( [ window_view qdPort ], dirty );
        DisposeRgn (dirty);
        DisposeRgn (temp);
    }
}

static void QZ_VideoQuit (_THIS) {

    QZ_UnsetVideoMode (this);
    CGPaletteRelease (palette);
}

static int  QZ_FillHWRect (_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color) {

    CGSDisplayHWFill (display_id, rect->x, rect->y, rect->w, rect->h, color);

    return 0;
}

static int  QZ_LockHWSurface(_THIS, SDL_Surface *surface) { 

    return 1;
}

static void QZ_UnlockHWSurface(_THIS, SDL_Surface *surface) {

}

static void QZ_FreeHWSurface (_THIS, SDL_Surface *surface) {
}

/*
int QZ_FlipHWSurface (_THIS, SDL_Surface *surface) {
    return 0;
}
*/

/* Gamma functions */
static int QZ_SetGamma (_THIS, float red, float green, float blue) {

    const CGGammaValue min = 0.0, max = 1.0;

    if (red == 0.0)
        red = FLT_MAX;
    else
        red = 1.0 / red;

    if (green == 0.0)
        green = FLT_MAX;
    else
        green = 1.0 / green;

    if (blue == 0.0)
        blue = FLT_MAX;
    else
        blue  = 1.0 / blue;
    
    if ( CGDisplayNoErr == CGSetDisplayTransferByFormula 
        (display_id, min, max, red, min, max, green, min, max, blue) ) {
            
        return 0;
    }
    else {
    
        return -1;
    }
}

static int QZ_GetGamma (_THIS, float *red, float *green, float *blue) {

    CGGammaValue dummy;
    if ( CGDisplayNoErr == CGGetDisplayTransferByFormula
        (display_id, &dummy, &dummy, red, 
	 &dummy, &dummy, green, &dummy, &dummy, blue) )
        
        return 0;
    else
        return -1;
}

static int QZ_SetGammaRamp (_THIS, Uint16 *ramp) {
 
   const CGTableCount tableSize = 255;
   CGGammaValue redTable[tableSize];
   CGGammaValue greenTable[tableSize];
   CGGammaValue blueTable[tableSize];
   
   int i;
   
   /* Extract gamma values into separate tables, convert to floats between 0.0 and 1.0 */
   for (i = 0; i < 256; i++)
    redTable[i % 256] = ramp[i] / 65535.0;
   
   for (i=256; i < 512; i++)
    greenTable[i % 256] = ramp[i] / 65535.0;
   
   for (i=512; i < 768; i++)
     blueTable[i % 256] = ramp[i] / 65535.0;
     
    if ( CGDisplayNoErr == CGSetDisplayTransferByTable 
            (display_id, tableSize, redTable, greenTable, blueTable) )        
        return 0;
    else
        return -1;
}

static int QZ_GetGammaRamp (_THIS, Uint16 *ramp) {
    
    const CGTableCount tableSize = 255;
    CGGammaValue redTable[tableSize];
    CGGammaValue greenTable[tableSize];
    CGGammaValue blueTable[tableSize];
    CGTableCount actual;
    int i;
    
    if ( CGDisplayNoErr != CGGetDisplayTransferByTable 
            (display_id, tableSize, redTable, greenTable, blueTable, &actual) ||
          actual != tableSize)
        
        return -1;
    
    /* Pack tables into one array, with values from 0 to 65535 */
    for (i = 0; i < 256; i++)
        ramp[i] = redTable[i % 256] * 65535.0;
   
    for (i=256; i < 512; i++)
        ramp[i] = greenTable[i % 256] * 65535.0;
   
    for (i=512; i < 768; i++)
        ramp[i] = blueTable[i % 256] * 65535.0;
      
    return 0;    
}

/* OpenGL helper functions (used internally) */

static int QZ_SetupOpenGL (_THIS, int bpp, Uint32 flags) {

    NSOpenGLPixelFormatAttribute attr[32];
    NSOpenGLPixelFormat *fmt;
    int i = 0;
    int colorBits = bpp;

    if ( flags & SDL_FULLSCREEN ) {
    
        attr[i++] = NSOpenGLPFAFullScreen;
    }
    /* In windowed mode, the OpenGL pixel depth must match device pixel depth */
    else if ( colorBits != device_bpp ) { 

        colorBits = device_bpp;
    }
    
    attr[i++] = NSOpenGLPFAColorSize;
    attr[i++] = colorBits;
    
    attr[i++] = NSOpenGLPFADepthSize;
    attr[i++] = this->gl_config.depth_size;

    if ( this->gl_config.double_buffer ) {
        attr[i++] = NSOpenGLPFADoubleBuffer;
    }
    
    if ( this->gl_config.stencil_size != 0 ) {
        attr[i++] = NSOpenGLPFAStencilSize;
        attr[i++] = this->gl_config.stencil_size;
    }
    
    attr[i++] = NSOpenGLPFAScreenMask;
    attr[i++] = CGDisplayIDToOpenGLDisplayMask (display_id);
    attr[i] = 0;
    
    fmt = [ [ NSOpenGLPixelFormat alloc ] initWithAttributes:attr ];
    if (fmt == nil) {
        SDL_SetError ("Failed creating OpenGL pixel format");
        return 0;
    }
    
    gl_context = [ [ NSOpenGLContext alloc ] initWithFormat:fmt 
        shareContext:nil];
    
    if (gl_context == nil) {
        SDL_SetError ("Failed creating OpenGL context");
        return 0;
    }	
    
    /* Convince SDL that the GL "driver" is loaded */
    this->gl_config.driver_loaded = 1;
    
    [ fmt release ];
    
    return 1;
}

static void QZ_TearDownOpenGL (_THIS) {

    [ NSOpenGLContext clearCurrentContext ];
    [ gl_context clearDrawable ];
    [ gl_context release ];
}


/* SDL OpenGL functions */

static int    QZ_GL_LoadLibrary    (_THIS, const char *location) {
    this->gl_config.driver_loaded = 1;
    return 1;
}

static void*  QZ_GL_GetProcAddress (_THIS, const char *proc) {

    /* We may want to cache the bundleRef at some point */
    CFBundleRef bundle;
    CFURLRef bundleURL = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, 
        CFSTR("/System/Library/Frameworks/OpenGL.framework"), kCFURLPOSIXPathStyle, true);
    
    CFStringRef functionName = CFStringCreateWithCString 
        (kCFAllocatorDefault, proc, kCFStringEncodingASCII);
    
    void *function;
    
    bundle = CFBundleCreate (kCFAllocatorDefault, bundleURL);
    assert (bundle != NULL);
    
    function = CFBundleGetFunctionPointerForName (bundle, functionName);

    CFRelease ( bundleURL );
    CFRelease ( functionName );
    CFRelease ( bundle );
    
    return function;
}

static int    QZ_GL_GetAttribute   (_THIS, SDL_GLattr attrib, int* value) {

    GLenum attr = 0;

    QZ_GL_MakeCurrent (this);

    switch (attrib) {
    case SDL_GL_RED_SIZE: attr = GL_RED_BITS;   break;
    case SDL_GL_BLUE_SIZE: attr = GL_BLUE_BITS;  break;
    case SDL_GL_GREEN_SIZE: attr = GL_GREEN_BITS; break;
    case SDL_GL_ALPHA_SIZE: attr = GL_ALPHA_BITS; break;
    case SDL_GL_DOUBLEBUFFER: attr = GL_DOUBLEBUFFER; break;
    case SDL_GL_DEPTH_SIZE: attr = GL_DEPTH_BITS;  break;
    case SDL_GL_STENCIL_SIZE: attr = GL_STENCIL_BITS; break;
    case SDL_GL_ACCUM_RED_SIZE: attr = GL_ACCUM_RED_BITS; break;
    case SDL_GL_ACCUM_GREEN_SIZE: attr = GL_ACCUM_GREEN_BITS; break;
    case SDL_GL_ACCUM_BLUE_SIZE: attr = GL_ACCUM_BLUE_BITS; break;
    case SDL_GL_ACCUM_ALPHA_SIZE: attr = GL_ACCUM_ALPHA_BITS; break;
    case SDL_GL_BUFFER_SIZE:
        {
            GLint bits = 0;
            GLint component;

            /* there doesn't seem to be a single flag in OpenGL for this! */
            glGetIntegerv (GL_RED_BITS, &component);   bits += component;
            glGetIntegerv (GL_GREEN_BITS,&component);  bits += component;
            glGetIntegerv (GL_BLUE_BITS, &component);  bits += component;
            glGetIntegerv (GL_ALPHA_BITS, &component); bits += component;

            *value = bits;
        }
        return 0;
    }

    glGetIntegerv (attr, (GLint *)value);
    return 0;
}

static int    QZ_GL_MakeCurrent    (_THIS) {
    [ gl_context makeCurrentContext ];
    return 0;
}

static void   QZ_GL_SwapBuffers    (_THIS) {
    [ gl_context flushBuffer ];
}


