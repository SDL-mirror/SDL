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
    slouken@devolution.com
*/

/*
 * Joystick driver for the uhid(4) interface found in OpenBSD,
 * NetBSD and FreeBSD.
 *
 * Maintainer: <vedge at csoft.org>
 */

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#if defined(HAVE_USB_H)
#include <usb.h>
#endif
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#if defined(HAVE_USBHID_H)
#include <usbhid.h>
#elif defined(HAVE_LIBUSB_H)
#include <libusb.h>
#elif defined(HAVE_LIBUSBHID_H)
#include <libusbhid.h>
#endif

#ifdef __FreeBSD__
#include <osreldate.h>
#include <sys/joystick.h>
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <machine/joystick.h>
#endif

#include "SDL_error.h"
#include "SDL_joystick.h"
#include "SDL_sysjoystick.h"
#include "SDL_joystick_c.h"

#define MAX_UHID_JOYS	4
#define MAX_JOY_JOYS	2
#define MAX_JOYS	(MAX_UHID_JOYS + MAX_JOY_JOYS)

struct report {
	struct	usb_ctl_report *buf;	/* Buffer */
	size_t	size;			/* Buffer size */
	int	rid;			/* Report ID */
	enum {
		SREPORT_UNINIT,
		SREPORT_CLEAN,
		SREPORT_DIRTY
	} status;
};

static struct {
	int	uhid_report;
	hid_kind_t kind;
	const	char *name;
} const repinfo[] = {
	{ UHID_INPUT_REPORT,	hid_input,	"input" },
	{ UHID_OUTPUT_REPORT,	hid_output,	"output" },
	{ UHID_FEATURE_REPORT,	hid_feature,	"feature" }
};

enum {
	REPORT_INPUT = 0,
	REPORT_OUTPUT = 1,
	REPORT_FEATURE = 2
};

enum {
	JOYAXE_X,
	JOYAXE_Y,
	JOYAXE_Z,
	JOYAXE_SLIDER,
	JOYAXE_WHEEL,
	JOYAXE_RX,
	JOYAXE_RY,
	JOYAXE_RZ,
	JOYAXE_count
};

struct joystick_hwdata {
	int	fd;
	char	*path;
	enum {
		BSDJOY_UHID,	/* uhid(4) */
		BSDJOY_JOY	/* joy(4) */
	} type;
	struct	report_desc *repdesc;
	struct	report inreport;
	int	axis_map[JOYAXE_count];	/* map present JOYAXE_* to 0,1,..*/
};

static char *joynames[MAX_JOYS];
static char *joydevnames[MAX_JOYS];

static int	report_alloc(struct report *, struct report_desc *, int);
static void	report_free(struct report *);

#ifdef USBHID_UCR_DATA
#define REP_BUF_DATA(rep) ((rep)->buf->ucr_data)
#else
#define REP_BUF_DATA(rep) ((rep)->buf->data)
#endif

int
SDL_SYS_JoystickInit(void)
{
	char s[16];
	int i, fd;

	SDL_numjoysticks = 0;

	memset(joynames, 0, sizeof(joynames));
	memset(joydevnames, 0, sizeof(joydevnames));

	for (i = 0; i < MAX_UHID_JOYS; i++) {
		SDL_Joystick nj;

		sprintf(s, "/dev/uhid%d", i);

		nj.index = SDL_numjoysticks;
		joynames[nj.index] = strdup(s);

		if (SDL_SYS_JoystickOpen(&nj) == 0) {
			SDL_SYS_JoystickClose(&nj);
			SDL_numjoysticks++;
		} else {
			free(joynames[nj.index]);
		}
	}
	for (i = 0; i < MAX_JOY_JOYS; i++) {
		sprintf(s, "/dev/joy%d", i);
		fd = open(s, O_RDONLY);
		if (fd != -1) {
			joynames[SDL_numjoysticks++] = strdup(s);
			close(fd);
		}
	}

	/* Read the default USB HID usage table. */
	hid_init(NULL);

	return (SDL_numjoysticks);
}

const char *
SDL_SYS_JoystickName(int index)
{
	if (joydevnames[index] != NULL) {
		return (joydevnames[index]);
	}
	return (joynames[index]);
}

static int
usage_to_joyaxe(unsigned usage)
{
    int joyaxe;
    switch (usage) {
    case HUG_X:
	joyaxe = JOYAXE_X; break;
    case HUG_Y:
	joyaxe = JOYAXE_Y; break;
    case HUG_Z:
	joyaxe = JOYAXE_Z; break;
    case HUG_SLIDER:
	joyaxe = JOYAXE_SLIDER; break;
    case HUG_WHEEL:
	joyaxe = JOYAXE_WHEEL; break;
    case HUG_RX:
	joyaxe = JOYAXE_RX; break;
    case HUG_RY:
	joyaxe = JOYAXE_RY; break;
    case HUG_RZ:
	joyaxe = JOYAXE_RZ; break;
    default:
	joyaxe = -1;
    }
    return joyaxe;    
}

