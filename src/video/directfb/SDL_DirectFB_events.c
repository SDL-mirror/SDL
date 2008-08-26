/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

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

/* Handle the event stream, converting DirectFB input events into SDL events */

#include <directfb.h>

#include "../SDL_sysvideo.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_linux.h"
#include "SDL_DirectFB_events.h"

/* The translation tables from a DirectFB keycode to a SDL keysym */
static SDLKey keymap[256];

static SDL_keysym *DirectFB_TranslateKey(_THIS, DFBWindowEvent * evt,
                                         SDL_keysym * keysym);

static void DirectFB_InitOSKeymap(_THIS);
static int DirectFB_TranslateButton(DFBInputDeviceButtonIdentifier button);

static void
DirectFB_SetContext(_THIS, SDL_WindowID id)
{
#if (DIRECTFB_MAJOR_VERSION >= 1)
    /* FIXME: does not work on 1.0/1.2 with radeon driver
     *        the approach did work with the matrox driver
     *        Perhaps make this depending on env var, e.g. SDLDIRECTFB_SWITCHCONTEXT_SUPPORTED
     */

    if (getenv("SDLDIRECTFB_SWITCHCONTEXT_SUPPORTED") != NULL) {
        SDL_DFB_DEVICEDATA(_this);
        SDL_Window *window = SDL_GetWindowFromID(id);
        SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
        DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;
        if (dispdata->vidID >= 0 && dispdata->vidIDinuse) {
            IDirectFBDisplayLayer *lay = NULL;
            devdata->dfb->GetDisplayLayer(devdata->dfb, dispdata->vidID,
                                          &lay);
            if (lay)
                lay->SwitchContext(lay, DFB_TRUE);
        }
    }
#endif

}

void
DirectFB_PumpEventsWindow(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);
    DFB_WindowData *p;
    DFBWindowEvent evt;
    char text[5];

    for (p = devdata->firstwin; p != NULL; p = p->next) {
        while (p->eventbuffer->GetEvent(p->eventbuffer,
                                        DFB_EVENT(&evt)) == DFB_OK) {
            SDL_keysym keysym;

            if (evt.clazz = DFEC_WINDOW) {
                switch (evt.type) {
                case DWET_BUTTONDOWN:
                    SDL_SendMouseButton(devdata->mouse, SDL_PRESSED,
                                        DirectFB_TranslateButton(evt.button));
                    break;
                case DWET_BUTTONUP:
                    SDL_SendMouseButton(devdata->mouse, SDL_RELEASED,
                                        DirectFB_TranslateButton(evt.button));
                    break;
                case DWET_MOTION:
                    SDL_SendMouseMotion(devdata->mouse, 0, evt.cx, evt.cy);
                    break;
                case DWET_KEYDOWN:
                    DirectFB_TranslateKey(_this, &evt, &keysym);
                    SDL_SendKeyboardKey(devdata->keyboard, SDL_PRESSED,
                                        keysym.scancode);
                    if (SDL_EventState(SDL_TEXTINPUT, SDL_QUERY)) {
                        SDL_memcpy(text, &keysym.unicode, 4);
                        text[4] = 0;
                        if (*text) {
                            SDL_SendKeyboardText(devdata->keyboard, text);
                        }
                    }
                    break;
                case DWET_KEYUP:
                    DirectFB_TranslateKey(_this, &evt, &keysym);
                    SDL_SendKeyboardKey(devdata->keyboard, SDL_RELEASED,
                                        keysym.scancode);
                    break;
                case DWET_POSITION_SIZE:
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_MOVED, evt.x,
                                        evt.y);
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_RESIZED, evt.w,
                                        evt.h);
                    break;
                case DWET_POSITION:
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_MOVED, evt.x,
                                        evt.y);
                    break;
                case DWET_SIZE:
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_RESIZED, evt.w,
                                        evt.h);
                    break;
                case DWET_CLOSE:
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_CLOSE, 0, 0);
                    break;
                case DWET_GOTFOCUS:
                    DirectFB_SetContext(_this, p->id);
                    SDL_SetKeyboardFocus(devdata->keyboard, p->id);
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_FOCUS_GAINED,
                                        0, 0);
                    break;
                case DWET_LOSTFOCUS:
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_FOCUS_LOST, 0,
                                        0);
                    SDL_SetKeyboardFocus(devdata->keyboard, 0);
                    break;
                case DWET_ENTER:
                    //SDL_DirectFB_ReshowCursor(_this, 0);
                    SDL_SetMouseFocus(devdata->mouse, p->id);
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_ENTER, 0, 0);
                    break;
                case DWET_LEAVE:
                    SDL_SendWindowEvent(p->id, SDL_WINDOWEVENT_LEAVE, 0, 0);
                    SDL_SetMouseFocus(devdata->mouse, 0);
                    //SDL_DirectFB_ReshowCursor(_this, 1);
                    break;
                default:
                    ;
                }
            }
        }
    }
}

