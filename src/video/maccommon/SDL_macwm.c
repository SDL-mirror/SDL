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

#include <stdlib.h>
#include <string.h>

#if TARGET_API_MAC_CARBON
#include <Carbon.h>
#else
#include <Windows.h>
#include <Strings.h>
#endif

#include "SDL_macwm_c.h"

void Mac_SetCaption(_THIS, const char *title, const char *icon)
{
	/* Don't convert C to P string in place, because it may be read-only */
	Str255		ptitle; /* MJS */
	ptitle[0] = strlen (title);
	memcpy(ptitle+1, title, ptitle[0]); /* MJS */
	if (SDL_Window)
		SetWTitle(SDL_Window, ptitle); /* MJS */
}