static unsigned
hatval_to_sdl(Sint32 hatval)
{
    static const unsigned hat_dir_map[8] = {
	SDL_HAT_UP, SDL_HAT_RIGHTUP, SDL_HAT_RIGHT, SDL_HAT_RIGHTDOWN, 
	SDL_HAT_DOWN, SDL_HAT_LEFTDOWN, SDL_HAT_LEFT, SDL_HAT_LEFTUP
    };
    unsigned result;
    if ((hatval & 7) == hatval) 
	result = hat_dir_map[hatval];
    else 
	result = SDL_HAT_CENTERED;
    return result;
}


int
SDL_SYS_JoystickOpen(SDL_Joystick *joy)
{
	char *path = joynames[joy->index];
	struct joystick_hwdata *hw;
	struct hid_item hitem;
	struct hid_data *hdata;
	struct report *rep;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		SDL_SetError("%s: %s", path, strerror(errno));
		return (-1);
	}

	hw = (struct joystick_hwdata *)malloc(sizeof(struct joystick_hwdata));
	if (hw == NULL) {
		SDL_OutOfMemory();
		close(fd);
		return (-1);
	}
	joy->hwdata = hw;
	hw->fd = fd;
	hw->path = strdup(path);
	if (! strncmp(path, "/dev/joy", 8)) {
		hw->type = BSDJOY_JOY;
		joy->naxes = 2;
		joy->nbuttons = 2;
		joy->nhats = 0;
		joy->nballs = 0;
		joydevnames[joy->index] = strdup("Gameport joystick");
		goto usbend;
	} else {
		hw->type = BSDJOY_UHID;
	}

	{
	    int ax;
	    for (ax = 0; ax < JOYAXE_count; ax++)
		hw->axis_map[ax] = -1;
	}
	hw->repdesc = hid_get_report_desc(fd);
	if (hw->repdesc == NULL) {
		SDL_SetError("%s: USB_GET_REPORT_DESC: %s", hw->path,
		    strerror(errno));
		goto usberr;
	}

	rep = &hw->inreport;
	rep->rid = 0;
	if (report_alloc(rep, hw->repdesc, REPORT_INPUT) < 0) {
		goto usberr;
	}
	if (rep->size <= 0) {
		SDL_SetError("%s: Input report descriptor has invalid length",
		    hw->path);
		goto usberr;
	}

#if defined(USBHID_NEW) || (defined(__FreeBSD__) && __FreeBSD_version >= 500111)
	hdata = hid_start_parse(hw->repdesc, 1 << hid_input, rep->rid);
#else
	hdata = hid_start_parse(hw->repdesc, 1 << hid_input);
