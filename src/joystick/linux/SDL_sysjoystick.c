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

/* This is the system specific header for the SDL joystick API */

#include <stdio.h>		/* For the definition of NULL */
#include <stdlib.h>		/* For getenv() prototype */
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <limits.h>		/* For the definition of PATH_MAX */
#ifdef __arm__
#include <linux/limits.h> /* Arm cross-compiler needs this */
#endif
#include <linux/joystick.h>
#ifdef USE_INPUT_EVENTS
#include <linux/input.h>
#endif

#include "SDL_error.h"
#include "SDL_joystick.h"
#include "SDL_sysjoystick.h"
#include "SDL_joystick_c.h"

/* Special joystick configurations */
static struct {
	const char *name;
	int naxes;
	int nhats;
	int nballs;
} special_joysticks[] = {
	{ "MadCatz Panther XL", 3, 2, 1 }, /* We don't handle rudder (axis 8) */
	{ "SideWinder Precision Pro", 4, 1, 0 },
	{ "SideWinder 3D Pro", 4, 1, 0 },
	{ "Microsoft SideWinder 3D Pro", 4, 1, 0 },
	{ "Microsoft SideWinder Dual Strike USB version 1.0", 2, 1, 0 },
	{ "WingMan Interceptor", 3, 3, 0 },
	{ "WingMan Extreme Digital 3D", 4, 1, 0 },
	{ "Microsoft SideWinder Precision 2 Joystick", 4, 1, 0 },
	{ "Logitech Inc. WingMan Extreme Digital 3D", 4, 1, 0 },
	{ "Saitek Saitek X45", 6, 1, 0 }
};

/* The maximum number of joysticks we'll detect */
#define MAX_JOYSTICKS	32

/* A list of available joysticks */
static char *SDL_joylist[MAX_JOYSTICKS];

/* The private structure used to keep track of a joystick */
struct joystick_hwdata {
	int fd;
	/* The current linux joystick driver maps hats to two axes */
	struct hwdata_hat {
		int axis[2];
	} *hats;
	/* The current linux joystick driver maps balls to two axes */
	struct hwdata_ball {
		int axis[2];
	} *balls;

	/* Support for the Linux 2.4 unified input interface */
#ifdef USE_INPUT_EVENTS
	SDL_bool is_hid;
	Uint8 key_map[KEY_MAX-BTN_MISC];
	Uint8 abs_map[ABS_MAX];
	struct axis_correct {
		int used;
		int coef[3];
	} abs_correct[ABS_MAX];
#endif
};

static char *mystrdup(const char *string)
{
	char *newstring;

	newstring = (char *)malloc(strlen(string)+1);
	if ( newstring ) {
		strcpy(newstring, string);
	}
	return(newstring);
}

#ifdef USE_INPUT_EVENTS
#define test_bit(nr, addr) \
	(((1UL << ((nr) & 31)) & (((const unsigned int *) addr)[(nr) >> 5])) != 0)

static int EV_IsJoystick(int fd)
{
	unsigned long evbit[40];
	unsigned long keybit[40];
	unsigned long absbit[40];

	if ( (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) ||
	     (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) ||
	     (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) < 0) ) {
		return(0);
	}
	if (!(test_bit(EV_KEY, evbit) && test_bit(EV_ABS, evbit) &&
	      test_bit(ABS_X, absbit) && test_bit(ABS_Y, absbit) &&
	     (test_bit(BTN_TRIGGER, keybit) || test_bit(BTN_A, keybit) || test_bit(BTN_1, keybit)))) return 0;
	return(1);
}

#endif /* USE_INPUT_EVENTS */

