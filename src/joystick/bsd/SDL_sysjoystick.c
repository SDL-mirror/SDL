/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

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
 "@(#) $Id $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>
#include <usbhid.h>

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
	enum	hid_kind kind;
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
	JOYAXE_WHEEL
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
#if 0
	int	axismin[];
	int	axismax[];
#endif
};

static char *joynames[MAX_JOYS];
static char *joydevnames[MAX_JOYS];

static int	report_alloc(struct report *, struct report_desc *, int);
static void	report_free(struct report *);

int
SDL_SYS_JoystickInit(void)
{
	char s[10];
	int i, fd;

	SDL_numjoysticks = 0;

	memset(joynames, NULL, sizeof(joynames));
	memset(joydevnames, NULL, sizeof(joydevnames));

	for (i = 0; i < MAX_UHID_JOYS; i++) {
		sprintf(s, "/dev/uhid%d", i);
		fd = open(s, O_RDWR);
		if (fd > 0) {
			joynames[SDL_numjoysticks++] = strdup(s);
			close(fd);
		}
	}
	for (i = 0; i < MAX_JOY_JOYS; i++) {
		sprintf(s, "/dev/joy%d", i);
		fd = open(s, O_RDWR);
		if (fd > 0) {
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

int
SDL_SYS_JoystickOpen(SDL_Joystick *joy)
{
	char *path = joynames[joy->index];
	struct joystick_hwdata *hw;
	struct hid_item hitem;
	struct hid_data *hdata;
	struct report *rep;
	int fd;

	fd = open(path, O_RDWR);
	if (fd < 0) {
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
	hw->type = BSDJOY_UHID;
	hw->repdesc = hid_get_report_desc(fd);
	if (hw->repdesc == NULL) {
		SDL_SetError("%s: USB_GET_REPORT_DESC: %s", hw->path,
		    strerror(errno));
		goto usberr;
	}

	rep = &hw->inreport;
	if (report_alloc(rep, hw->repdesc, REPORT_INPUT) < 0) {
		goto usberr;
	}
	if (rep->size <= 0) {
		SDL_SetError("%s: Input report descriptor has invalid length",
		    hw->path);
		goto usberr;
	}

	hdata = hid_start_parse(hw->repdesc, 1 << hid_input);
	if (hdata == NULL) {
		SDL_SetError("%s: Cannot start HID parser", hw->path);
		goto usberr;
	}
	joy->naxes = 0;
	joy->nbuttons = 0;
	joy->nhats = 0;
	joy->nballs = 0;

	while (hid_get_item(hdata, &hitem) > 0) {
		char *s, *sp;

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
			case HUP_UNDEFINED:
				break;
			case HUP_GENERIC_DESKTOP:
				switch (HID_USAGE(hitem.usage)) {
				case HUG_X:
				case HUG_Y:
				case HUG_Z:
				case HUG_SLIDER:
				case HUG_WHEEL:
#if 0
					hw->axismin[joy->naxes] =
					    hitem.logical_minimum;
					hw->axismax[joy->naxes] =
					    hitem.logical_maximum;
#endif
					joy->naxes++;
					break;
				}
				break;
			case HUP_BUTTON:
				joy->nbuttons++;
				break;
			}
			break;
		default:
			break;
		}
	}
	hid_end_parse(hdata);

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
	static struct hid_item hitem;
	static struct hid_data *hdata;
	static struct report *rep;
	int nbutton, naxe;
	Sint32 v;
	
	rep = &joy->hwdata->inreport;
	if (read(joy->hwdata->fd, rep->buf->data, rep->size) != rep->size) {
		return;
	}
	hdata = hid_start_parse(joy->hwdata->repdesc, 1 << hid_input);
	if (hdata == NULL) {
		fprintf(stderr, "%s: Cannot start HID parser\n",
		    joy->hwdata->path);
		return;
	}

	for (nbutton = 0; hid_get_item(hdata, &hitem) > 0;) {
		switch (hitem.kind) {
		case hid_input:
			switch (HID_PAGE(hitem.usage)) {
			case HUP_UNDEFINED:
				continue;
			case HUP_GENERIC_DESKTOP:
				switch (HID_USAGE(hitem.usage)) {
				case HUG_X:
					naxe = JOYAXE_X;
					goto scaleaxe;
				case HUG_Y:
					naxe = JOYAXE_Y;
					goto scaleaxe;
				case HUG_Z:
					naxe = JOYAXE_Z;
					goto scaleaxe;
				case HUG_SLIDER:
					naxe = JOYAXE_SLIDER;
					goto scaleaxe;
				case HUG_WHEEL:
					naxe = JOYAXE_WHEEL;
					goto scaleaxe;
				}
scaleaxe:
				v = (Sint32)hid_get_data(rep->buf->data, &hitem);
				if (v != 127) {
					if (v < 127) {
						v = -(256 - v);
						v <<= 7;
						v++;
					} else {
						v++;
						v <<= 7;
						v--;
					}
				} else {
					v = 0;
				}
				if (v != joy->axes[naxe]) {
					SDL_PrivateJoystickAxis(joy, naxe, v);
				}
				break;
			case HUP_BUTTON:
				v = (Sint32)hid_get_data(rep->buf->data,
				    &hitem);
				if (joy->buttons[nbutton] != v) {
					SDL_PrivateJoystickButton(joy,
					    nbutton, v);
				}
				nbutton++;
				break;
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
	report_free(&joy->hwdata->inreport);
	hid_dispose_report_desc(joy->hwdata->repdesc);
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

	len = hid_report_size(rd, repinfo[repind].kind, &r->rid);
	if (len < 0) {
		SDL_SetError("Negative HID report size");
		return (-1);
	}
	r->size = len;

	if (r->size > 0) {
		r->buf = malloc(sizeof(*r->buf) - sizeof(r->buf->data) +
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

