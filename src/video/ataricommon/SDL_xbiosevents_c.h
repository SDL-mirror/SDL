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

/*
 *	Xbios mouse & joystick vectors
 *
 *	Patrice Mandin
 */

#ifndef _SDL_ATARI_XBIOSEVENTS_H_
#define _SDL_ATARI_XBIOSEVENTS_H_

#include "SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

#define ATARI_XBIOS_MOUSEEVENTS (1<<0)
#define ATARI_XBIOS_JOYSTICKEVENTS (1<<1)

extern int SDL_AtariXbios_enabled;

extern void SDL_AtariXbios_InstallVectors(int vectors_mask);
extern void SDL_AtariXbios_RestoreVectors(void);
extern void SDL_AtariXbios_PostMouseEvents(_THIS);

#endif /* _SDL_XBIOSEVENTS_H_ */
