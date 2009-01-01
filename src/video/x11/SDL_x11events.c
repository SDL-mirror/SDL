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

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "SDL_syswm.h"
#include "SDL_x11video.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_mouse_c.h"

static void
X11_DispatchEvent(_THIS)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    SDL_WindowData *data;
    XEvent xevent;
    int i;

    SDL_zero(xevent);           /* valgrind fix. --ryan. */
    XNextEvent(videodata->display, &xevent);

    /* filter events catchs XIM events and sends them to the correct
       handler */
    if (XFilterEvent(&xevent, None) == True) {
#if 0
        printf("Filtered event type = %d display = %d window = %d\n",
               xevent.type, xevent.xany.display, xevent.xany.window);
#endif
        return;
    }

    /* Send a SDL_SYSWMEVENT if the application wants them */
    if (SDL_ProcessEvents[SDL_SYSWMEVENT] == SDL_ENABLE) {
        SDL_SysWMmsg wmmsg;

        SDL_VERSION(&wmmsg.version);
        wmmsg.subsystem = SDL_SYSWM_X11;
        wmmsg.event.xevent = xevent;
        SDL_SendSysWMEvent(&wmmsg);
    }

    data = NULL;
    if (videodata && videodata->windowlist) {
        for (i = 0; i < videodata->numwindows; ++i) {
            if ((videodata->windowlist[i] != NULL) &&
                (videodata->windowlist[i]->window == xevent.xany.window)) {
                data = videodata->windowlist[i];
                break;
            }
        }
    }
    if (!data) {
        return;
    }
#if 0
    printf("type = %d display = %d window = %d\n",
           xevent.type, xevent.xany.display, xevent.xany.window);
