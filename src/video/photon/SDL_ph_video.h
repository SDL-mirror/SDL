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
static PhEvent_t *event;
static PtAppContext_t app;
static PtWidget_t *window;
static PgVideoModes_t modelist;
static PdOffscreenContext_t *Buff[2];
static PdDirectContext_t *directContext;
static PhRect_t screenRect,windowRect;
static PgColor_t currRGB;
static PhPoint_t zeroPoint;
static char keyque[ QUE_SIZE ],keyMatrix[256];
static int queput,queget;
static int modeSet;
static PgHWCaps_t hwCaps;
static PgDisplaySettings_t mode_settings;
static int rshift1,rshift2,gshift1,gshift2,bshift1,bshift2;
static int backPitch;
static unsigned RBitMask,GBitMask,BBitMask;
static unsigned TranslatedFillColor;
*/

#ifndef _SDL_ph_video_h
#define _SDL_ph_video_h

#include "SDL_mouse.h"
#include "SDL_sysvideo.h"

#include "Ph.h"
#include "Pt.h"
#include <photon/Pg.h>
#include <photon/PdDirect.h>

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

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
    int local_ph;		/* Flag: true if local display */
    PtAppContext_t app;
    PgDisplaySettings_t mode_settings;	
    PtWidget_t *window;    /* used to handle input events */
	PhImage_t *image;	 /* used to display image */

 	struct {
       PdDirectContext_t *direct_context;
		PdOffscreenContext_t *offscreen_context;
		VidPtr_t    dc_ptr;
		FRAMEDATA *CurrentFrameData;
		FRAMEDATA *FrameData0;
		FRAMEDATA *FrameData1;
		int current;
		long Stride;
		long flags;
	} ocimage;

    PhDrawContext_t *ScreenDC; //=NULL;
	signed short old_video_mode; //=-1;
	signed short old_refresh_rate; //=-1;
	PgHWCaps_t graphics_card_caps;
	
	PdDirectContext_t *directContext;
	PdOffscreenContext_t *Buff[2];
	struct _Ph_ctrl* ctrl_channel;
	
	PhGC_t *Pt_GC, *Pg_GC; /* Graphic contexts to switch between Pt and Pg APIs */

    /* The variables used for displaying graphics */

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

  

    int depth;			/* current visual depth (not bpp) */

    int use_vidmode;
    int currently_fullscreen;

    /* Automatic mode switching support (entering/leaving fullscreen) */
    Uint32 switch_waiting;
    Uint32 switch_time;

    /* Prevent too many XSync() calls */
    int blit_queued;

    short *iconcolors;		/* List of colors used by the icon */
	PhEvent_t* event;
};

#define local_ph 		(this->hidden->local_ph)
#define app				(this->hidden->app)
#define mode_settings	(this->hidden->mode_settings)
#define window			(this->hidden->window)
#define directContext	(this->hidden->directContext)
#define Buff			(this->hidden->Buff)
#define ctrl_channel 	(this->hidden->ctrl_channel)
#define SDL_Image		(this->hidden->image)
#define OCImage		(this->hidden->ocimage)
#define old_video_mode		(this->hidden->old_video_mode)
#define old_refresh_rate		(this->hidden->old_refresh_rate)
#define graphics_card_caps		(this->hidden->graphics_card_caps)
#define Pt_GC			(this->hidden->Pt_GC)
#define Pg_GC			(this->hidden->Pg_GC)

/* Old variable names */
#define swap_pixels		(this->hidden->swap_pixels)
#define current_w		(this->hidden->current_w)
#define current_h		(this->hidden->current_h)
#define mouse_last		(this->hidden->mouse_last)
#define mouse_accel		(this->hidden->mouse_accel)
#define mouse_relative		(this->hidden->mouse_relative)
#define saved_mode		(this->hidden->saved_mode)
#define saved_view		(this->hidden->saved_view)
#define use_vidmode		(this->hidden->use_vidmode)
#define currently_fullscreen	(this->hidden->currently_fullscreen)
#define switch_waiting		(this->hidden->switch_waiting)
#define switch_time		(this->hidden->switch_time)
#define blit_queued		(this->hidden->blit_queued)
#define SDL_iconcolorIs		(this->hidden->iconcolors)
#define event				(this->hidden->event)
#define SDL_BlankCursor     (this->hidden->BlankCursor)

#endif /* _SDL_x11video_h */
