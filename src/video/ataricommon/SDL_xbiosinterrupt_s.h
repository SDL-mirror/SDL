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
 *	Mouse vector
 *
 *	Patrice Mandin
 */

#ifndef _SDL_XBIOSINTERRUPT_S_H_
#define _SDL_XBIOSINTERRUPT_S_H_

#include <mint/osbind.h>

#include "SDL_types.h"

/* Variables */

extern Uint16 SDL_AtariXbios_mouseb;	/* buttons */
extern Sint16 SDL_AtariXbios_mousex;	/* X relative motion */
extern Sint16 SDL_AtariXbios_mousey;	/* Y relative motion */
extern Uint16 SDL_AtariXbios_joystick;	/* Joystick */

/* Functions */ 

extern void SDL_AtariXbios_Install(_KBDVECS *kbdvecs,void *newmousevector,void *newjoystickvector);
extern void SDL_AtariXbios_Restore(_KBDVECS *kbdvecs);
extern void SDL_AtariXbios_MouseVector(void *buf);
extern void SDL_AtariXbios_JoystickVector(void *buf);

#endif /* _SDL_XBIOSINTERRUPT_S_H_ */
