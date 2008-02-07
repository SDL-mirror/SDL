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

#include "SDL_x11video.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_darwin.h"
#include "../../events/scancodes_xfree86.h"

#include <X11/keysym.h>

#include "imKStoUCS.h"

static KeySym XKeySymTable[SDL_NUM_SCANCODES] = {
    0, 0, 0, 0,
    XK_a,
    XK_b,
    XK_c,
    XK_d,
    XK_e,
    XK_f,
    XK_g,
    XK_h,
    XK_i,
    XK_j,
    XK_k,
    XK_l,
    XK_m,
    XK_n,
    XK_o,
    XK_p,
    XK_q,
    XK_r,
    XK_s,
    XK_t,
    XK_u,
    XK_v,
    XK_w,
    XK_x,
    XK_y,
    XK_z,
    XK_1,
    XK_2,
    XK_3,
    XK_4,
    XK_5,
    XK_6,
    XK_7,
    XK_8,
    XK_9,
    XK_0,
    XK_Return,
    XK_Escape,
    XK_BackSpace,
    XK_Tab,
    XK_space,
    XK_minus,
    XK_equal,
    XK_bracketleft,
    XK_bracketright,
    XK_backslash,
    0,                          /* SDL_SCANCODE_NONUSHASH ? */
    XK_semicolon,
    XK_apostrophe,
    XK_grave,
    XK_comma,
    XK_period,
    XK_slash,
    XK_Caps_Lock,
    XK_F1,
    XK_F2,
    XK_F3,
    XK_F4,
    XK_F5,
    XK_F6,
    XK_F7,
    XK_F8,
    XK_F9,
    XK_F10,
    XK_F11,
    XK_F12,
    XK_Print,
    XK_Scroll_Lock,
    XK_Pause,
    XK_Insert,
    XK_Home,
    XK_Prior,
    XK_Delete,
    XK_End,
    XK_Next,
    XK_Right,
    XK_Left,
    XK_Down,
    XK_Up,
    XK_Num_Lock,
    XK_KP_Divide,
    XK_KP_Multiply,
    XK_KP_Subtract,
    XK_KP_Add,
    XK_KP_Enter,
    XK_KP_1,
    XK_KP_2,
    XK_KP_3,
    XK_KP_4,
    XK_KP_5,
    XK_KP_6,
    XK_KP_7,
    XK_KP_8,
    XK_KP_9,
    XK_KP_0,
    XK_KP_Decimal,
    0,                          /* SDL_SCANCODE_NONUSBACKSLASH ? */
    XK_Hyper_R,
    0,                          /* SDL_SCANCODE_POWER ? */
    XK_KP_Equal,
    XK_F13,
    XK_F14,
    XK_F15,
    XK_F16,
    XK_F17,
    XK_F18,
    XK_F19,
    XK_F20,
    XK_F21,
    XK_F22,
    XK_F23,
    XK_F24,
    XK_Execute,
    XK_Help,
    XK_Menu,
    XK_Select,
    XK_Cancel,
    XK_Redo,
    XK_Undo,
    0,                          /* SDL_SCANCODE_CUT ? */
    0,                          /* SDL_SCANCODE_COPY ? */
    0,                          /* SDL_SCANCODE_PASTE ? */
    XK_Find,
    0,                          /* SDL_SCANCODE_MUTE ? */
    0,                          /* SDL_SCANCODE_VOLUMEUP ? */
    0,                          /* SDL_SCANCODE_VOLUMEDOWN ? */
    0, 0, 0,
    XK_KP_Separator,
    0,                          /* SDL_SCANCODE_KP_EQUALSAS400 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL1 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL2 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL3 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL4 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL5 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL6 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL7 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL8 ? */
    0,                          /* SDL_SCANCODE_INTERNATIONAL9 ? */
    0,                          /* SDL_SCANCODE_LANG1 ? */
    0,                          /* SDL_SCANCODE_LANG2 ? */
    0,                          /* SDL_SCANCODE_LANG3 ? */
    0,                          /* SDL_SCANCODE_LANG4 ? */
    0,                          /* SDL_SCANCODE_LANG5 ? */
    0,                          /* SDL_SCANCODE_LANG6 ? */
    0,                          /* SDL_SCANCODE_LANG7 ? */
    0,                          /* SDL_SCANCODE_LANG8 ? */
    0,                          /* SDL_SCANCODE_LANG9 ? */
    0,                          /* SDL_SCANCODE_ALTERASE ? */
    XK_Sys_Req,
    0,                          /* SDL_SCANCODE_CANCEL ? - XK_Cancel was used above... */
    0,                          /* SDL_SCANCODE_CLEAR ? */
    0,                          /* SDL_SCANCODE_PRIOR ? - XK_Prior was used above... */
    0,                          /* SDL_SCANCODE_RETURN2 ? */
    0,                          /* SDL_SCANCODE_SEPARATOR ? */
    0,                          /* SDL_SCANCODE_OUT ? */
    0,                          /* SDL_SCANCODE_OPER ? */
    0,                          /* SDL_SCANCODE_CLEARAGAIN ? */
    0,                          /* SDL_SCANCODE_CRSEL ? */
    0,                          /* SDL_SCANCODE_EXSEL ? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,                          /* SDL_SCANCODE_KP_00 ? */
    0,                          /* SDL_SCANCODE_KP_000 ? */
    0,                          /* SDL_SCANCODE_THOUSANDSSEPARATOR ? */
    0,                          /* SDL_SCANCODE_DECIMALSEPARATOR ? */
    0,                          /* SDL_SCANCODE_CURRENCYUNIT ? */
    0,                          /* SDL_SCANCODE_CURRENCYSUBUNIT ? */
    0,                          /* SDL_SCANCODE_KP_LEFTPAREN ? */
    0,                          /* SDL_SCANCODE_KP_RIGHTPAREN ? */
    0,                          /* SDL_SCANCODE_KP_LEFTBRACE ? */
    0,                          /* SDL_SCANCODE_KP_RIGHTBRACE ? */
    0,                          /* SDL_SCANCODE_KP_TAB ? */
    0,                          /* SDL_SCANCODE_KP_BACKSPACE ? */
    0,                          /* SDL_SCANCODE_KP_A ? */
    0,                          /* SDL_SCANCODE_KP_B ? */
    0,                          /* SDL_SCANCODE_KP_C ? */
    0,                          /* SDL_SCANCODE_KP_D ? */
    0,                          /* SDL_SCANCODE_KP_E ? */
    0,                          /* SDL_SCANCODE_KP_F ? */
    0,                          /* SDL_SCANCODE_KP_XOR ? */
    0,                          /* SDL_SCANCODE_KP_POWER ? */
    0,                          /* SDL_SCANCODE_KP_PERCENT ? */
    0,                          /* SDL_SCANCODE_KP_LESS ? */
    0,                          /* SDL_SCANCODE_KP_GREATER ? */
    0,                          /* SDL_SCANCODE_KP_AMPERSAND ? */
    0,                          /* SDL_SCANCODE_KP_DBLAMPERSAND ? */
    0,                          /* SDL_SCANCODE_KP_VERTICALBAR ? */
    0,                          /* SDL_SCANCODE_KP_DBLVERTICALBAR ? */
    0,                          /* SDL_SCANCODE_KP_COLON ? */
    0,                          /* SDL_SCANCODE_KP_HASH ? */
    0,                          /* SDL_SCANCODE_KP_SPACE ? */
    0,                          /* SDL_SCANCODE_KP_AT ? */
    0,                          /* SDL_SCANCODE_KP_EXCLAM ? */
    0,                          /* SDL_SCANCODE_KP_MEMSTORE ? */
    0,                          /* SDL_SCANCODE_KP_MEMRECALL ? */
    0,                          /* SDL_SCANCODE_KP_MEMCLEAR ? */
    0,                          /* SDL_SCANCODE_KP_MEMADD ? */
    0,                          /* SDL_SCANCODE_KP_MEMSUBTRACT ? */
    0,                          /* SDL_SCANCODE_KP_MEMMULTIPLY ? */
    0,                          /* SDL_SCANCODE_KP_MEMDIVIDE ? */
    0,                          /* SDL_SCANCODE_KP_PLUSMINUS ? */
    0,                          /* SDL_SCANCODE_KP_CLEAR ? */
    0,                          /* SDL_SCANCODE_KP_CLEARENTRY ? */
    0,                          /* SDL_SCANCODE_KP_BINARY ? */
    0,                          /* SDL_SCANCODE_KP_OCTAL ? */
    0,                          /* SDL_SCANCODE_KP_DECIMAL ? */
    0,                          /* SDL_SCANCODE_KP_HEXADECIMAL ? */
    0, 0,
    XK_Control_L,
    XK_Shift_L,
    XK_Alt_L,
    XK_Meta_L,
    XK_Control_R,
    XK_Shift_R,
    XK_Alt_R,
    XK_Meta_R,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    XK_Mode_switch /*XK_ISO_Level3_Shift */ ,
    0,                          /* SDL_SCANCODE_AUDIONEXT ? */
    0,                          /* SDL_SCANCODE_AUDIOPREV ? */
    0,                          /* SDL_SCANCODE_AUDIOSTOP ? */
    0,                          /* SDL_SCANCODE_AUDIOPLAY ? */
    0,                          /* SDL_SCANCODE_AUDIOMUTE ? */
    0,                          /* SDL_SCANCODE_MEDIASELECT ? */
    0,                          /* SDL_SCANCODE_WWW ? */
    0,                          /* SDL_SCANCODE_MAIL ? */
    0,                          /* SDL_SCANCODE_CALCULATOR ? */
    0,                          /* SDL_SCANCODE_COMPUTER ? */
    0,                          /* SDL_SCANCODE_AC_SEARCH ? */
    0,                          /* SDL_SCANCODE_AC_HOME ? */
    0,                          /* SDL_SCANCODE_AC_BACK ? */
    0,                          /* SDL_SCANCODE_AC_FORWARD ? */
    0,                          /* SDL_SCANCODE_AC_STOP ? */
    0,                          /* SDL_SCANCODE_AC_REFRESH ? */
    0,                          /* SDL_SCANCODE_AC_BOOKMARKS ? */
    0,                          /* SDL_SCANCODE_BRIGHTNESSDOWN ? */
    0,                          /* SDL_SCANCODE_BRIGHTNESSUP ? */
    0,                          /* SDL_SCANCODE_DISPLAYSWITCH ? */
    0,                          /* SDL_SCANCODE_KBDILLUMTOGGLE ? */
    0,                          /* SDL_SCANCODE_KBDILLUMDOWN ? */
    0,                          /* SDL_SCANCODE_KBDILLUMUP ? */
    0,                          /* SDL_SCANCODE_EJECT ? */
    0,                          /* SDL_SCANCODE_SLEEP ? */
};

