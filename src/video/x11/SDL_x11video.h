/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_x11video_h
#define _SDL_x11video_h

#include "../SDL_sysvideo.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#if SDL_VIDEO_DRIVER_X11_XINERAMA
#include "../Xext/extensions/Xinerama.h"
#endif
#if SDL_VIDEO_DRIVER_X11_XRANDR
#include <X11/extensions/Xrandr.h>
#endif
#if SDL_VIDEO_DRIVER_X11_VIDMODE
#include "../Xext/extensions/xf86vmode.h"
#endif
#if SDL_VIDEO_DRIVER_X11_XME
#include "../Xext/extensions/xme.h"
#endif
#if SDL_VIDEO_DRIVER_X11_DPMS
#include <X11/extensions/dpms.h>
#endif
#if SDL_VIDEO_DRIVER_X11_XINPUT
#include <X11/extensions/XInput.h>
#endif

#include "SDL_x11dyn.h"

#include "SDL_x11events.h"
#include "SDL_x11gamma.h"
#include "SDL_x11keyboard.h"
#include "SDL_x11modes.h"
#include "SDL_x11mouse.h"
#include "SDL_x11opengl.h"
#include "SDL_x11window.h"

/* Private display data */

#if SDL_VIDEO_DRIVER_X11_XINPUT
/* !!! FIXME: should be in SDL_VideoData, not globals. */
extern XDevice **SDL_XDevices;
extern int SDL_NumOfXDevices;
extern XEventClass SDL_XEvents[256];
extern int SDL_NumOfXEvents;
#endif

/* !!! FIXME: should be in SDL_VideoData, not globals. */
/* !!! FIXME: change these names, too. */
extern int motion;              /* the motion event id defined by an XInput function */
extern int button_pressed;      /* the button_pressed event id defined by an XInput function */
extern int button_released;     /* the button_released event id defined by an XInput function */
extern int proximity_in;        /* the proximity in event defined by an XInput function */
extern int proximity_out;       /* the proximity out event defined by an XInput function */

typedef struct SDL_VideoData
{
    Display *display;
    char *classname;
    XIM im;
    int screensaver_timeout;
    BOOL dpms_enabled;
    int numwindows;
    SDL_WindowData **windowlist;
    int windowlistlength;
    int mouse;
    int keyboard;
    Atom WM_DELETE_WINDOW;
    SDL_scancode key_layout[256];
} SDL_VideoData;

#endif /* _SDL_x11video_h */

/* vi: set ts=4 sw=4 expandtab: */
