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

/* Utilities for getting and setting the X display mode */

#include "SDL_cgxvideo.h"

/* Define this if you want to grab the keyboard in fullscreen mode.
   If you do not define this, SDL will return from SDL_SetVideoMode()
   immediately, but will not actually go fullscreen until the window
   manager is idle.
*/
#define GRAB_FULLSCREEN

extern int CGX_GetVideoModes(_THIS);
extern SDL_Rect **CGX_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
extern void CGX_FreeVideoModes(_THIS);
extern int CGX_ResizeFullScreen(_THIS);
/*
extern void CGX_WaitMapped(_THIS, Window win);
extern void CGX_WaitUnmapped(_THIS, Window win);
extern void CGX_QueueEnterFullScreen(_THIS);
*/
extern int CGX_EnterFullScreen(_THIS);
extern int CGX_LeaveFullScreen(_THIS);