/* Function to scan the system for joysticks */
int SDL_SYS_JoystickInit(void)
{
	/* The base path of the joystick devices */
	const char *joydev_pattern[] = {
#ifdef USE_INPUT_EVENTS
		"/dev/input/event%d",
#endif
		"/dev/input/js%d",
		"/dev/js%d"
	};
	int numjoysticks;
	int i, j;
	int fd;
	char path[PATH_MAX];
	dev_t dev_nums[MAX_JOYSTICKS];  /* major/minor device numbers */
	struct stat sb;
	int n, duplicate;

	numjoysticks = 0;

	/* First see if the user specified a joystick to use */
	if ( getenv("SDL_JOYSTICK_DEVICE") != NULL ) {
		strncpy(path, getenv("SDL_JOYSTICK_DEVICE"), sizeof(path));
		path[sizeof(path)-1] = '\0';
		if ( stat(path, &sb) == 0 ) {
			fd = open(path, O_RDONLY, 0);
			if ( fd >= 0 ) {
				/* Assume the user knows what they're doing. */
				SDL_joylist[numjoysticks] = mystrdup(path);
				if ( SDL_joylist[numjoysticks] ) {
					dev_nums[numjoysticks] = sb.st_rdev;
					++numjoysticks;
				}
				close(fd);
			}
		}
	}

	for ( i=0; i<SDL_TABLESIZE(joydev_pattern); ++i ) {
		for ( j=0; j < MAX_JOYSTICKS; ++j ) {
			sprintf(path, joydev_pattern[i], j);

			/* rcg06302000 replaced access(F_OK) call with stat().
			 * stat() will fail if the file doesn't exist, so it's
			 * equivalent behaviour.
			 */
			if ( stat(path, &sb) == 0 ) {
				/* Check to make sure it's not already in list.
				 * This happens when we see a stick via symlink.
				 */
				duplicate = 0;
				for (n=0; (n<numjoysticks) && !duplicate; ++n) {
					if ( sb.st_rdev == dev_nums[n] ) {
						duplicate = 1;
					}
				}
				if (duplicate) {
					continue;
				}

				fd = open(path, O_RDONLY, 0);
				if ( fd < 0 ) {
					continue;
				}
#ifdef USE_INPUT_EVENTS
#ifdef DEBUG_INPUT_EVENTS
				printf("Checking %s\n", path);
#endif
				if ( (i == 0) && ! EV_IsJoystick(fd) ) {
					close(fd);
					continue;
				}
#endif
				close(fd);

				/* We're fine, add this joystick */
				SDL_joylist[numjoysticks] = mystrdup(path);
				if ( SDL_joylist[numjoysticks] ) {
					dev_nums[numjoysticks] = sb.st_rdev;
					++numjoysticks;
				}
			} else
				break;
		}

#ifdef USE_INPUT_EVENTS
		/* This is a special case...
		   If the event devices are valid then the joystick devices
		   will be duplicates but without extra information about their
		   hats or balls. Unfortunately, the event devices can't
		   currently be calibrated, so it's a win-lose situation.
		   So : /dev/input/eventX = /dev/input/jsY = /dev/jsY
		*/
		if ( (i == 0) && (numjoysticks > 0) )
			break;
#endif
	}

	return(numjoysticks);
}

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	int fd;
	static char namebuf[128];
	char *name;

	name = NULL;
	fd = open(SDL_joylist[index], O_RDONLY, 0);
	if ( fd >= 0 ) {
		if ( 
#ifdef USE_INPUT_EVENTS
		     (ioctl(fd, EVIOCGNAME(sizeof(namebuf)), namebuf) <= 0) &&
#endif
		     (ioctl(fd, JSIOCGNAME(sizeof(namebuf)), namebuf) <= 0) ) {
			name = SDL_joylist[index];
		} else {
			name = namebuf;
		}
		close(fd);
	}
	return name;
}

static int allocate_hatdata(SDL_Joystick *joystick)
{
	int i;

	joystick->hwdata->hats = (struct hwdata_hat *)malloc(
		joystick->nhats * sizeof(struct hwdata_hat));
	if ( joystick->hwdata->hats == NULL ) {
		return(-1);
	}
	for ( i=0; i<joystick->nhats; ++i ) {
		joystick->hwdata->hats[i].axis[0] = 1;
		joystick->hwdata->hats[i].axis[1] = 1;
	}
	return(0);
}

static int allocate_balldata(SDL_Joystick *joystick)
{
	int i;

	joystick->hwdata->balls = (struct hwdata_ball *)malloc(
		joystick->nballs * sizeof(struct hwdata_ball));
	if ( joystick->hwdata->balls == NULL ) {
		return(-1);
	}
	for ( i=0; i<joystick->nballs; ++i ) {
		joystick->hwdata->balls[i].axis[0] = 0;
		joystick->hwdata->balls[i].axis[1] = 0;
	}
	return(0);
}

