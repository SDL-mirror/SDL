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

#ifndef __SDL_PH_WM_H__
#define __SDL_PH_WM_H__

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif /* SAVE_RCSID */

#include "SDL_ph_video.h"

/* Functions to be exported */
extern void ph_SetCaption(_THIS, const char *title, const char *icon);
extern void ph_SetIcon(_THIS, SDL_Surface *icon, Uint8 *mask);
extern int ph_IconifyWindow(_THIS);
extern SDL_GrabMode ph_GrabInputNoLock(_THIS, SDL_GrabMode mode);
extern SDL_GrabMode ph_GrabInput(_THIS, SDL_GrabMode mode);
extern int ph_GetWMInfo(_THIS, SDL_SysWMinfo *info);

#endif /* __SDL_PH_WM_H__ */