#endif
	if (hdata == NULL) {
		SDL_SetError("%s: Cannot start HID parser", hw->path);
		goto usberr;
	}
	joy->naxes = 0;
	joy->nbuttons = 0;
	joy->nhats = 0;
	joy->nballs = 0;

	while (hid_get_item(hdata, &hitem) > 0) {
		char *sp;
		const char *s;

		switch (hitem.kind) {
		case hid_collection:
			switch (HID_PAGE(hitem.usage)) {
			case HUP_GENERIC_DESKTOP:
				switch (HID_USAGE(hitem.usage)) {
				case HUG_JOYSTICK:
				case HUG_GAME_PAD:
					s = hid_usage_in_page(hitem.usage);
					sp = malloc(strlen(s) + 5);
					sprintf(sp, "%s (%d)", s,
					    joy->index);
					joydevnames[joy->index] = sp;
				}
			}
			break;
		case hid_input:
			switch (HID_PAGE(hitem.usage)) {
			case HUP_GENERIC_DESKTOP: {
			    unsigned usage = HID_USAGE(hitem.usage);
			    int joyaxe = usage_to_joyaxe(usage);
			    if (joyaxe >= 0) {
				hw->axis_map[joyaxe] = joy->naxes;
				joy->naxes++;
			    } else if (usage == HUG_HAT_SWITCH) {
				joy->nhats++;
			    }
			    break;
			}
			case HUP_BUTTON:
				joy->nbuttons++;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	hid_end_parse(hdata);

usbend:
	/* The poll blocks the event thread. */
	fcntl(fd, F_SETFL, O_NONBLOCK);

	return (0);
usberr:
	close(hw->fd);
	free(hw->path);
	free(hw);
	return (-1);
}

void
SDL_SYS_JoystickUpdate(SDL_Joystick *joy)
{
	struct hid_item hitem;
	struct hid_data *hdata;
	struct report *rep;
	int nbutton, naxe = -1;
	Sint32 v;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	struct joystick gameport;
	static int x, y, xmin = 0xffff, ymin = 0xffff, xmax = 0, ymax = 0;
 
	if (joy->hwdata->type == BSDJOY_JOY) {
		if (read(joy->hwdata->fd, &gameport, sizeof gameport) != sizeof gameport)
			return;
		if (abs(x - gameport.x) > 8) {
			x = gameport.x;
			if (x < xmin) {
				xmin = x;
			}
			if (x > xmax) {
				xmax = x;
			}
			if (xmin == xmax) {
				xmin--;
				xmax++;
			}
			v = (Sint32)x;
			v -= (xmax + xmin + 1)/2;
			v *= 32768/((xmax - xmin + 1)/2);
			SDL_PrivateJoystickAxis(joy, 0, v);
		}
		if (abs(y - gameport.y) > 8) {
			y = gameport.y;
			if (y < ymin) {
				ymin = y;
			}
			if (y > ymax) {
				ymax = y;
			}
			if (ymin == ymax) {
				ymin--;
				ymax++;
			}
			v = (Sint32)y;
			v -= (ymax + ymin + 1)/2;
			v *= 32768/((ymax - ymin + 1)/2);
			SDL_PrivateJoystickAxis(joy, 1, v);
		}
		if (gameport.b1 != joy->buttons[0]) {
			SDL_PrivateJoystickButton(joy, 0, gameport.b1);
		}
		if (gameport.b2 != joy->buttons[1]) {
			SDL_PrivateJoystickButton(joy, 1, gameport.b2);
		}
		return;
	}
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) */
	
	rep = &joy->hwdata->inreport;

	if (read(joy->hwdata->fd, REP_BUF_DATA(rep), rep->size) != rep->size) {
		return;
	}
#if defined(USBHID_NEW) || (defined(__FreeBSD__) && __FreeBSD_version >= 500111)
	hdata = hid_start_parse(joy->hwdata->repdesc, 1 << hid_input, rep->rid);
#else
	hdata = hid_start_parse(joy->hwdata->repdesc, 1 << hid_input);
#endif
	if (hdata == NULL) {
		fprintf(stderr, "%s: Cannot start HID parser\n",
		    joy->hwdata->path);
		return;
	}

	for (nbutton = 0; hid_get_item(hdata, &hitem) > 0;) {
		switch (hitem.kind) {
		case hid_input:
			switch (HID_PAGE(hitem.usage)) {
			case HUP_GENERIC_DESKTOP: {
			    unsigned usage = HID_USAGE(hitem.usage);
			    int joyaxe = usage_to_joyaxe(usage);
			    if (joyaxe >= 0) {
				naxe = joy->hwdata->axis_map[joyaxe];
				/* scaleaxe */
				v = (Sint32)hid_get_data(REP_BUF_DATA(rep),
							 &hitem);
				v -= (hitem.logical_maximum + hitem.logical_minimum + 1)/2;
				v *= 32768/((hitem.logical_maximum - hitem.logical_minimum + 1)/2);
				if (v != joy->axes[naxe]) {
				    SDL_PrivateJoystickAxis(joy, naxe, v);
				}
			    } else if (usage == HUG_HAT_SWITCH) {
				v = (Sint32)hid_get_data(REP_BUF_DATA(rep),
							 &hitem);
				SDL_PrivateJoystickHat(joy, 0, hatval_to_sdl(v));
			    }
			    break;
			}
			case HUP_BUTTON:
				v = (Sint32)hid_get_data(REP_BUF_DATA(rep),
				    &hitem);
				if (joy->buttons[nbutton] != v) {
					SDL_PrivateJoystickButton(joy,
					    nbutton, v);
				}
				nbutton++;
				break;
			default:
				continue;
			}
			break;
		default:
			break;
		}
	}
	hid_end_parse(hdata);

	return;
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick *joy)
{
	if (strncmp(joy->hwdata->path, "/dev/joy", 8))	{
		report_free(&joy->hwdata->inreport);
		hid_dispose_report_desc(joy->hwdata->repdesc);
	}
	close(joy->hwdata->fd);
	free(joy->hwdata->path);
	free(joy->hwdata);

	return;
}

void
SDL_SYS_JoystickQuit(void)
{
	int i;

	for (i = 0; i < MAX_JOYS; i++) {
		if (joynames[i] != NULL)
			free(joynames[i]);
		if (joydevnames[i] != NULL)
			free(joydevnames[i]);
	}

	return;
}

static int
report_alloc(struct report *r, struct report_desc *rd, int repind)
{
	int len;

#ifdef __FreeBSD__
# if (__FreeBSD_version >= 460000)
#  if (__FreeBSD_version <= 500111)
	len = hid_report_size(rd, r->rid, repinfo[repind].kind);
#  else
	len = hid_report_size(rd, repinfo[repind].kind, r->rid);
#  endif
# else
	len = hid_report_size(rd, repinfo[repind].kind, &r->rid);
#endif
#else
# ifdef USBHID_NEW
	len = hid_report_size(rd, repinfo[repind].kind, &r->rid);
# else
	len = hid_report_size(rd, repinfo[repind].kind, r->rid);
# endif
#endif

	if (len < 0) {
		SDL_SetError("Negative HID report size");
		return (-1);
	}
	r->size = len;

	if (r->size > 0) {
		r->buf = malloc(sizeof(*r->buf) - sizeof(REP_BUF_DATA(r)) +
		    r->size);
		if (r->buf == NULL) {
			SDL_OutOfMemory();
			return (-1);
		}
	} else {
		r->buf = NULL;
	}

	r->status = SREPORT_CLEAN;
	return (0);
}

static void
report_free(struct report *r)
{
	if (r->buf != NULL) {
		free(r->buf);
	}
	r->status = SREPORT_UNINIT;
}