static SDL_bool JS_ConfigJoystick(SDL_Joystick *joystick, int fd)
{
	SDL_bool handled;
	unsigned char n;
	int old_axes, tmp_naxes, tmp_nhats, tmp_nballs;
	const char *name;
	char *env, env_name[128];
	int i;

	handled = SDL_FALSE;

	/* Default joystick device settings */
	if ( ioctl(fd, JSIOCGAXES, &n) < 0 ) {
		joystick->naxes = 2;
	} else {
		joystick->naxes = n;
	}
	if ( ioctl(fd, JSIOCGBUTTONS, &n) < 0 ) {
		joystick->nbuttons = 2;
	} else {
		joystick->nbuttons = n;
	}

	name = SDL_SYS_JoystickName(joystick->index);
	old_axes = joystick->naxes;

	/* Generic analog joystick support */
	if ( strstr(name, "Analog") == name && strstr(name, "-hat") ) {
		if ( sscanf(name,"Analog %d-axis %*d-button %d-hat",
			&tmp_naxes, &tmp_nhats) == 2 ) {

			joystick->naxes = tmp_naxes;
			joystick->nhats = tmp_nhats;

			handled = SDL_TRUE;
		}
	}

	/* Special joystick support */
	for ( i=0; i < SDL_TABLESIZE(special_joysticks); ++i ) {
		if ( strcmp(name, special_joysticks[i].name) == 0 ) {

			joystick->naxes = special_joysticks[i].naxes;
			joystick->nhats = special_joysticks[i].nhats;
			joystick->nballs = special_joysticks[i].nballs;

			handled = SDL_TRUE;
			break;
		}
	}

	/* User environment joystick support */
	if ( (env = getenv("SDL_LINUX_JOYSTICK")) ) {
		strcpy(env_name, "");
		if ( *env == '\'' && sscanf(env, "'%[^']s'", env_name) == 1 )
			env += strlen(env_name)+2;
		else if ( sscanf(env, "%s", env_name) == 1 )
			env += strlen(env_name);

		if ( strcmp(name, env_name) == 0 ) {

			if ( sscanf(env, "%d %d %d", &tmp_naxes, &tmp_nhats,
				&tmp_nballs) == 3 ) {

				joystick->naxes = tmp_naxes;
				joystick->nhats = tmp_nhats;
				joystick->nballs = tmp_nballs;

				handled = SDL_TRUE;
			}
		}
	}

	/* Remap hats and balls */
	if (handled) {
		if ( joystick->nhats > 0 ) {
			if ( allocate_hatdata(joystick) < 0 ) {
				joystick->nhats = 0;
			}
		}
		if ( joystick->nballs > 0 ) {
			if ( allocate_balldata(joystick) < 0 ) {
				joystick->nballs = 0;
			}
		}
	}

	return(handled);
}

#ifdef USE_INPUT_EVENTS

static SDL_bool EV_ConfigJoystick(SDL_Joystick *joystick, int fd)
{
	int i;
	unsigned long keybit[40];
	unsigned long absbit[40];
	unsigned long relbit[40];

	/* See if this device uses the new unified event API */
	if ( (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) &&
	     (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) >= 0) &&
	     (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit) >= 0) ) {
		joystick->hwdata->is_hid = SDL_TRUE;

		/* Get the number of buttons, axes, and other thingamajigs */
		for ( i=BTN_JOYSTICK; i < KEY_MAX; ++i ) {
			if ( test_bit(i, keybit) ) {
#ifdef DEBUG_INPUT_EVENTS
				printf("Joystick has button: 0x%x\n", i);
#endif
				joystick->hwdata->key_map[i-BTN_MISC] =
						joystick->nbuttons;
				++joystick->nbuttons;
			}
		}
		for ( i=BTN_MISC; i < BTN_JOYSTICK; ++i ) {
			if ( test_bit(i, keybit) ) {
#ifdef DEBUG_INPUT_EVENTS
				printf("Joystick has button: 0x%x\n", i);
#endif
				joystick->hwdata->key_map[i-BTN_MISC] =
						joystick->nbuttons;
				++joystick->nbuttons;
			}
		}
		for ( i=0; i<ABS_MAX; ++i ) {
			/* Skip hats */
			if ( i == ABS_HAT0X ) {
				i = ABS_HAT3Y;
				continue;
			}
			if ( test_bit(i, absbit) ) {
				int values[5];

				ioctl(fd, EVIOCGABS(i), values);
#ifdef DEBUG_INPUT_EVENTS
				printf("Joystick has absolute axis: %x\n", i);
				printf("Values = { %d, %d, %d, %d, %d }\n",
					values[0], values[1],
					values[2], values[3], values[4]);
#endif /* DEBUG_INPUT_EVENTS */
				joystick->hwdata->abs_map[i] = joystick->naxes;
				if ( values[1] == values[2] ) {
				    joystick->hwdata->abs_correct[i].used = 0;
				} else {
				    joystick->hwdata->abs_correct[i].used = 1;
				    joystick->hwdata->abs_correct[i].coef[0] =
					(values[2] + values[1]) / 2 - values[4];
				    joystick->hwdata->abs_correct[i].coef[1] =
					(values[2] + values[1]) / 2 + values[4];
				    joystick->hwdata->abs_correct[i].coef[2] =
					(1 << 29) / ((values[2] - values[1]) / 2 - 2 * values[4]);
				}
				++joystick->naxes;
			}
		}
		for ( i=ABS_HAT0X; i <= ABS_HAT3Y; i += 2 ) {
			if ( test_bit(i, absbit) || test_bit(i+1, absbit) ) {
#ifdef DEBUG_INPUT_EVENTS
				printf("Joystick has hat %d\n",(i-ABS_HAT0X)/2);
#endif
				++joystick->nhats;
			}
		}
		if ( test_bit(REL_X, relbit) || test_bit(REL_Y, relbit) ) {
			++joystick->nballs;
		}

		/* Allocate data to keep track of these thingamajigs */
		if ( joystick->nhats > 0 ) {
			if ( allocate_hatdata(joystick) < 0 ) {
				joystick->nhats = 0;
			}
		}
		if ( joystick->nballs > 0 ) {
			if ( allocate_balldata(joystick) < 0 ) {
				joystick->nballs = 0;
			}
		}
	}
	return(joystick->hwdata->is_hid);
}

