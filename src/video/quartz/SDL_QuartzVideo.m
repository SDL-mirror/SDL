/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
    slouken@devolution.com
*/

#include "SDL_QuartzVideo.h"

/* Some variables to share among files, put in device structure eventually */
static SDL_GrabMode currentGrabMode = SDL_GRAB_OFF;
static BOOL   inForeground = YES;
static SDLKey keymap[256];
static unsigned int currentMods = 0; /* Current keyboard modifiers, to track modifier state */

/* Include files into one compile unit...break apart eventually */
#include "SDL_QuartzWM.m"
#include "SDL_QuartzEvents.m"
#include "SDL_QuartzWindow.m"

char QZ_Error[255]; /* Global error buffer to temporarily store more informative error messages */

/* Bootstrap binding, enables entry point into the driver */
VideoBootStrap QZ_bootstrap = {
    "Quartz", "MacOS X CoreGraphics", QZ_Available, QZ_CreateDevice
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
    
    CFIndex num_modes = CFArrayGetCount (mode_list);
    CFIndex i;

    static SDL_Rect **list = NULL;
    int list_size = 0;

    /* Any windowed mode is acceptable */
    if ( (flags & SDL_FULLSCREEN) == 0 )
        return (SDL_Rect**)-1;
        
    /* Free memory from previous call, if any */
    if ( list != NULL ) {

      int i = 0;

      for (i = 0; list[i] != NULL; i++)
	free (list[i]);

      free (list);
      list = NULL;
    }

    /* Build list of modes with the requested bpp */
    for (i = num_modes-1; i >= 0; i--) {
   
        CFDictionaryRef onemode = CFArrayGetValueAtIndex (mode_list, i);
	CFNumberRef     number;
	int bpp;
	
	number = CFDictionaryGetValue (onemode, kCGDisplayBitsPerPixel);
	CFNumberGetValue (number, kCFNumberSInt32Type, &bpp);

	if (bpp == format->BitsPerPixel) {
	  
	  int       intvalue;
	  SDL_Rect *rect;
          int       lastwidth = 0, lastheight = 0, width, height;

	  number = CFDictionaryGetValue (onemode, kCGDisplayWidth);
	  CFNumberGetValue (number, kCFNumberSInt32Type, &intvalue);
	  width = (Uint16) intvalue;
	  
	  number = CFDictionaryGetValue (onemode, kCGDisplayHeight);
	  CFNumberGetValue (number, kCFNumberSInt32Type, &intvalue);
	  height = (Uint16) intvalue;
          
          /* We'll get a lot of modes with the same size, so ignore them */
          if ( width != lastwidth && height != lastheight ) {

            lastwidth  = width;
            lastheight = height;
	  
            list_size++;

            if ( list == NULL)
                list = (SDL_Rect**) malloc (sizeof(*list) * list_size+1);
            else
                list = (SDL_Rect**) realloc (list, sizeof(*list) * list_size+1);
            
            rect = (SDL_Rect*) malloc (sizeof(**list));
            
            if (list == NULL || rect == NULL)
                SDL_OutOfMemory ();
    
            rect->w = width;
            rect->h = height;
    
            list[list_size-1] = rect;
            list[list_size]   = NULL;
        }
      }
    }
    
    return list;
}

static void QZ_UnsetVideoMode (_THIS) {

    if ( mode_flags & SDL_OPENGL )
        QZ_TearDownOpenGL (this);

    /* Reset values that may change between switches */
    this->info.blit_fill = 0;
    this->FillHWRect     = NULL;
    this->UpdateRects    = NULL;
    
    /* Restore gamma settings */
    CGDisplayRestoreColorSyncSettings ();
   
    /* Restore original screen resolution */
    if ( mode_flags & SDL_FULLSCREEN ) {
        CGDisplaySwitchToMode (display_id, save_mode);
        CGDisplayRelease (display_id);
        this->screen->pixels = NULL;
    }
    /* Release window mode data structures */
    else { 
        if ( (mode_flags & SDL_OPENGL) == 0 ) {
            UnlockPortBits ( [ windowView qdPort ] );
            [ windowView release  ];
        }
        [ window setContentView:nil ];
        [ window close ];
        [ window release ];
    }
    
    /* Ensure the cursor will be visible and working when we quit */
    CGDisplayShowCursor (display_id);
    CGAssociateMouseAndMouseCursorPosition (1);
}

