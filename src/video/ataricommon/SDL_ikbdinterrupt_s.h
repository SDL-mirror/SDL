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

#ifndef _SDL_IKBDINTERRUPT_S_H_
#define _SDL_IKBDINTERRUPT_S_H_

#include <mint/osbind.h>

#include "SDL_types.h"

/* Const */

#define IKBD_JOY_UP		(1<<0)
#define IKBD_JOY_DOWN	(1<<1)
#define IKBD_JOY_LEFT	(1<<2)
#define IKBD_JOY_RIGHT	(1<<3)
#define IKBD_JOY_FIRE	(1<<7)

/* Variables */

extern Uint8  SDL_AtariIkbd_keyboard[128];	/* Keyboard table */
extern Uint16 SDL_AtariIkbd_mouseb;	/* Mouse on port 0, buttons */
extern Sint16 SDL_AtariIkbd_mousex;	/* Mouse X relative motion */
extern Sint16 SDL_AtariIkbd_mousey;	/* Mouse Y relative motion */
extern Uint16 SDL_AtariIkbd_joystick;	/* Joystick on port 1 */

extern Uint16 SDL_AtariIkbd_enabled;	/* For joystick driver to know
											if this is usable */
										
/* Functions */ 

extern void SDL_AtariIkbdInstall(void);
extern void SDL_AtariIkbdUninstall(void);

#endif /* _SDL_IKBDINTERRUPT_S_H_ */