#endif /* USE_INPUT_EVENTS */

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	int fd;

	/* Open the joystick and set the joystick file descriptor */
	fd = open(SDL_joylist[joystick->index], O_RDONLY, 0);
	if ( fd < 0 ) {
		SDL_SetError("Unable to open %s\n",
		             SDL_joylist[joystick->index]);
		return(-1);
	}
	joystick->hwdata = (struct joystick_hwdata *)
	                   malloc(sizeof(*joystick->hwdata));
	if ( joystick->hwdata == NULL ) {
		SDL_OutOfMemory();
		close(fd);
		return(-1);
	}
	memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));
	joystick->hwdata->fd = fd;

	/* Set the joystick to non-blocking read mode */
	fcntl(fd, F_SETFL, O_NONBLOCK);

	/* Get the number of buttons and axes on the joystick */
#ifdef USE_INPUT_EVENTS
	if ( ! EV_ConfigJoystick(joystick, fd) )
#endif
		JS_ConfigJoystick(joystick, fd);

	return(0);
}

static __inline__
void HandleHat(SDL_Joystick *stick, Uint8 hat, int axis, int value)
{
	struct hwdata_hat *the_hat;
	const Uint8 position_map[3][3] = {
		{ SDL_HAT_LEFTUP, SDL_HAT_UP, SDL_HAT_RIGHTUP },
		{ SDL_HAT_LEFT, SDL_HAT_CENTERED, SDL_HAT_RIGHT },
		{ SDL_HAT_LEFTDOWN, SDL_HAT_DOWN, SDL_HAT_RIGHTDOWN }
	};

	the_hat = &stick->hwdata->hats[hat];
	if ( value < 0 ) {
		value = 0;
	} else
	if ( value == 0 ) {
		value = 1;
	} else
	if ( value > 0 ) {
		value = 2;
	}
	if ( value != the_hat->axis[axis] ) {
		the_hat->axis[axis] = value;
		SDL_PrivateJoystickHat(stick, hat,
			position_map[the_hat->axis[1]][the_hat->axis[0]]);
	}
}

