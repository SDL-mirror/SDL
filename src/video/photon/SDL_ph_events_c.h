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

#ifndef __SDL_PH_EVENTS_H__
#define __SDL_PH_EVENTS_H__

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif /* SAVE_RCSID */

#include "SDL_ph_video.h"

#define PH_SDL_MAX_RECTS 256
#define PH_EVENT_SAFETY_POOL 512
#define EVENT_SIZE (sizeof(PhEvent_t) + 1000 + PH_EVENT_SAFETY_POOL)

/* Functions to be exported */
extern void ph_InitOSKeymap(_THIS);
extern void ph_PumpEvents(_THIS);

#endif /* __SDL_PH_EVENTS_H__ */
