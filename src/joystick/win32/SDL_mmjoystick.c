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

/* Win32 MultiMedia Joystick driver, contributed by Andrei de A. Formiga */

#include <stdlib.h>
#include <stdio.h>		/* For the definition of NULL */

#include "SDL_error.h"
#include "SDL_joystick.h"
#include "SDL_sysjoystick.h"
#include "SDL_joystick_c.h"

#include <windows.h>
#include <mmsystem.h>

#define MAX_JOYSTICKS	16
#define MAX_AXES	6	/* each joystick can have up to 6 axes */
#define MAX_BUTTONS	32	/* and 32 buttons                      */
#define AXIS_MIN	-32768  /* minimum value for axis coordinate */
#define AXIS_MAX	32767   /* maximum value for axis coordinate */
/* limit axis to 256 possible positions to filter out noise */
#define JOY_AXIS_THRESHOLD      (((AXIS_MAX)-(AXIS_MIN))/256)
#define JOY_BUTTON_FLAG(n)	(1<<n)


/* array to hold joystick ID values */
static UINT	SYS_JoystickID[MAX_JOYSTICKS];
static JOYCAPS	SYS_Joystick[MAX_JOYSTICKS];

/* The private structure used to keep track of a joystick */
struct joystick_hwdata
{
	/* joystick ID */
	UINT	id;

	/* values used to translate device-specific coordinates into
	   SDL-standard ranges */
	struct _transaxis {
		int offset;
		float scale;
	} transaxis[6];
};

/* Convert a win32 Multimedia API return code to a text message */
static void SetMMerror(char *function, int code);


/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int SDL_SYS_JoystickInit(void)
{
	int	i;
	int maxdevs;
	int numdevs;
	JOYINFOEX joyinfo;
	JOYCAPS	joycaps;
	MMRESULT result;

	numdevs = 0;
	maxdevs = joyGetNumDevs();

	if ( maxdevs > MAX_JOYSTICKS ) {
		maxdevs = MAX_JOYSTICKS;
	}


	for ( i = 0; i < MAX_JOYSTICKS; i++ ) {
		SYS_JoystickID[i] = JOYSTICKID1 + i;
	}


	for ( i = 0; (i < maxdevs); ++i ) {
		
		/* added 8/31/2001 By Vitaliy Mikitchenko */
		joyinfo.dwSize = sizeof(joyinfo);
		joyinfo.dwFlags = JOY_RETURNALL;
		/* end addition */

		result = joyGetPosEx(SYS_JoystickID[i], &joyinfo);
		if ( result == JOYERR_NOERROR ) {
			result = joyGetDevCaps(SYS_JoystickID[i], &joycaps, sizeof(joycaps));
			if ( result == JOYERR_NOERROR ) {
				SYS_JoystickID[numdevs] = SYS_JoystickID[i];
				SYS_Joystick[numdevs] = joycaps;
				numdevs++;
			}
		}
	}
	return(numdevs);
}

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	/***-> test for invalid index ? */
	return(SYS_Joystick[index].szPname);
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	int index, i;
	int caps_flags[MAX_AXES-2] =
		{ JOYCAPS_HASZ, JOYCAPS_HASR, JOYCAPS_HASU, JOYCAPS_HASV };
	int axis_min[MAX_AXES], axis_max[MAX_AXES];


	/* shortcut */
	index = joystick->index;
	axis_min[0] = SYS_Joystick[index].wXmin;
	axis_max[0] = SYS_Joystick[index].wXmax;
	axis_min[1] = SYS_Joystick[index].wYmin;
	axis_max[1] = SYS_Joystick[index].wYmax;
	axis_min[2] = SYS_Joystick[index].wZmin;
	axis_max[2] = SYS_Joystick[index].wZmax;
	axis_min[3] = SYS_Joystick[index].wRmin;
	axis_max[3] = SYS_Joystick[index].wRmax;
	axis_min[4] = SYS_Joystick[index].wUmin;
	axis_max[4] = SYS_Joystick[index].wUmax;
	axis_min[5] = SYS_Joystick[index].wVmin;
	axis_max[5] = SYS_Joystick[index].wVmax;

	/* allocate memory for system specific hardware data */
	joystick->hwdata = (struct joystick_hwdata *) malloc(sizeof(*joystick->hwdata));
	if (joystick->hwdata == NULL)
	{
		SDL_OutOfMemory();
		return(-1);
	}
	memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));

	/* set hardware data */
	joystick->hwdata->id = SYS_JoystickID[index];
	for ( i = 0; i < MAX_AXES; ++i ) {
		if ( (i<2) || (SYS_Joystick[index].wCaps & caps_flags[i-2]) ) {
			joystick->hwdata->transaxis[i].offset =
				AXIS_MIN - axis_min[i];
			joystick->hwdata->transaxis[i].scale =
				(float)(AXIS_MAX - AXIS_MIN) / (axis_max[i] - axis_min[i]);
		} else {
			joystick->hwdata->transaxis[i].offset = 0;
			joystick->hwdata->transaxis[i].scale = 1.0; /* Just in case */
		}
	}

	/* fill nbuttons, naxes, and nhats fields */
	joystick->nbuttons = SYS_Joystick[index].wNumButtons;
	joystick->naxes = SYS_Joystick[index].wNumAxes;
	if ( SYS_Joystick[index].wCaps & JOYCAPS_HASPOV ) {
		joystick->nhats = 1;
	} else {
		joystick->nhats = 0;
	}
	return(0);
}

