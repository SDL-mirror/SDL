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

/*
 *	GEM SDL video driver implementation
 *	Window manager functions
 * 
 *	Patrice Mandin
 */

/* Mint includes */
#include <gem.h>

#include "SDL_gemwm_c.h"

/* Defines */

#define ICONWIDTH 64
#define ICONHEIGHT 64

/* Functions */

void GEM_SetCaption(_THIS, const char *title, const char *icon)
{
	short parm[4];
	const char *new_name;

	new_name = NULL;

	if (title)
		GEM_title_name = title;

	if (icon)
		GEM_icon_name = icon;

	/* Is window iconified ? */
	parm[0]=0;
	if (GEM_wfeatures & (1<<WF_ICONIFY))
		wind_get(GEM_handle, WF_ICONIFY, &parm[0], &parm[1], &parm[2], &parm[3]);

	if (parm[0]==0) {
		/* Change window name */
		if (title)
			new_name = title;
	} else {
		/* Change icon name */
		if (icon)
			new_name = icon;
	}

	parm[0]= ((unsigned long) new_name)>>16;
	parm[1]= ((unsigned long) new_name) & 65535;

	if (new_name) {
		wind_set(GEM_handle, WF_NAME, parm[0], parm[1], 0, 0);
	}
}

void GEM_SetIcon(_THIS, SDL_Surface *icon, Uint8 *mask)
{
	if ((GEM_wfeatures & (1<<WF_ICONIFY))==0)
		return;

	/* Todo */
}

int GEM_IconifyWindow(_THIS)
{
	short message[8];
	
	if ((GEM_wfeatures & (1<<WF_ICONIFY))==0)
		return 0;

	message[0] = WM_ICONIFY;
	message[1] = GEM_ap_id;
	message[2] = 0;
	message[3] = GEM_handle;
	message[4] = 0;
	message[5] = GEM_desk_h-ICONHEIGHT;
	message[6] = ICONWIDTH;
	message[7] = ICONHEIGHT;

	appl_write(GEM_ap_id, sizeof(message), message);

	return 1;
}

SDL_GrabMode GEM_GrabInput(_THIS, SDL_GrabMode mode)
{
	return mode;
}
