/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#ifndef _SDL_DirectFB_video_h
#define _SDL_DirectFB_video_h

#include <directfb.h>

#include "SDL_mouse.h"
#include "SDL_sysvideo.h"

#define _THIS SDL_VideoDevice *this

/* Private display data */

struct SDL_PrivateVideoData
{
  int                    initialized;

  IDirectFB             *dfb;
  IDirectFBDisplayLayer *layer;
  IDirectFBEventBuffer  *eventbuffer;

  int nummodes;
  SDL_Rect **modelist;

  /* MGA CRTC2 support */
  int enable_mga_crtc2;
  int mga_crtc2_stretch;
  float mga_crtc2_stretch_overscan;
  IDirectFBDisplayLayer *c2layer;
  IDirectFBSurface *c2frame;
  DFBRectangle c2ssize;	/* Real screen size */
  DFBRectangle c2dsize;	/* Stretched screen size */
  DFBRectangle c2framesize;    /* CRTC2 screen size */
};

#define HIDDEN (this->hidden)

void SetDirectFBerror (const char *function, DFBResult code);

#endif /* _SDL_DirectFB_video_h */
