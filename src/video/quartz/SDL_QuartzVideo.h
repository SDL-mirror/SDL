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

/*	
    @file   SDL_QuartzVideo.h
    @author Darrell Walisser
    
    @abstract SDL video driver for MacOS X.
    
    @discussion
    
    TODO
        - Hardware Cursor support with NSCursor instead of Carbon
        - Keyboard repeat/mouse speed adjust (if needed)
        - Multiple monitor support (currently only main display)
        - Accelerated blitting support
        - Set the window icon (dock icon when API is available)
        - Avoid erasing window on minimize, or disable minimize
    Problems:
        - OGL not working in full screen with software renderer
        - SetColors sets palette correctly but clears framebuffer
        - Crash in CG after several mode switches
        - Retained windows don't draw their title bar quite right (OS Bug)
        - Should I do depth switching for windowed modes? - No, not usually.
        - Launch times are slow, maybe prebinding will help
        - Direct framebuffer access has some artifacts, maybe a driver issue
        - Cursor in 8 bit modes is screwy
*/

#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "SDL_video.h"
#include "SDL_error.h"
#include "SDL_syswm.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"

/* This is a workaround to directly access NSOpenGLContext's CGL context */
/* We need to do this in order to check for errors */
@interface NSOpenGLContext (CGLContextAccess)
- (CGLContextObj) cglContext;
@end

@implementation NSOpenGLContext (CGLContextAccess)
- (CGLContextObj) cglContext;
{
    return _contextAuxiliary;
}
@end

typedef struct SDL_PrivateVideoData {

    CGDirectDisplayID  display; /* 0 == main display */
    CFDictionaryRef    mode;
    CFDictionaryRef    save_mode;
    CFArrayRef         mode_list;
    CGDirectPaletteRef palette;
    NSOpenGLContext    *gl_context;
    Uint32             width, height, bpp;
    Uint32             flags;
    SDL_bool           video_is_set; /* tell if the video mode was set */
    
    /* Window-only fields */
    NSWindow        *window;
    NSQuickDrawView *view;
    
} SDL_PrivateVideoData ;

#define _THIS	SDL_VideoDevice *this
#define display_id (this->hidden->display)
#define mode (this->hidden->mode)
#define save_mode (this->hidden->save_mode)
#define mode_list (this->hidden->mode_list)
#define palette (this->hidden->palette)
#define glcontext (this->hidden->glcontext)
#define objc_video (this->hidden->objc_video)
#define gl_context (this->hidden->gl_context)
#define device_width (this->hidden->width)
#define device_height (this->hidden->height)
#define device_bpp (this->hidden->bpp)
#define mode_flags (this->hidden->flags)
#define video_set (this->hidden->video_is_set)
#define qz_window (this->hidden->window)
#define windowView (this->hidden->view)

/* Interface for hardware fill not (yet) in the public API */
int CGSDisplayHWFill (CGDirectDisplayID id, unsigned int x, unsigned int y, 
                      unsigned int w, unsigned int h, unsigned int color);
int CGSDisplayCanHWFill (CGDirectDisplayID id);

/* Bootstrap functions */
static int              QZ_Available ();
static SDL_VideoDevice* QZ_CreateDevice (int device_index);
static void             QZ_DeleteDevice (SDL_VideoDevice *device);

/* Initialization, Query, Setup, and Redrawing functions */
static int          QZ_VideoInit        (_THIS, SDL_PixelFormat *video_format);

static SDL_Rect**   QZ_ListModes        (_THIS, SDL_PixelFormat *format, 
					 Uint32 flags);
static void         QZ_UnsetVideoMode   (_THIS);

static SDL_Surface* QZ_SetVideoMode     (_THIS, SDL_Surface *current, 
					 int width, int height, int bpp, 
					 Uint32 flags);
static int          QZ_ToggleFullScreen (_THIS, int on);
static int          QZ_SetColors        (_THIS, int first_color, 
					 int num_colors, SDL_Color *colors);
static void         QZ_DirectUpdate     (_THIS, int num_rects, SDL_Rect *rects);
static void         QZ_UpdateRects      (_THIS, int num_rects, SDL_Rect *rects);
static void         QZ_VideoQuit        (_THIS);

/* Hardware surface functions (for fullscreen mode only) */
static int  QZ_FillHWRect (_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color);
static int  QZ_LockHWSurface(_THIS, SDL_Surface *surface);
static void QZ_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void QZ_FreeHWSurface (_THIS, SDL_Surface *surface);
/* static int  QZ_FlipHWSurface (_THIS, SDL_Surface *surface); */

/* Gamma Functions */
static int QZ_SetGamma     (_THIS, float red, float green, float blue);
static int QZ_GetGamma     (_THIS, float *red, float *green, float *blue);
static int QZ_SetGammaRamp (_THIS, Uint16 *ramp);
static int QZ_GetGammaRamp (_THIS, Uint16 *ramp);

/* OpenGL functions */
static int    QZ_SetupOpenGL (_THIS, int bpp, Uint32 flags);
static void   QZ_TearDownOpenGL (_THIS);
static void*  QZ_GL_GetProcAddress (_THIS, const char *proc);
static int    QZ_GL_GetAttribute   (_THIS, SDL_GLattr attrib, int* value);
static int    QZ_GL_MakeCurrent    (_THIS);
static void   QZ_GL_SwapBuffers    (_THIS);
static int    QZ_GL_LoadLibrary    (_THIS, const char *location);

/* Private function to warp the cursor (used internally) */
static void  QZ_PrivateWarpCursor (_THIS, int fullscreen, int h, int x, int y);

/* Cursor and Mouse functions */
static void         QZ_FreeWMCursor     (_THIS, WMcursor *cursor);
static WMcursor*    QZ_CreateWMCursor   (_THIS, Uint8 *data, Uint8 *mask, 
                                          int w, int h, int hot_x, int hot_y);
static int          QZ_ShowWMCursor     (_THIS, WMcursor *cursor);
static void         QZ_WarpWMCursor     (_THIS, Uint16 x, Uint16 y);
static void         QZ_MoveWMCursor     (_THIS, int x, int y);
static void         QZ_CheckMouseMode   (_THIS);

/* Event functions */
static void         QZ_InitOSKeymap     (_THIS);
static void         QZ_PumpEvents       (_THIS);

/* Window Manager functions */
static void QZ_SetCaption    (_THIS, const char *title, const char *icon);
static void QZ_SetIcon       (_THIS, SDL_Surface *icon, Uint8 *mask);
static int  QZ_IconifyWindow (_THIS);
static SDL_GrabMode QZ_GrabInput (_THIS, SDL_GrabMode grab_mode);
/*static int  QZ_GetWMInfo     (_THIS, SDL_SysWMinfo *info);*/