#endif
    switch (xevent.type) {

        /* Gaining mouse coverage? */
    case EnterNotify:{
#ifdef DEBUG_XEVENTS
            printf("EnterNotify! (%d,%d)\n", xevent.xcrossing.x,
                   xevent.xcrossing.y);
            if (xevent.xcrossing.mode == NotifyGrab)
                printf("Mode: NotifyGrab\n");
            if (xevent.xcrossing.mode == NotifyUngrab)
                printf("Mode: NotifyUngrab\n");
#endif
            if ((xevent.xcrossing.mode != NotifyGrab) &&
                (xevent.xcrossing.mode != NotifyUngrab)) {
                /* FIXME: Should we reset data for all mice? */
#if 0
                SDL_SetMouseFocus(0, data->windowID);
                SDL_SendMouseMotion(0, 0, move->x, move->y, 0);
#endif
            }
        }
        break;

        /* Losing mouse coverage? */
    case LeaveNotify:{
#ifdef DEBUG_XEVENTS
            printf("LeaveNotify! (%d,%d)\n", xevent.xcrossing.x,
                   xevent.xcrossing.y);
            if (xevent.xcrossing.mode == NotifyGrab)
                printf("Mode: NotifyGrab\n");
            if (xevent.xcrossing.mode == NotifyUngrab)
                printf("Mode: NotifyUngrab\n");
#endif
            if ((xevent.xcrossing.mode != NotifyGrab) &&
                (xevent.xcrossing.mode != NotifyUngrab) &&
                (xevent.xcrossing.detail != NotifyInferior)) {
                /* FIXME: Should we reset data for all mice? */
#if 0
                SDL_SetMouseFocus(0, 0);
#endif
            }
        }
        break;

        /* Gaining input focus? */
    case FocusIn:{
#ifdef DEBUG_XEVENTS
            printf("FocusIn!\n");
#endif
            SDL_SetKeyboardFocus(videodata->keyboard, data->windowID);
#ifdef X_HAVE_UTF8_STRING
            if (data->ic) {
                XSetICFocus(data->ic);
            }
#endif
        }
        break;

        /* Losing input focus? */
    case FocusOut:{
#ifdef DEBUG_XEVENTS
            printf("FocusOut!\n");
#endif
            SDL_SetKeyboardFocus(videodata->keyboard, 0);
#ifdef X_HAVE_UTF8_STRING
            if (data->ic) {
                XUnsetICFocus(data->ic);
            }
#endif
        }
        break;

        /* Generated upon EnterWindow and FocusIn */
    case KeymapNotify:{
#ifdef DEBUG_XEVENTS
            printf("KeymapNotify!\n");
#endif
            /* FIXME:
               X11_SetKeyboardState(SDL_Display, xevent.xkeymap.key_vector);
             */
        }
        break;

        /* Has the keyboard layout changed? */
    case MappingNotify:{
#ifdef DEBUG_XEVENTS
            printf("MappingNotify!\n");
#endif
            X11_UpdateKeymap(_this);
        }
        break;

        /* Key press? */
    case KeyPress:{
            KeyCode keycode = xevent.xkey.keycode;
            KeySym keysym = NoSymbol;
            char text[SDL_TEXTINPUTEVENT_TEXT_SIZE];
            Status status = 0;

#ifdef DEBUG_XEVENTS
            printf("KeyPress (X11 keycode = 0x%X)\n", xevent.xkey.keycode);
#endif
            SDL_SendKeyboardKey(videodata->keyboard, SDL_PRESSED,
                                videodata->key_layout[keycode]);
#if 0
            if (videodata->key_layout[keycode] == SDLK_UNKNOWN) {
                int min_keycode, max_keycode;
                XDisplayKeycodes(videodata->display, &min_keycode,
                                 &max_keycode);
                keysym = XKeycodeToKeysym(videodata->display, keycode, 0);
                fprintf(stderr,
                        "The key you just pressed is not recognized by SDL. To help get this fixed, please report this to the SDL mailing list <sdl@libsdl.org> X11 KeyCode %d (%d), X11 KeySym 0x%X (%s).\n",
                        keycode, keycode - min_keycode, keysym,
                        XKeysymToString(keysym));
            }
#endif
            /* */
            SDL_zero(text);
#ifdef X_HAVE_UTF8_STRING
            if (data->ic) {
                Xutf8LookupString(data->ic, &xevent.xkey, text, sizeof(text),
                                  &keysym, &status);
            }
#else
            XLookupString(&xevent.xkey, text, sizeof(text), &keysym, NULL);
#endif
            if (*text) {
                printf("Sending text event %s\n", text);
                SDL_SendKeyboardText(videodata->keyboard, text);
            }
        }
        break;

        /* Key release? */
    case KeyRelease:{
            KeyCode keycode = xevent.xkey.keycode;

#ifdef DEBUG_XEVENTS
            printf("KeyRelease (X11 keycode = 0x%X)\n", xevent.xkey.keycode);
#endif
            SDL_SendKeyboardKey(videodata->keyboard, SDL_RELEASED,
                                videodata->key_layout[keycode]);
        }
        break;

        /* Have we been iconified? */
    case UnmapNotify:{
#ifdef DEBUG_XEVENTS
            printf("UnmapNotify!\n");
#endif
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_HIDDEN, 0, 0);
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_MINIMIZED, 0,
                                0);
        }
        break;

        /* Have we been restored? */
    case MapNotify:{
#ifdef DEBUG_XEVENTS
            printf("MapNotify!\n");
#endif
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_SHOWN, 0, 0);
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_RESTORED, 0,
                                0);
        }
        break;

        /* Have we been resized or moved? */
    case ConfigureNotify:{
#ifdef DEBUG_XEVENTS
            printf("ConfigureNotify! (resize: %dx%d)\n",
                   xevent.xconfigure.width, xevent.xconfigure.height);
#endif
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_MOVED,
                                xevent.xconfigure.x, xevent.xconfigure.y);
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_RESIZED,
                                xevent.xconfigure.width,
                                xevent.xconfigure.height);
        }
        break;

        /* Have we been requested to quit (or another client message?) */
    case ClientMessage:{
            if ((xevent.xclient.format == 32) &&
                (xevent.xclient.data.l[0] == videodata->WM_DELETE_WINDOW)) {

                SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_CLOSE, 0,
                                    0);
            }
        }
        break;

        /* Do we need to refresh ourselves? */
    case Expose:{
#ifdef DEBUG_XEVENTS
            printf("Expose (count = %d)\n", xevent.xexpose.count);
#endif
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_EXPOSED, 0,
                                0);
        }
        break;

    case MotionNotify:
#ifdef DEBUG_MOTION
        printf("X11 motion: %d,%d\n", xevent.xmotion.x, xevent.xmotion.y);
