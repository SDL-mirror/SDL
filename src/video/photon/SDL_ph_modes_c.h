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
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif


#ifndef _PH_MODES_INCLUDED_
#define _PH_MODES_INCLUDED_

#include "SDL_ph_video.h"

#define PH_MAX_VIDEOMODES 127

//extern int ph_GetVideoModes(_THIS);
extern SDL_Rect **ph_ListModes(_THIS,SDL_PixelFormat *format, Uint32 flags);
extern void ph_FreeVideoModes(_THIS);
extern int ph_ResizeFullScreen(_THIS);
extern void ph_WaitMapped(_THIS);
extern void ph_WaitUnmapped(_THIS);
extern void ph_QueueEnterFullScreen(_THIS);
extern int ph_EnterFullScreen(_THIS);
extern int ph_LeaveFullScreen(_THIS);
extern int get_mode(int width, int height, int bpp);
extern int get_mode_any_format(int width, int height, int bpp);
extern int ph_ToggleFullScreen(_THIS, int on);

#endif /* _PH_MODES_INCLUDED_ */