static struct
{
    SDL_scancode *table;
    int table_size;
} scancode_set[] = {
    {
    darwin_scancode_table, SDL_arraysize(darwin_scancode_table)}, {
xfree86_scancode_table, SDL_arraysize(xfree86_scancode_table)},};

int
X11_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_Keyboard keyboard;
    int i, j;
    int min_keycode, max_keycode;
    SDL_scancode fingerprint_scancodes[] = {
        SDL_SCANCODE_HOME,
        SDL_SCANCODE_PAGEUP,
        SDL_SCANCODE_PAGEDOWN
    };
    int fingerprint[3];
    SDL_bool fingerprint_detected;

    XAutoRepeatOn(data->display);

    /* Try to determine which scancodes are being used based on fingerprint */
    fingerprint_detected = SDL_FALSE;
    XDisplayKeycodes(data->display, &min_keycode, &max_keycode);
    for (i = 0; i < SDL_arraysize(fingerprint_scancodes); ++i) {
        fingerprint[i] =
            XKeysymToKeycode(data->display,
                             XKeySymTable[fingerprint_scancodes[i]]) -
            min_keycode;
    }
    for (i = 0; i < SDL_arraysize(scancode_set); ++i) {
        /* Make sure the scancode set isn't too big */
        if ((max_keycode - min_keycode + 1) <= scancode_set[i].table_size) {
            continue;
        }
        for (j = 0; j < SDL_arraysize(fingerprint); ++j) {
            if (fingerprint[j] < 0
                || fingerprint[j] >= scancode_set[i].table_size) {
                break;
            }
            if (scancode_set[i].table[fingerprint[j]] !=
                fingerprint_scancodes[j]) {
                break;
            }
        }
        if (j == SDL_arraysize(fingerprint)) {
            printf("Using scancode set %d\n", i);
            SDL_memcpy(&data->key_layout[min_keycode], scancode_set[i].table,
                       sizeof(SDL_scancode) * scancode_set[i].table_size);
            fingerprint_detected = SDL_TRUE;
            break;
        }
    }

    if (!fingerprint_detected) {
        printf
            ("Keyboard layout unknown, please send the following to the SDL mailing list (sdl@libsdl.org):\n");

        /* Determine key_layout - only works on US QWERTY layout */
        for (i = min_keycode; i <= max_keycode; ++i) {
            KeySym sym;
            sym = XKeycodeToKeysym(data->display, i, 0);
            if (sym) {
                printf("code = %d, sym = 0x%X (%s) ", i - min_keycode, sym,
                       XKeysymToString(sym));
                for (j = 0; j < SDL_arraysize(XKeySymTable); ++j) {
                    if (XKeySymTable[j] == sym) {
                        data->key_layout[i] = (SDL_scancode) j;
                        break;
                    }
                }
                if (j == SDL_arraysize(XKeySymTable)) {
                    printf("scancode not found\n");
                } else {
                    printf("scancode = %d (%s)\n", j, SDL_GetScancodeName(j));
                }
            }
        }
    }

    SDL_zero(keyboard);
    data->keyboard = SDL_AddKeyboard(&keyboard, -1);
    X11_UpdateKeymap(this);

    SDL_SetScancodeName(SDL_SCANCODE_APPLICATION, "Menu");

    return 0;
}

void
X11_UpdateKeymap(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int i;
    SDL_scancode scancode;
    SDLKey keymap[SDL_NUM_SCANCODES];

    SDL_GetDefaultKeymap(keymap);

    for (i = 0; i < SDL_arraysize(data->key_layout); i++) {

        /* Make sure this scancode is a valid character scancode */
        scancode = data->key_layout[i];
        if (scancode == SDL_SCANCODE_UNKNOWN ||
            (keymap[scancode] & SDLK_SCANCODE_MASK)) {
            continue;
        }

        keymap[scancode] =
            (SDLKey) X11_KeySymToUcs4(XKeycodeToKeysym(data->display, i, 0));
    }
    SDL_SetKeymap(data->keyboard, 0, keymap, SDL_NUM_SCANCODES);
}

void
X11_QuitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    SDL_DelKeyboard(data->keyboard);
}

/* vi: set ts=4 sw=4 expandtab: */
