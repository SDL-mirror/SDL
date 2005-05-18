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

/*
	Centscreen extension definitions

	Patrice Mandin
*/

#include <stdlib.h>

#include <mint/falcon.h>

#include "SDL_xbios.h"
#include "SDL_xbios_centscreen.h"

void SDL_XBIOS_CentscreenInit(_THIS)
{
	centscreen_mode_t	curmode;

	/* Reset current mode list */
	if (XBIOS_modelist) {
		free(XBIOS_modelist);
		XBIOS_nummodes = 0;
		XBIOS_modelist = NULL;
	}

	/* Add current active mode */
	Vread(&curmode);

	SDL_XBIOS_AddMode(this, -1, curmode.physx, curmode.physy, curmode.plan,
		SDL_FALSE
	);
}
