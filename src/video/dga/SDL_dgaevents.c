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

/* Handle the event stream, converting DGA events into SDL events */

#include <stdio.h>
#include <X11/Xlib.h>
#include <XFree86/extensions/xf86dga.h>

#include "SDL_sysvideo.h"
#include "SDL_events_c.h"
#include "SDL_dgavideo.h"
#include "SDL_dgaevents_c.h"

/* Heheh we're using X11 event code */
extern int X11_Pending(Display *display);
extern void X11_InitKeymap(void);
extern SDL_keysym *X11_TranslateKey(Display *display, XKeyEvent *xkey,
				    KeyCode kc, SDL_keysym *keysym);

static int DGA_DispatchEvent(_THIS)
{
	int posted;
	SDL_NAME(XDGAEvent) xevent;

	XNextEvent(DGA_Display, (XEvent *)&xevent);

	posted = 0;
	xevent.type -= DGA_event_base;
	switch (xevent.type) {

	    /* Mouse motion? */
	    case MotionNotify: {
		if ( SDL_VideoSurface ) {
			posted = SDL_PrivateMouseMotion(0, 1,
					xevent.xmotion.dx, xevent.xmotion.dy);
		}
	    }
	    break;

	    /* Mouse button press? */
	    case ButtonPress: {
		posted = SDL_PrivateMouseButton(SDL_PRESSED, 
					xevent.xbutton.button, 0, 0);
	    }
	    break;

	    /* Mouse button release? */
	    case ButtonRelease: {
		posted = SDL_PrivateMouseButton(SDL_RELEASED, 
					xevent.xbutton.button, 0, 0);
	    }
	    break;

	    /* Key press or release? */
	    case KeyPress:
	    case KeyRelease: {
		SDL_keysym keysym;
		XKeyEvent xkey;

		SDL_NAME(XDGAKeyEventToXKeyEvent)(&xevent.xkey, &xkey);
		posted = SDL_PrivateKeyboard((xevent.type == KeyPress), 
					X11_TranslateKey(DGA_Display,
							 &xkey, xkey.keycode,
							 &keysym));
	    }
	    break;

	}
	return(posted);
}

void DGA_PumpEvents(_THIS)
{
	/* Keep processing pending events */
	LOCK_DISPLAY();
	while ( X11_Pending(DGA_Display) ) {
		DGA_DispatchEvent(this);
	}
	UNLOCK_DISPLAY();
}

void DGA_InitOSKeymap(_THIS)
{
	X11_InitKeymap();
}

