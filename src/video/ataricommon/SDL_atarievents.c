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
 *	Atari keyboard events manager
 *
 *	Patrice Mandin
 *
 *	This routines choose what the final event manager will be
 */

#include <stdlib.h>
#include <string.h>

#include <mint/cookie.h>

#include "SDL.h"
#include "SDL_sysevents.h"
#include "SDL_events_c.h"

#include "SDL_atarievents_c.h"
#include "SDL_biosevents_c.h"
#include "SDL_gemdosevents_c.h"
#include "SDL_ikbdevents_c.h"

enum {
	MCH_ST=0,
	MCH_STE,
	MCH_TT,
	MCH_F30
};

void (*Atari_ShutdownEvents)(void);

static void Atari_InitializeEvents(_THIS)
{
	const char *envr;
	unsigned long cookie_mch;

	/* Test if we are on an Atari machine or not */
	if (Getcookie(C__MCH, &cookie_mch) == C_NOTFOUND) {
		cookie_mch = 0;
	}
	cookie_mch >>= 16;

	/* Default is Ikbd, the faster except for clones */
	switch(cookie_mch) {
		case MCH_ST:
		case MCH_STE:
		case MCH_TT:
		case MCH_F30:
			this->InitOSKeymap=AtariIkbd_InitOSKeymap;
			this->PumpEvents=AtariIkbd_PumpEvents;
			Atari_ShutdownEvents=AtariIkbd_ShutdownEvents;
			break;
		default:
			this->InitOSKeymap=AtariGemdos_InitOSKeymap;
			this->PumpEvents=AtariGemdos_PumpEvents;
			Atari_ShutdownEvents=AtariGemdos_ShutdownEvents;
			break;
	}

	envr = getenv("SDL_ATARI_EVENTSDRIVER");

 	if (!envr) {
		return;
	}

	if (strcmp(envr, "ikbd") == 0) {
		this->InitOSKeymap=AtariIkbd_InitOSKeymap;
		this->PumpEvents=AtariIkbd_PumpEvents;
		Atari_ShutdownEvents=AtariIkbd_ShutdownEvents;
	}

	if (strcmp(envr, "gemdos") == 0) {
		this->InitOSKeymap=AtariGemdos_InitOSKeymap;
		this->PumpEvents=AtariGemdos_PumpEvents;
		Atari_ShutdownEvents=AtariGemdos_ShutdownEvents;
	}

	if (strcmp(envr, "bios") == 0) {
		this->InitOSKeymap=AtariBios_InitOSKeymap;
		this->PumpEvents=AtariBios_PumpEvents;
		Atari_ShutdownEvents=AtariBios_ShutdownEvents;
	}
}

void Atari_InitOSKeymap(_THIS)
{
	Atari_InitializeEvents(this);

	/* Call choosen routine */
	this->InitOSKeymap(this);
}

void Atari_PumpEvents(_THIS)
{
	Atari_InitializeEvents(this);

	/* Call choosen routine */
	this->PumpEvents(this);
}