static SDL_Surface* QZ_SetVideoFullScreen (_THIS, SDL_Surface *current, int width,
                                           int height, int bpp, Uint32 flags) {
    int exact_match;

    /* See if requested mode exists */
    mode = CGDisplayBestModeForParameters (display_id, bpp, width, 
					   height, &exact_match);
    
    /* Require an exact match to the requested mode */
    if ( ! exact_match ) {
        sprintf (QZ_Error, "Failed to find display resolution: %dx%dx%d", width, height, bpp);
        SDL_SetError (QZ_Error);
        goto ERR_NO_MATCH;
    }
    
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
    
    /* None of these methods seem to fix the fullscreen artifacts bug(s) */
#if USE_GDHANDLE
    SetGDevice(GetMainDevice());
    current->pitch = (**(**  GetMainDevice() ).gdPMap).rowBytes & 0x3FFF;
    current->pixels = (**(** GetMainDevice() ).gdPMap).baseAddr;
#elif USE_CREATEPORT
    device_port = CreateNewPortForCGDisplayID((Uint32*)display_id);
    SetPort (device_port);
    LockPortBits ( device_port );
    current->pixels = GetPixBaseAddr ( GetPortPixMap ( device_port ) );
    current->pitch  = GetPixRowBytes ( GetPortPixMap ( device_port ) );
    UnlockPortBits ( device_port );
#else
    current->pixels = (Uint32*) CGDisplayBaseAddress (display_id);
    current->pitch  = CGDisplayBytesPerRow (display_id);
#endif

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
            return NULL;
        }
       
        ctx = [ gl_context cglContext ];
        err = CGLSetFullScreen (ctx);
        
        if (err) {
            sprintf (QZ_Error, "Error setting OpenGL fullscreen: %s", CGLErrorString(err));
            SDL_SetError (QZ_Error);
            goto ERR_NO_GL;
        }
         
        [ gl_context makeCurrentContext];
        
        current->flags |= SDL_OPENGL;
    }

    /* If we don't hide menu bar, it will get events and interrupt the program */
    HideMenuBar ();
    
    /* Save the flags to ensure correct tear-down */
    mode_flags = current->flags;
    
    return current;

 /* Since the blanking window covers *all* windows (even force quit) correct recovery is crutial */
 ERR_NO_GL:      CGDisplaySwitchToMode (display_id, save_mode);
 ERR_NO_SWITCH:  CGDisplayRelease (display_id);
 ERR_NO_CAPTURE:
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
    window = [ [ SDL_QuartzWindow alloc ] initWithContentRect:rect 
        styleMask:style backing:NSBackingStoreRetained defer:NO ];
    if (window == nil) {
        SDL_SetError ("Could not create the Cocoa window");
        return NULL;
    }
    
    current->flags = 0;
    current->w = width;
    current->h = height;
    
    [ window setReleasedWhenClosed:YES ];
    QZ_SetCaption(this, this->wm_title, this->wm_icon);
    [ window setAcceptsMouseMovedEvents:YES ];
    [ window setViewsNeedDisplay:NO ];
    [ window center ];
    [ window setDelegate:
        [ [ [ SDL_QuartzWindowDelegate alloc ] init ] autorelease ] ];
    
    /* For OpenGL, we set the content view to a NSOpenGLView */
    if ( flags & SDL_OPENGL ) {
    
        if ( ! QZ_SetupOpenGL (this, bpp, flags) ) {
            return NULL;
        }
        
        [ gl_context setView: [ window contentView ] ];
        [ gl_context makeCurrentContext];
        [ window orderFront:nil ];
        current->flags |= SDL_OPENGL;
    }
    /* For 2D, we set the content view to a NSQuickDrawView */
    else {
    
        windowView = [ [ NSQuickDrawView alloc ] init ];
        [ window setContentView:windowView ];
        [ window orderFront:nil ];    
        
        LockPortBits ( [ windowView qdPort ] );
        current->pixels = GetPixBaseAddr ( GetPortPixMap ( [ windowView qdPort ] ) );
        current->pitch  = GetPixRowBytes ( GetPortPixMap ( [ windowView qdPort ] ) );
            
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
    return current;
}

static SDL_Surface* QZ_SetVideoMode (_THIS, SDL_Surface *current, int width, 
				     int height, int bpp, Uint32 flags) {

    if (SDL_VideoSurface != NULL)
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
        bpp = current->format->BitsPerPixel;
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
                amask = 0x00000000; /* These are the correct semantics */
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
    
    /* Warp mouse to origin in order to get passive mouse motion events started correctly */
    QZ_PrivateWarpCursor (this, current->flags & SDL_FULLSCREEN, height, 0, 0);
    
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

static void QZ_UpdateRects (_THIS, int numRects, SDL_Rect *rects) { 
    
    if (SDL_VideoSurface->flags & SDL_OPENGLBLIT) {
        QZ_GL_SwapBuffers (this);
    }
    else {
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
        QDFlushPortBuffer ( [ windowView qdPort ], dirty );
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
    
    if ( CGDisplayNoErr != CGSetDisplayTransferByFormula 
        (display_id, min, max, red, min, max, green, min, max, blue) )
        return -1;
    
    return 0;
}

static int QZ_GetGamma (_THIS, float *red, float *green, float *blue) {

    CGGammaValue dummy;
    if ( CGDisplayNoErr != CGGetDisplayTransferByFormula
        (display_id, &dummy, &dummy, red, 
	 &dummy, &dummy, green, &dummy, &dummy, blue) )
        
        return -1;
    
    return 0;
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
     
    if ( CGDisplayNoErr != CGSetDisplayTransferByTable 
            (display_id, tableSize, redTable, greenTable, blueTable) )        
        return -1;
    
    return 0;
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

/* OpenGL helper functions */
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

/*
    CGLContextRef ctx = [ gl_context cglContext ];
    CGLContextParameter param;
    
    switch (attrib) {
    case SDL_GL_RED_SIZE: param = CGLContextParameter break;
    
    }
    
    CGLGetParameter (ctx, param, (long*)value);
*/
    *value = -1;
    return -1;
}

static int    QZ_GL_MakeCurrent    (_THIS) {
    [ gl_context makeCurrentContext ];
    return 0;
}

static void   QZ_GL_SwapBuffers    (_THIS) {
    [ gl_context flushBuffer ];
}
