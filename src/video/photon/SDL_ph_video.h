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
  uint8_t*  volatile ptr8;
  uint16_t* volatile ptr16;
  uint32_t* volatile ptr32;
 } VidPtr_t;

typedef struct {
	unsigned char* Y;
	unsigned char* V;
	unsigned char* U;
} FRAMEDATA;

/* Private display data */
struct SDL_PrivateVideoData {
    PgDisplaySettings_t mode_settings;	
    PtWidget_t *Window;                  /* used to handle input events */
    PhImage_t *image;	                 /* used to display image       */
#ifdef HAVE_OPENGL
    PdOpenGLContext_t* OGLContext;       /* OpenGL context              */
#endif /* HAVE_OPENGL */
    PgColor_t savedpal[_Pg_MAX_PALETTE];
    PgColor_t syspalph[_Pg_MAX_PALETTE];

    struct {
        PdDirectContext_t*    direct_context;
        PdOffscreenContext_t* offscreen_context;
        PhDrawContext_t*      oldDC;
        VidPtr_t dc_ptr;
        unsigned char* CurrentFrameData;
        unsigned char* FrameData0;
        unsigned char* FrameData1;
        int current;
        long flags;
    } ocimage;

    PgHWCaps_t graphics_card_caps;  /* Graphics card caps at the moment of start   */
    int old_video_mode;             /* Stored mode before fullscreen switch        */
    int old_refresh_rate;           /* Stored refresh rate befor fullscreen switch */

    int mouse_relative;
    WMcursor* BlankCursor;

    int depth;			/* current visual depth (not bpp)        */
    int desktopbpp;             /* bpp of desktop at the moment of start */
    int desktoppal;             /* palette mode emulation or system      */

    int currently_fullscreen;

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
#define savedpal             (this->hidden->savedpal)
#define syspalph             (this->hidden->syspalph)

/* Old variable names */
#define mouse_relative       (this->hidden->mouse_relative)
#define currently_fullscreen (this->hidden->currently_fullscreen)
#define event                (this->hidden->event)
#define SDL_BlankCursor      (this->hidden->BlankCursor)

#endif /* _SDL_x11video_h */