static Uint8 TranslatePOV(DWORD value)
{
	Uint8 pos;

	pos = SDL_HAT_CENTERED;
	if ( value != JOY_POVCENTERED ) {
		if ( (value > JOY_POVLEFT) || (value < JOY_POVRIGHT) ) {
			pos |= SDL_HAT_UP;
		}
		if ( (value > JOY_POVFORWARD) && (value < JOY_POVBACKWARD) ) {
			pos |= SDL_HAT_RIGHT;
		}
		if ( (value > JOY_POVRIGHT) && (value < JOY_POVLEFT) ) {
			pos |= SDL_HAT_DOWN;
		}
		if ( value > JOY_POVBACKWARD ) {
			pos |= SDL_HAT_LEFT;
		}
	}
	return(pos);
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	MMRESULT result;
	int i;
	DWORD flags[MAX_AXES] = { JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, 
				  JOY_RETURNR, JOY_RETURNU, JOY_RETURNV };
	DWORD pos[MAX_AXES];
	struct _transaxis *transaxis;
	int value, change;
	JOYINFOEX joyinfo;

	joyinfo.dwSize = sizeof(joyinfo);
	joyinfo.dwFlags = JOY_RETURNALL|JOY_RETURNPOVCTS;
	if ( ! joystick->hats ) {
		joyinfo.dwFlags &= ~(JOY_RETURNPOV|JOY_RETURNPOVCTS);
	}
	result = joyGetPosEx(joystick->hwdata->id, &joyinfo);
	if ( result != JOYERR_NOERROR ) {
		SetMMerror("joyGetPosEx", result);
		return;
	}

	/* joystick motion events */
	pos[0] = joyinfo.dwXpos;
	pos[1] = joyinfo.dwYpos;
	pos[2] = joyinfo.dwZpos;
	pos[3] = joyinfo.dwRpos;
	pos[4] = joyinfo.dwUpos;
	pos[5] = joyinfo.dwVpos;

	transaxis = joystick->hwdata->transaxis;
	for (i = 0; i < joystick->naxes; i++) {
		if (joyinfo.dwFlags & flags[i]) {
			value = (int)(((float)pos[i] + transaxis[i].offset) * transaxis[i].scale);
			change = (value - joystick->axes[i]);
			if ( (change < -JOY_AXIS_THRESHOLD) || (change > JOY_AXIS_THRESHOLD) ) {
				SDL_PrivateJoystickAxis(joystick, (Uint8)i, (Sint16)value);
			}
		}
	}

	/* joystick button events */
	if ( joyinfo.dwFlags & JOY_RETURNBUTTONS ) {
		for ( i = 0; i < joystick->nbuttons; ++i ) {
			if ( joyinfo.dwButtons & JOY_BUTTON_FLAG(i) ) {
				if ( ! joystick->buttons[i] ) {
					SDL_PrivateJoystickButton(joystick, (Uint8)i, SDL_PRESSED);
				}
			} else {
				if ( joystick->buttons[i] ) {
					SDL_PrivateJoystickButton(joystick, (Uint8)i, SDL_RELEASED);
				}
			}
		}
	}

	/* joystick hat events */
	if ( joyinfo.dwFlags & JOY_RETURNPOV ) {
		Uint8 pos;

		pos = TranslatePOV(joyinfo.dwPOV);
		if ( pos != joystick->hats[0] ) {
			SDL_PrivateJoystickHat(joystick, 0, pos);
		}
	}
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	if (joystick->hwdata != NULL) {
		/* free system specific hardware data */
		free(joystick->hwdata);
	}
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
	return;
}


/* implementation functions */
void SetMMerror(char *function, int code)
{
	static char *error;
	static char  errbuf[BUFSIZ];

	errbuf[0] = 0;
	switch (code) 
	{
		case MMSYSERR_NODRIVER:
			error = "Joystick driver not present";
		break;

		case MMSYSERR_INVALPARAM:
		case JOYERR_PARMS:
			error = "Invalid parameter(s)";
		break;
		
		case MMSYSERR_BADDEVICEID:
			error = "Bad device ID";
		break;

		case JOYERR_UNPLUGGED:
			error = "Joystick not attached";
		break;

		case JOYERR_NOCANDO:
			error = "Can't capture joystick input";
		break;

		default:
			sprintf(errbuf, "%s: Unknown Multimedia system error: 0x%x",
								function, code);
		break;
	}

	if ( ! errbuf[0] ) {
		sprintf(errbuf, "%s: %s", function, error);
	}
	SDL_SetError("%s", errbuf);
}