void
DirectFB_InitOSKeymap(_THIS)
{
    int i;

    /* Initialize the DirectFB key translation table */
    for (i = 0; i < SDL_arraysize(keymap); ++i)
        keymap[i] = SDL_SCANCODE_UNKNOWN;

    keymap[DIKI_A - DIKI_UNKNOWN] = SDL_SCANCODE_A;
    keymap[DIKI_B - DIKI_UNKNOWN] = SDL_SCANCODE_B;
    keymap[DIKI_C - DIKI_UNKNOWN] = SDL_SCANCODE_C;
    keymap[DIKI_D - DIKI_UNKNOWN] = SDL_SCANCODE_D;
    keymap[DIKI_E - DIKI_UNKNOWN] = SDL_SCANCODE_E;
    keymap[DIKI_F - DIKI_UNKNOWN] = SDL_SCANCODE_F;
    keymap[DIKI_G - DIKI_UNKNOWN] = SDL_SCANCODE_G;
    keymap[DIKI_H - DIKI_UNKNOWN] = SDL_SCANCODE_H;
    keymap[DIKI_I - DIKI_UNKNOWN] = SDL_SCANCODE_I;
    keymap[DIKI_J - DIKI_UNKNOWN] = SDL_SCANCODE_J;
    keymap[DIKI_K - DIKI_UNKNOWN] = SDL_SCANCODE_K;
    keymap[DIKI_L - DIKI_UNKNOWN] = SDL_SCANCODE_L;
    keymap[DIKI_M - DIKI_UNKNOWN] = SDL_SCANCODE_M;
    keymap[DIKI_N - DIKI_UNKNOWN] = SDL_SCANCODE_N;
    keymap[DIKI_O - DIKI_UNKNOWN] = SDL_SCANCODE_O;
    keymap[DIKI_P - DIKI_UNKNOWN] = SDL_SCANCODE_P;
    keymap[DIKI_Q - DIKI_UNKNOWN] = SDL_SCANCODE_Q;
    keymap[DIKI_R - DIKI_UNKNOWN] = SDL_SCANCODE_R;
    keymap[DIKI_S - DIKI_UNKNOWN] = SDL_SCANCODE_S;
    keymap[DIKI_T - DIKI_UNKNOWN] = SDL_SCANCODE_T;
    keymap[DIKI_U - DIKI_UNKNOWN] = SDL_SCANCODE_U;
    keymap[DIKI_V - DIKI_UNKNOWN] = SDL_SCANCODE_V;
    keymap[DIKI_W - DIKI_UNKNOWN] = SDL_SCANCODE_W;
    keymap[DIKI_X - DIKI_UNKNOWN] = SDL_SCANCODE_X;
    keymap[DIKI_Y - DIKI_UNKNOWN] = SDL_SCANCODE_Y;
    keymap[DIKI_Z - DIKI_UNKNOWN] = SDL_SCANCODE_Z;

    keymap[DIKI_0 - DIKI_UNKNOWN] = SDL_SCANCODE_0;
    keymap[DIKI_1 - DIKI_UNKNOWN] = SDL_SCANCODE_1;
    keymap[DIKI_2 - DIKI_UNKNOWN] = SDL_SCANCODE_2;
    keymap[DIKI_3 - DIKI_UNKNOWN] = SDL_SCANCODE_3;
    keymap[DIKI_4 - DIKI_UNKNOWN] = SDL_SCANCODE_4;
    keymap[DIKI_5 - DIKI_UNKNOWN] = SDL_SCANCODE_5;
    keymap[DIKI_6 - DIKI_UNKNOWN] = SDL_SCANCODE_6;
    keymap[DIKI_7 - DIKI_UNKNOWN] = SDL_SCANCODE_7;
    keymap[DIKI_8 - DIKI_UNKNOWN] = SDL_SCANCODE_8;
    keymap[DIKI_9 - DIKI_UNKNOWN] = SDL_SCANCODE_9;

    keymap[DIKI_F1 - DIKI_UNKNOWN] = SDL_SCANCODE_F1;
    keymap[DIKI_F2 - DIKI_UNKNOWN] = SDL_SCANCODE_F2;
    keymap[DIKI_F3 - DIKI_UNKNOWN] = SDL_SCANCODE_F3;
    keymap[DIKI_F4 - DIKI_UNKNOWN] = SDL_SCANCODE_F4;
    keymap[DIKI_F5 - DIKI_UNKNOWN] = SDL_SCANCODE_F5;
    keymap[DIKI_F6 - DIKI_UNKNOWN] = SDL_SCANCODE_F6;
    keymap[DIKI_F7 - DIKI_UNKNOWN] = SDL_SCANCODE_F7;
    keymap[DIKI_F8 - DIKI_UNKNOWN] = SDL_SCANCODE_F8;
    keymap[DIKI_F9 - DIKI_UNKNOWN] = SDL_SCANCODE_F9;
    keymap[DIKI_F10 - DIKI_UNKNOWN] = SDL_SCANCODE_F10;
    keymap[DIKI_F11 - DIKI_UNKNOWN] = SDL_SCANCODE_F11;
    keymap[DIKI_F12 - DIKI_UNKNOWN] = SDL_SCANCODE_F12;

    keymap[DIKI_ESCAPE - DIKI_UNKNOWN] = SDL_SCANCODE_ESCAPE;
    keymap[DIKI_LEFT - DIKI_UNKNOWN] = SDL_SCANCODE_LEFT;
    keymap[DIKI_RIGHT - DIKI_UNKNOWN] = SDL_SCANCODE_RIGHT;
    keymap[DIKI_UP - DIKI_UNKNOWN] = SDL_SCANCODE_UP;
    keymap[DIKI_DOWN - DIKI_UNKNOWN] = SDL_SCANCODE_DOWN;
    keymap[DIKI_CONTROL_L - DIKI_UNKNOWN] = SDL_SCANCODE_LCTRL;
    keymap[DIKI_CONTROL_R - DIKI_UNKNOWN] = SDL_SCANCODE_RCTRL;
    keymap[DIKI_SHIFT_L - DIKI_UNKNOWN] = SDL_SCANCODE_LSHIFT;
    keymap[DIKI_SHIFT_R - DIKI_UNKNOWN] = SDL_SCANCODE_RSHIFT;
    keymap[DIKI_ALT_L - DIKI_UNKNOWN] = SDL_SCANCODE_LALT;
    keymap[DIKI_ALT_R - DIKI_UNKNOWN] = SDL_SCANCODE_RALT;
    keymap[DIKI_META_L - DIKI_UNKNOWN] = SDL_SCANCODE_LGUI;
    keymap[DIKI_META_R - DIKI_UNKNOWN] = SDL_SCANCODE_RGUI;
    keymap[DIKI_SUPER_L - DIKI_UNKNOWN] = SDL_SCANCODE_APPLICATION;
    keymap[DIKI_SUPER_R - DIKI_UNKNOWN] = SDL_SCANCODE_APPLICATION;
    //FIXME:Do we read hyper keys ?
    //keymap[DIKI_HYPER_L - DIKI_UNKNOWN] = SDL_SCANCODE_APPLICATION;
    //keymap[DIKI_HYPER_R - DIKI_UNKNOWN] = SDL_SCANCODE_APPLICATION;
    keymap[DIKI_TAB - DIKI_UNKNOWN] = SDL_SCANCODE_TAB;
    keymap[DIKI_ENTER - DIKI_UNKNOWN] = SDL_SCANCODE_RETURN;
    keymap[DIKI_SPACE - DIKI_UNKNOWN] = SDL_SCANCODE_SPACE;
    keymap[DIKI_BACKSPACE - DIKI_UNKNOWN] = SDL_SCANCODE_BACKSPACE;
    keymap[DIKI_INSERT - DIKI_UNKNOWN] = SDL_SCANCODE_INSERT;
    keymap[DIKI_DELETE - DIKI_UNKNOWN] = SDL_SCANCODE_DELETE;
    keymap[DIKI_HOME - DIKI_UNKNOWN] = SDL_SCANCODE_HOME;
    keymap[DIKI_END - DIKI_UNKNOWN] = SDL_SCANCODE_END;
    keymap[DIKI_PAGE_UP - DIKI_UNKNOWN] = SDL_SCANCODE_PAGEUP;
    keymap[DIKI_PAGE_DOWN - DIKI_UNKNOWN] = SDL_SCANCODE_PAGEDOWN;
    keymap[DIKI_CAPS_LOCK - DIKI_UNKNOWN] = SDL_SCANCODE_CAPSLOCK;
    keymap[DIKI_NUM_LOCK - DIKI_UNKNOWN] = SDL_SCANCODE_NUMLOCKCLEAR;
    keymap[DIKI_SCROLL_LOCK - DIKI_UNKNOWN] = SDL_SCANCODE_SCROLLLOCK;
    keymap[DIKI_PRINT - DIKI_UNKNOWN] = SDL_SCANCODE_PRINTSCREEN;
    keymap[DIKI_PAUSE - DIKI_UNKNOWN] = SDL_SCANCODE_PAUSE;

    keymap[DIKI_KP_EQUAL - DIKI_UNKNOWN] = SDL_SCANCODE_KP_EQUALS;
    keymap[DIKI_KP_DECIMAL - DIKI_UNKNOWN] = SDL_SCANCODE_KP_PERIOD;
    keymap[DIKI_KP_0 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_0;
    keymap[DIKI_KP_1 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_1;
    keymap[DIKI_KP_2 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_2;
    keymap[DIKI_KP_3 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_3;
    keymap[DIKI_KP_4 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_4;
    keymap[DIKI_KP_5 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_5;
    keymap[DIKI_KP_6 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_6;
    keymap[DIKI_KP_7 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_7;
    keymap[DIKI_KP_8 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_8;
    keymap[DIKI_KP_9 - DIKI_UNKNOWN] = SDL_SCANCODE_KP_9;
    keymap[DIKI_KP_DIV - DIKI_UNKNOWN] = SDL_SCANCODE_KP_DIVIDE;
    keymap[DIKI_KP_MULT - DIKI_UNKNOWN] = SDL_SCANCODE_KP_MULTIPLY;
    keymap[DIKI_KP_MINUS - DIKI_UNKNOWN] = SDL_SCANCODE_KP_MINUS;
    keymap[DIKI_KP_PLUS - DIKI_UNKNOWN] = SDL_SCANCODE_KP_PLUS;
    keymap[DIKI_KP_ENTER - DIKI_UNKNOWN] = SDL_SCANCODE_KP_ENTER;

    keymap[DIKI_QUOTE_LEFT - DIKI_UNKNOWN] = SDL_SCANCODE_GRAVE;        /*  TLDE  */
    keymap[DIKI_MINUS_SIGN - DIKI_UNKNOWN] = SDL_SCANCODE_MINUS;        /*  AE11  */
    keymap[DIKI_EQUALS_SIGN - DIKI_UNKNOWN] = SDL_SCANCODE_EQUALS;      /*  AE12  */
    keymap[DIKI_BRACKET_LEFT - DIKI_UNKNOWN] = SDL_SCANCODE_RIGHTBRACKET;       /*  AD11  */
    keymap[DIKI_BRACKET_RIGHT - DIKI_UNKNOWN] = SDL_SCANCODE_LEFTBRACKET;       /*  AD12  */
    keymap[DIKI_BACKSLASH - DIKI_UNKNOWN] = SDL_SCANCODE_BACKSLASH;     /*  BKSL  */
    keymap[DIKI_SEMICOLON - DIKI_UNKNOWN] = SDL_SCANCODE_SEMICOLON;     /*  AC10  */
    keymap[DIKI_QUOTE_RIGHT - DIKI_UNKNOWN] = SDL_SCANCODE_APOSTROPHE;  /*  AC11  */
    keymap[DIKI_COMMA - DIKI_UNKNOWN] = SDL_SCANCODE_COMMA;     /*  AB08  */
    keymap[DIKI_PERIOD - DIKI_UNKNOWN] = SDL_SCANCODE_PERIOD;   /*  AB09  */
    keymap[DIKI_SLASH - DIKI_UNKNOWN] = SDL_SCANCODE_SLASH;     /*  AB10  */
    keymap[DIKI_LESS_SIGN - DIKI_UNKNOWN] = SDL_SCANCODE_NONUSBACKSLASH;        /*  103rd  */

}

static SDL_keysym *
DirectFB_TranslateKey(_THIS, DFBWindowEvent * evt, SDL_keysym * keysym)
{
    SDL_DFB_DEVICEDATA(_this);

    if (evt->key_code >= 0
        && evt->key_code < SDL_arraysize(linux_scancode_table))
        keysym->scancode = linux_scancode_table[evt->key_code]; // key_id;
    else
        keysym->scancode = SDL_SCANCODE_UNKNOWN;

    if (keysym->scancode == SDL_SCANCODE_UNKNOWN || devdata->kbdgeneric) {
        if (evt->key_id - DIKI_UNKNOWN < SDL_arraysize(keymap))
            keysym->scancode = keymap[evt->key_id - DIKI_UNKNOWN];
        else
            keysym->scancode = SDL_SCANCODE_UNKNOWN;
    }

    keysym->unicode =
        (DFB_KEY_TYPE(evt->key_symbol) == DIKT_UNICODE) ? evt->key_symbol : 0;
    if (keysym->unicode == 0
        && (evt->key_symbol > 0 && evt->key_symbol < 255))
        keysym->unicode = evt->key_symbol;

    return keysym;
}

static int
DirectFB_TranslateButton(DFBInputDeviceButtonIdentifier button)
{
    switch (button) {
    case DIBI_LEFT:
        return 1;
    case DIBI_MIDDLE:
        return 2;
    case DIBI_RIGHT:
        return 3;
    default:
        return 0;
    }
}

static DFBEnumerationResult
input_device_cb(DFBInputDeviceID device_id, DFBInputDeviceDescription desc,
                void *callbackdata)
{
    DFB_DeviceData *devdata = callbackdata;
    SDL_Keyboard keyboard;
    SDL_scancode scancode;
    SDLKey keymap[SDL_NUM_SCANCODES];

    if ((desc.caps & DIDTF_KEYBOARD) && device_id == DIDID_KEYBOARD) {
        SDL_zero(keyboard);
        devdata->keyboard = SDL_AddKeyboard(&keyboard, -1);
        if (!strncmp("X11", desc.name, 3))
            devdata->kbdgeneric = 1;

        SDL_GetDefaultKeymap(keymap);
        SDL_SetKeymap(devdata->keyboard, 0, keymap, SDL_NUM_SCANCODES);
    }
    return DFB_OK;
}

void
DirectFB_InitKeyboard(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);
    int ret;

    DirectFB_InitOSKeymap(_this);

    devdata->kbdgeneric = 0;

    SDL_DFB_CHECK(devdata->dfb->
                  EnumInputDevices(devdata->dfb, input_device_cb, devdata));
}

#if 0
/* FIXME: Remove once determined this is not needed in fullscreen mode */
void
DirectFB_PumpEvents(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);
    DFBInputEvent evt;
    static last_x = 0, last_y = 0;

    while (devdata->eventbuffer->GetEvent(devdata->eventbuffer,
                                          DFB_EVENT(&evt)) == DFB_OK) {
        SDL_keysym keysym;
        DFBInputDeviceModifierMask mod;

        if (evt.clazz = DFEC_INPUT) {
            if (evt.flags & DIEF_MODIFIERS)
                mod = evt.modifiers;
            else
                mod = 0;

            switch (evt.type) {
            case DIET_BUTTONPRESS:
                posted += SDL_PrivateMouseButton(SDL_PRESSED,
                                                 DirectFB_TranslateButton(evt.
                                                                          button),
                                                 0, 0);
                break;
            case DIET_BUTTONRELEASE:
                posted += SDL_PrivateMouseButton(SDL_RELEASED,
                                                 DirectFB_TranslateButton(evt.
                                                                          button),
                                                 0, 0);
                break;
            case DIET_KEYPRESS:
                posted += SDL_PrivateKeyboard(SDL_PRESSED,
                                              DirectFB_TranslateKey(evt.
                                                                    key_id,
                                                                    evt.
                                                                    key_symbol,
                                                                    mod,
                                                                    &keysym));
                break;
            case DIET_KEYRELEASE:
                posted += SDL_PrivateKeyboard(SDL_RELEASED,
                                              DirectFB_TranslateKey(evt.
                                                                    key_id,
                                                                    evt.
                                                                    key_symbol,
                                                                    mod,
                                                                    &keysym));
                break;
            case DIET_AXISMOTION:
                if (evt.flags & DIEF_AXISREL) {
                    if (evt.axis == DIAI_X)
                        posted +=
                            SDL_PrivateMouseMotion(0, 1, evt.axisrel, 0);
                    else if (evt.axis == DIAI_Y)
                        posted +=
                            SDL_PrivateMouseMotion(0, 1, 0, evt.axisrel);
                } else if (evt.flags & DIEF_AXISABS) {
                    if (evt.axis == DIAI_X)
                        last_x = evt.axisabs;
                    else if (evt.axis == DIAI_Y)
                        last_y = evt.axisabs;
                    posted += SDL_PrivateMouseMotion(0, 0, last_x, last_y);
                }
                break;
            default:
                ;
            }
        }
    }
}
#endif
