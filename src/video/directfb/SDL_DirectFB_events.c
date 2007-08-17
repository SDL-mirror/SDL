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

#include "SDL.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include "SDL_DirectFB_events.h"

/* The translation tables from a DirectFB keycode to a SDL keysym */
static SDLKey keymap[256];

static SDL_keysym *DirectFB_TranslateKey(DFBInputDeviceKeyIdentifier key_id,
                                         DFBInputDeviceKeySymbol key_symbol,
                                         DFBInputDeviceModifierMask key_mod,
                                         SDL_keysym * keysym);

static int DirectFB_TranslateButton(DFBInputDeviceButtonIdentifier button);

void
DirectFB_PumpEventsWindow(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);
    DFB_WindowData *p;
    DFBWindowEvent evt;

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
                    DirectFB_TranslateKey(evt.key_id, evt.key_symbol,
                                          evt.modifiers, &keysym);
                    SDL_SendKeyboardKey(devdata->keyboard, SDL_PRESSED,
                                        keysym.scancode, keysym.sym);
                    break;
                case DWET_KEYUP:
                    DirectFB_TranslateKey(evt.key_id, evt.key_symbol,
                                          evt.modifiers, &keysym);
                    SDL_SendKeyboardKey(devdata->keyboard, SDL_RELEASED,
                                        keysym.scancode, keysym.sym);
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
                    //TODO: Implement for yuv-overlay DirectFB_SwitchOverlayContext(this, evt.window_id);
                    SDL_SetKeyboardFocus(devdata->keyboard, p->id);
                    break;
                case DWET_LOSTFOCUS:
                    SDL_SetKeyboardFocus(devdata->keyboard, 0);
                    break;
                case DWET_ENTER:
                    //SDL_DirectFB_ReshowCursor(_this, 0);
                    SDL_SetMouseFocus(devdata->mouse, p->id);
                    break;
                case DWET_LEAVE:
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
        keymap[i] = SDLK_UNKNOWN;

    keymap[DIKI_A - DIKI_UNKNOWN] = SDLK_a;
    keymap[DIKI_B - DIKI_UNKNOWN] = SDLK_b;
    keymap[DIKI_C - DIKI_UNKNOWN] = SDLK_c;
    keymap[DIKI_D - DIKI_UNKNOWN] = SDLK_d;
    keymap[DIKI_E - DIKI_UNKNOWN] = SDLK_e;
    keymap[DIKI_F - DIKI_UNKNOWN] = SDLK_f;
    keymap[DIKI_G - DIKI_UNKNOWN] = SDLK_g;
    keymap[DIKI_H - DIKI_UNKNOWN] = SDLK_h;
    keymap[DIKI_I - DIKI_UNKNOWN] = SDLK_i;
    keymap[DIKI_J - DIKI_UNKNOWN] = SDLK_j;
    keymap[DIKI_K - DIKI_UNKNOWN] = SDLK_k;
    keymap[DIKI_L - DIKI_UNKNOWN] = SDLK_l;
    keymap[DIKI_M - DIKI_UNKNOWN] = SDLK_m;
    keymap[DIKI_N - DIKI_UNKNOWN] = SDLK_n;
    keymap[DIKI_O - DIKI_UNKNOWN] = SDLK_o;
    keymap[DIKI_P - DIKI_UNKNOWN] = SDLK_p;
    keymap[DIKI_Q - DIKI_UNKNOWN] = SDLK_q;
    keymap[DIKI_R - DIKI_UNKNOWN] = SDLK_r;
    keymap[DIKI_S - DIKI_UNKNOWN] = SDLK_s;
    keymap[DIKI_T - DIKI_UNKNOWN] = SDLK_t;
    keymap[DIKI_U - DIKI_UNKNOWN] = SDLK_u;
    keymap[DIKI_V - DIKI_UNKNOWN] = SDLK_v;
    keymap[DIKI_W - DIKI_UNKNOWN] = SDLK_w;
    keymap[DIKI_X - DIKI_UNKNOWN] = SDLK_x;
    keymap[DIKI_Y - DIKI_UNKNOWN] = SDLK_y;
    keymap[DIKI_Z - DIKI_UNKNOWN] = SDLK_z;

    keymap[DIKI_0 - DIKI_UNKNOWN] = SDLK_0;
    keymap[DIKI_1 - DIKI_UNKNOWN] = SDLK_1;
    keymap[DIKI_2 - DIKI_UNKNOWN] = SDLK_2;
    keymap[DIKI_3 - DIKI_UNKNOWN] = SDLK_3;
    keymap[DIKI_4 - DIKI_UNKNOWN] = SDLK_4;
    keymap[DIKI_5 - DIKI_UNKNOWN] = SDLK_5;
    keymap[DIKI_6 - DIKI_UNKNOWN] = SDLK_6;
    keymap[DIKI_7 - DIKI_UNKNOWN] = SDLK_7;
    keymap[DIKI_8 - DIKI_UNKNOWN] = SDLK_8;
    keymap[DIKI_9 - DIKI_UNKNOWN] = SDLK_9;

    keymap[DIKI_F1 - DIKI_UNKNOWN] = SDLK_F1;
    keymap[DIKI_F2 - DIKI_UNKNOWN] = SDLK_F2;
    keymap[DIKI_F3 - DIKI_UNKNOWN] = SDLK_F3;
    keymap[DIKI_F4 - DIKI_UNKNOWN] = SDLK_F4;
    keymap[DIKI_F5 - DIKI_UNKNOWN] = SDLK_F5;
    keymap[DIKI_F6 - DIKI_UNKNOWN] = SDLK_F6;
    keymap[DIKI_F7 - DIKI_UNKNOWN] = SDLK_F7;
    keymap[DIKI_F8 - DIKI_UNKNOWN] = SDLK_F8;
    keymap[DIKI_F9 - DIKI_UNKNOWN] = SDLK_F9;
    keymap[DIKI_F10 - DIKI_UNKNOWN] = SDLK_F10;
    keymap[DIKI_F11 - DIKI_UNKNOWN] = SDLK_F11;
    keymap[DIKI_F12 - DIKI_UNKNOWN] = SDLK_F12;

    keymap[DIKI_ESCAPE - DIKI_UNKNOWN] = SDLK_ESCAPE;
    keymap[DIKI_LEFT - DIKI_UNKNOWN] = SDLK_LEFT;
    keymap[DIKI_RIGHT - DIKI_UNKNOWN] = SDLK_RIGHT;
    keymap[DIKI_UP - DIKI_UNKNOWN] = SDLK_UP;
    keymap[DIKI_DOWN - DIKI_UNKNOWN] = SDLK_DOWN;
    keymap[DIKI_CONTROL_L - DIKI_UNKNOWN] = SDLK_LCTRL;
    keymap[DIKI_CONTROL_R - DIKI_UNKNOWN] = SDLK_RCTRL;
    keymap[DIKI_SHIFT_L - DIKI_UNKNOWN] = SDLK_LSHIFT;
    keymap[DIKI_SHIFT_R - DIKI_UNKNOWN] = SDLK_RSHIFT;
    keymap[DIKI_ALT_L - DIKI_UNKNOWN] = SDLK_LALT;
    keymap[DIKI_ALT_R - DIKI_UNKNOWN] = SDLK_RALT;
    keymap[DIKI_TAB - DIKI_UNKNOWN] = SDLK_TAB;
    keymap[DIKI_ENTER - DIKI_UNKNOWN] = SDLK_RETURN;
    keymap[DIKI_SPACE - DIKI_UNKNOWN] = SDLK_SPACE;
    keymap[DIKI_BACKSPACE - DIKI_UNKNOWN] = SDLK_BACKSPACE;
    keymap[DIKI_INSERT - DIKI_UNKNOWN] = SDLK_INSERT;
    keymap[DIKI_DELETE - DIKI_UNKNOWN] = SDLK_DELETE;
    keymap[DIKI_HOME - DIKI_UNKNOWN] = SDLK_HOME;
    keymap[DIKI_END - DIKI_UNKNOWN] = SDLK_END;
    keymap[DIKI_PAGE_UP - DIKI_UNKNOWN] = SDLK_PAGEUP;
    keymap[DIKI_PAGE_DOWN - DIKI_UNKNOWN] = SDLK_PAGEDOWN;
    keymap[DIKI_CAPS_LOCK - DIKI_UNKNOWN] = SDLK_CAPSLOCK;
    keymap[DIKI_NUM_LOCK - DIKI_UNKNOWN] = SDLK_NUMLOCK;
    keymap[DIKI_SCROLL_LOCK - DIKI_UNKNOWN] = SDLK_SCROLLOCK;
    keymap[DIKI_PRINT - DIKI_UNKNOWN] = SDLK_PRINT;
    keymap[DIKI_PAUSE - DIKI_UNKNOWN] = SDLK_PAUSE;

    keymap[DIKI_KP_EQUAL - DIKI_UNKNOWN] = SDLK_KP_EQUALS;
    keymap[DIKI_KP_DECIMAL - DIKI_UNKNOWN] = SDLK_KP_PERIOD;
    keymap[DIKI_KP_0 - DIKI_UNKNOWN] = SDLK_KP0;
    keymap[DIKI_KP_1 - DIKI_UNKNOWN] = SDLK_KP1;
    keymap[DIKI_KP_2 - DIKI_UNKNOWN] = SDLK_KP2;
    keymap[DIKI_KP_3 - DIKI_UNKNOWN] = SDLK_KP3;
    keymap[DIKI_KP_4 - DIKI_UNKNOWN] = SDLK_KP4;
    keymap[DIKI_KP_5 - DIKI_UNKNOWN] = SDLK_KP5;
    keymap[DIKI_KP_6 - DIKI_UNKNOWN] = SDLK_KP6;
    keymap[DIKI_KP_7 - DIKI_UNKNOWN] = SDLK_KP7;
    keymap[DIKI_KP_8 - DIKI_UNKNOWN] = SDLK_KP8;
    keymap[DIKI_KP_9 - DIKI_UNKNOWN] = SDLK_KP9;
    keymap[DIKI_KP_DIV - DIKI_UNKNOWN] = SDLK_KP_DIVIDE;
    keymap[DIKI_KP_MULT - DIKI_UNKNOWN] = SDLK_KP_MULTIPLY;
    keymap[DIKI_KP_MINUS - DIKI_UNKNOWN] = SDLK_KP_MINUS;
    keymap[DIKI_KP_PLUS - DIKI_UNKNOWN] = SDLK_KP_PLUS;
    keymap[DIKI_KP_ENTER - DIKI_UNKNOWN] = SDLK_KP_ENTER;

    keymap[DIKI_QUOTE_LEFT - DIKI_UNKNOWN] = SDLK_BACKQUOTE;    /*  TLDE  */
    keymap[DIKI_MINUS_SIGN - DIKI_UNKNOWN] = SDLK_MINUS;        /*  AE11  */
    keymap[DIKI_EQUALS_SIGN - DIKI_UNKNOWN] = SDLK_EQUALS;      /*  AE12  */
    keymap[DIKI_BRACKET_LEFT - DIKI_UNKNOWN] = SDLK_RIGHTBRACKET;       /*  AD11  */
    keymap[DIKI_BRACKET_RIGHT - DIKI_UNKNOWN] = SDLK_LEFTBRACKET;       /*  AD12  */
    keymap[DIKI_BACKSLASH - DIKI_UNKNOWN] = SDLK_BACKSLASH;     /*  BKSL  */
    keymap[DIKI_SEMICOLON - DIKI_UNKNOWN] = SDLK_SEMICOLON;     /*  AC10  */
    keymap[DIKI_QUOTE_RIGHT - DIKI_UNKNOWN] = SDLK_QUOTE;       /*  AC11  */
    keymap[DIKI_COMMA - DIKI_UNKNOWN] = SDLK_COMMA;     /*  AB08  */
    keymap[DIKI_PERIOD - DIKI_UNKNOWN] = SDLK_PERIOD;   /*  AB09  */
    keymap[DIKI_SLASH - DIKI_UNKNOWN] = SDLK_SLASH;     /*  AB10  */
    keymap[DIKI_LESS_SIGN - DIKI_UNKNOWN] = SDLK_LESS;  /*  103rd  */
}

static SDL_keysym *
DirectFB_TranslateKey(DFBInputDeviceKeyIdentifier key_id,
                      DFBInputDeviceKeySymbol key_symbol,
                      DFBInputDeviceModifierMask key_mod, SDL_keysym * keysym)
{
    SDLMod mod = KMOD_NONE;

    /*
     *  Set modifier information 
     */

    if (key_mod & DIMM_SHIFT)
        mod = mod | KMOD_LSHIFT;
    if (key_mod & DIMM_CONTROL)
        mod = mod | KMOD_LCTRL;
    if (key_mod & DIMM_ALT)
        mod = mod | KMOD_LALT;
    if (key_mod & DIMM_ALTGR)
        mod = mod | KMOD_RALT;
    if (key_mod & DIMM_META)
        mod = mod | KMOD_LMETA;

    /* Set the keysym information */
    keysym->scancode = key_id;

    keysym->mod = mod;
    keysym->unicode =
        (DFB_KEY_TYPE(key_symbol) == DIKT_UNICODE) ? key_symbol : 0;

    if (key_symbol > 0 && key_symbol < 255)
        keysym->sym = key_symbol;
    else
        keysym->sym = keymap[key_id - DIKI_UNKNOWN];

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

#if 0
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