static __inline__
void HandleBall(SDL_Joystick *stick, Uint8 ball, int axis, int value)
{
	stick->hwdata->balls[ball].axis[axis] += value;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
static __inline__ void JS_HandleEvents(SDL_Joystick *joystick)
{
	struct js_event events[32];
	int i, len;
	Uint8 other_axis;

	while ((len=read(joystick->hwdata->fd, events, (sizeof events))) > 0) {
		len /= sizeof(events[0]);
		for ( i=0; i<len; ++i ) {
			switch (events[i].type & ~JS_EVENT_INIT) {
			    case JS_EVENT_AXIS:
				if ( events[i].number < joystick->naxes ) {
					SDL_PrivateJoystickAxis(joystick,
				           events[i].number, events[i].value);
					break;
				}
				events[i].number -= joystick->naxes;
				other_axis = (events[i].number / 2);
				if ( other_axis < joystick->nhats ) {
					HandleHat(joystick, other_axis,
						events[i].number%2,
						events[i].value);
					break;
				}
				events[i].number -= joystick->nhats*2;
				other_axis = (events[i].number / 2);
				if ( other_axis < joystick->nballs ) {
					HandleBall(joystick, other_axis,
						events[i].number%2,
						events[i].value);
					break;
				}
				break;
			    case JS_EVENT_BUTTON:
				SDL_PrivateJoystickButton(joystick,
				           events[i].number, events[i].value);
				break;
			    default:
				/* ?? */
				break;
			}
		}
	}
}
#ifdef USE_INPUT_EVENTS
static __inline__ int EV_AxisCorrect(SDL_Joystick *joystick, int which, int value)
{
	struct axis_correct *correct;

	correct = &joystick->hwdata->abs_correct[which];
	if ( correct->used ) {
		if ( value > correct->coef[0] ) {
			if ( value < correct->coef[1] ) {
				return 0;
			}
			value -= correct->coef[1];
		} else {
			value -= correct->coef[0];
		}
		value *= correct->coef[2];
		value >>= 14;
	}

	/* Clamp and return */
	if ( value < -32767 ) return -32767;
	if ( value >  32767 ) return  32767;

	return value;
}

static __inline__ void EV_HandleEvents(SDL_Joystick *joystick)
{
	struct input_event events[32];
	int i, len;
	int code;

	while ((len=read(joystick->hwdata->fd, events, (sizeof events))) > 0) {
		len /= sizeof(events[0]);
		for ( i=0; i<len; ++i ) {
			code = events[i].code;
			switch (events[i].type) {
			    case EV_KEY:
				if ( code >= BTN_MISC ) {
					code -= BTN_MISC;
					SDL_PrivateJoystickButton(joystick,
				           joystick->hwdata->key_map[code],
					   events[i].value);
				}
				break;
			    case EV_ABS:
				switch (code) {
				    case ABS_HAT0X:
				    case ABS_HAT0Y:
				    case ABS_HAT1X:
				    case ABS_HAT1Y:
				    case ABS_HAT2X:
				    case ABS_HAT2Y:
				    case ABS_HAT3X:
				    case ABS_HAT3Y:
					code -= ABS_HAT0X;
					HandleHat(joystick, code/2, code%2,
							events[i].value);
					break;
				    default:
					events[i].value = EV_AxisCorrect(joystick, code, events[i].value);
					SDL_PrivateJoystickAxis(joystick,
				           joystick->hwdata->abs_map[code],
					   events[i].value);
					break;
				}
				break;
			    case EV_REL:
				switch (code) {
				    case REL_X:
				    case REL_Y:
					code -= REL_X;
					HandleBall(joystick, code/2, code%2,
							events[i].value);
					break;
				    default:
					break;
				}
				break;
			    default:
				break;
			}
		}
	}
}
#endif /* USE_INPUT_EVENTS */

void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	int i;
	
#ifdef USE_INPUT_EVENTS
	if ( joystick->hwdata->is_hid )
		EV_HandleEvents(joystick);
	else
#endif
		JS_HandleEvents(joystick);

	/* Deliver ball motion updates */
	for ( i=0; i<joystick->nballs; ++i ) {
		int xrel, yrel;

		xrel = joystick->hwdata->balls[i].axis[0];
		yrel = joystick->hwdata->balls[i].axis[1];
		if ( xrel || yrel ) {
			joystick->hwdata->balls[i].axis[0] = 0;
			joystick->hwdata->balls[i].axis[1] = 0;
			SDL_PrivateJoystickBall(joystick, (Uint8)i, xrel, yrel);
		}
	}
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	if ( joystick->hwdata ) {
		close(joystick->hwdata->fd);
		if ( joystick->hwdata->hats ) {
			free(joystick->hwdata->hats);
		}
		if ( joystick->hwdata->balls ) {
			free(joystick->hwdata->balls);
		}
		free(joystick->hwdata);
		joystick->hwdata = NULL;
	}
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
	int i;

	for ( i=0; SDL_joylist[i]; ++i ) {
		free(SDL_joylist[i]);
	}
	SDL_joylist[0] = NULL;
}