#endif
        SDL_SendMouseMotion(0, 0, xevent.xmotion.x, xevent.xmotion.y, 0);
        break;

    case ButtonPress:
        SDL_SendMouseButton(0, SDL_PRESSED, xevent.xbutton.button);
        break;

    case ButtonRelease:
        SDL_SendMouseButton(0, SDL_RELEASED, xevent.xbutton.button);
        break;

    default:{
#if SDL_VIDEO_DRIVER_X11_XINPUT
        for (i = 0; i < SDL_GetNumMice(); ++i) {
            SDL_Mouse *mouse;
            X11_MouseData *data;

            mouse = SDL_GetMouse(i);
            data = (X11_MouseData *)mouse->driverdata;
            if (!data) {
                continue;
            }
                    
            if (xevent.type == data->motion) {          /* MotionNotify */
                XDeviceMotionEvent *move = (XDeviceMotionEvent *) & xevent;
#ifdef DEBUG_MOTION
                printf("X11 motion: %d,%d\n", move->x, move->y);
#endif
                SDL_SendMouseMotion(move->deviceid, 0, move->x, move->y, move->axis_data[2]);
                return;
            }
            if (xevent.type == data->button_pressed) {  /* ButtonPress */
                XDeviceButtonPressedEvent *pressed =
                    (XDeviceButtonPressedEvent *) & xevent;
                SDL_SendMouseButton(pressed->deviceid, SDL_PRESSED, pressed->button);
                return;
            }
            if (xevent.type == data->button_released) { /* ButtonRelease */
                XDeviceButtonReleasedEvent *released =
                    (XDeviceButtonReleasedEvent *) & xevent;
                SDL_SendMouseButton(released->deviceid, SDL_RELEASED, released->button);
                return;
            }
            if (xevent.type == data->proximity_in) {
                XProximityNotifyEvent *proximity =
                    (XProximityNotifyEvent *) & xevent;
                SDL_SendProximity(proximity->deviceid, proximity->x, proximity->y, SDL_PROXIMITYIN);
                return;
            }
            if (xevent.type == data->proximity_out) {
                XProximityNotifyEvent *proximity =
                    (XProximityNotifyEvent *) & xevent;
                SDL_SendProximity(proximity->deviceid, proximity->x, proximity->y, SDL_PROXIMITYOUT);
                return;
            }
        }
#endif
#ifdef DEBUG_XEVENTS
            printf("Unhandled event %d\n", xevent.type);
#endif
        }
        break;
    }
}

/* Ack!  XPending() actually performs a blocking read if no events available */
int
X11_Pending(Display * display)
{
    /* Flush the display connection and look to see if events are queued */
    XFlush(display);
    if (XEventsQueued(display, QueuedAlready)) {
        return (1);
    }

    /* More drastic measures are required -- see if X is ready to talk */
    {
        static struct timeval zero_time;        /* static == 0 */
        int x11_fd;
        fd_set fdset;

        x11_fd = ConnectionNumber(display);
        FD_ZERO(&fdset);
        FD_SET(x11_fd, &fdset);
        if (select(x11_fd + 1, &fdset, NULL, NULL, &zero_time) == 1) {
            return (XPending(display));
        }
    }

    /* Oh well, nothing is ready .. */
    return (0);
}

void
X11_PumpEvents(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    /* Keep processing pending events */
    while (X11_Pending(data->display)) {
        X11_DispatchEvent(_this);
    }
}

void
X11_SaveScreenSaver(Display * display, int *saved_timeout, BOOL * dpms)
{
    int timeout, interval, prefer_blank, allow_exp;
    XGetScreenSaver(display, &timeout, &interval, &prefer_blank, &allow_exp);
    *saved_timeout = timeout;

#if SDL_VIDEO_DRIVER_X11_DPMS
    if (SDL_X11_HAVE_DPMS) {
        int dummy;
        if (DPMSQueryExtension(display, &dummy, &dummy)) {
            CARD16 state;
            DPMSInfo(display, &state, dpms);
        }
    }
#else
    *dpms = 0;
#endif /* SDL_VIDEO_DRIVER_X11_DPMS */
}

void
X11_DisableScreenSaver(Display * display)
{
    int timeout, interval, prefer_blank, allow_exp;
    XGetScreenSaver(display, &timeout, &interval, &prefer_blank, &allow_exp);
    timeout = 0;
    XSetScreenSaver(display, timeout, interval, prefer_blank, allow_exp);

#if SDL_VIDEO_DRIVER_X11_DPMS
    if (SDL_X11_HAVE_DPMS) {
        int dummy;
        if (DPMSQueryExtension(display, &dummy, &dummy)) {
            DPMSDisable(display);
        }
    }
#endif /* SDL_VIDEO_DRIVER_X11_DPMS */
}

void
X11_RestoreScreenSaver(Display * display, int saved_timeout, BOOL dpms)
{
    int timeout, interval, prefer_blank, allow_exp;
    XGetScreenSaver(display, &timeout, &interval, &prefer_blank, &allow_exp);
    timeout = saved_timeout;
    XSetScreenSaver(display, timeout, interval, prefer_blank, allow_exp);

#if SDL_VIDEO_DRIVER_X11_DPMS
    if (SDL_X11_HAVE_DPMS) {
        int dummy;
        if (DPMSQueryExtension(display, &dummy, &dummy)) {
            if (dpms) {
                DPMSEnable(display);
            }
        }
    }
#endif /* SDL_VIDEO_DRIVER_X11_DPMS */
}

/* vi: set ts=4 sw=4 expandtab: */
