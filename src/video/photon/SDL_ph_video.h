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

#ifndef _SDL_ph_video_h
#define _SDL_ph_video_h

#include "SDL_mouse.h"
#include "SDL_sysvideo.h"

#include "Ph.h"
#include "Pt.h"
#include <photon/Pg.h>
#include <photon/PdDirect.h>
#ifdef HAVE_OPENGL
#include <photon/PdGL.h>
#endif /* HAVE_OPENGL */

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

#define PH_OGL_MAX_ATTRIBS 32

#define SDLPH_PAL_NONE    0x00000000L
#define SDLPH_PAL_EMULATE 0x00000001L
#define SDLPH_PAL_SYSTEM  0x00000002L

typedef union vidptr{
  uint8_t  *volatile ptr8;
  uint16_t *volatile ptr16;
  uint32_t *volatile ptr32;
 } VidPtr_t;

typedef struct {
	unsigned char *Y;
	unsigned char *V;
	unsigned char *U;
}FRAMEDATA;

#define EVENT_SIZE    sizeof( PhEvent_t ) + 1000

/* Private display data */
struct SDL_PrivateVideoData {
    PgDisplaySettings_t mode_settings;	
    PtWidget_t *Window;                  /* used to handle input events */
    PhImage_t *image;	                 /* used to display image       */
#ifdef HAVE_OPENGL
    PdOpenGLContext_t* OGLContext;       /* OpenGL context              */
#endif /* HAVE_OPENGL */
    PgColor_t ph_palette[_Pg_MAX_PALETTE];

    struct {
        PdDirectContext_t *direct_context;
        PdOffscreenContext_t *offscreen_context;
        VidPtr_t dc_ptr;
        FRAMEDATA *CurrentFrameData;
        FRAMEDATA *FrameData0;
        FRAMEDATA *FrameData1;
        int current;
        long flags;
    } ocimage;

    PgHWCaps_t graphics_card_caps;  /* Graphics card caps at the moment of start   */
    int old_video_mode;             /* Stored mode before fullscreen switch        */
    int old_refresh_rate;           /* Stored refresh rate befor fullscreen switch */

    /* The current width and height of the fullscreen mode */
    int current_w;
    int current_h;

    /* Support for internal mouse warping */
    struct {
        int x;
        int y;
    } mouse_last;

    struct {
        int numerator;
        int denominator;
        int threshold;
    } mouse_accel;

    int mouse_relative;
    WMcursor* BlankCursor;

    int depth;			/* current visual depth (not bpp)        */
    int desktopbpp;             /* bpp of desktop at the moment of start */
    int desktoppal;             /* palette mode emulation or system      */
    int captionflag;            /* caption setting flag                  */

    int use_vidmode;
    int currently_fullscreen;

    /* Automatic mode switching support (entering/leaving fullscreen) */
    Uint32 switch_waiting;
    Uint32 switch_time;

    /* Prevent too many XSync() calls */
    int blit_queued;

    PhEvent_t* event;
};

#define mode_settings        (this->hidden->mode_settings)
#define window	             (this->hidden->Window)
#define oglctx               (this->hidden->OGLContext)
#define SDL_Image            (this->hidden->image)
#define OCImage              (this->hidden->ocimage)
#define old_video_mode       (this->hidden->old_video_mode)
#define old_refresh_rate     (this->hidden->old_refresh_rate)
#define graphics_card_caps   (this->hidden->graphics_card_caps)
#define desktopbpp           (this->hidden->desktopbpp)
#define desktoppal           (this->hidden->desktoppal)
#define ph_palette           (this->hidden->ph_palette)

/* Old variable names */
#define current_w            (this->hidden->current_w)
#define current_h            (this->hidden->current_h)
#define mouse_last           (this->hidden->mouse_last)
#define mouse_accel          (this->hidden->mouse_accel)
#define mouse_relative       (this->hidden->mouse_relative)
#define saved_mode           (this->hidden->saved_mode)
#define saved_view           (this->hidden->saved_view)
#define use_vidmode          (this->hidden->use_vidmode)
#define currently_fullscreen (this->hidden->currently_fullscreen)
#define switch_waiting       (this->hidden->switch_waiting)
#define switch_time          (this->hidden->switch_time)
#define blit_queued          (this->hidden->blit_queued)
#define event                (this->hidden->event)
#define SDL_BlankCursor      (this->hidden->BlankCursor)
#define captionflag          (this->hidden->captionflag)

#endif /* _SDL_x11video_h */
