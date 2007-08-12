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

/* General keyboard handling code for SDL */

#include "SDL_timer.h"
#include "SDL_events.h"
#include "SDL_events_c.h"
#include "SDL_sysevents.h"


/* Global keyboard information */
int SDL_TranslateUNICODE = 0;
static int SDL_num_keyboards;
static int SDL_current_keyboard;
static SDL_Keyboard **SDL_keyboards;

static const char *SDL_keynames[SDLK_LAST];     /* Array of keycode names */

/* Public functions */
int
SDL_KeyboardInit(void)
{
    int i;

    /* Set default mode of UNICODE translation */
    SDL_EnableUNICODE(DEFAULT_UNICODE_TRANSLATION);

    /* Initialize the tables */
    for (i = 0; i < SDL_arraysize(SDL_keynames); ++i) {
        switch (i) {
        case SDLK_BACKSPACE:
            SDL_keynames[i] = "backspace";
            break;
        case SDLK_TAB:
            SDL_keynames[i] = "tab";
            break;
        case SDLK_CLEAR:
            SDL_keynames[i] = "clear";
            break;
        case SDLK_RETURN:
            SDL_keynames[i] = "return";
            break;
        case SDLK_PAUSE:
            SDL_keynames[i] = "pause";
            break;
        case SDLK_ESCAPE:
            SDL_keynames[i] = "escape";
            break;
        case SDLK_SPACE:
            SDL_keynames[i] = "space";
            break;

        case SDLK_KP0:
            SDL_keynames[i] = "[0]";
            break;
        case SDLK_KP1:
            SDL_keynames[i] = "[1]";
            break;
        case SDLK_KP2:
            SDL_keynames[i] = "[2]";
            break;
        case SDLK_KP3:
            SDL_keynames[i] = "[3]";
            break;
        case SDLK_KP4:
            SDL_keynames[i] = "[4]";
            break;
        case SDLK_KP5:
            SDL_keynames[i] = "[5]";
            break;
        case SDLK_KP6:
            SDL_keynames[i] = "[6]";
            break;
        case SDLK_KP7:
            SDL_keynames[i] = "[7]";
            break;
        case SDLK_KP8:
            SDL_keynames[i] = "[8]";
            break;
        case SDLK_KP9:
            SDL_keynames[i] = "[9]";
            break;
        case SDLK_KP_PERIOD:
            SDL_keynames[i] = "[.]";
            break;
        case SDLK_KP_DIVIDE:
            SDL_keynames[i] = "[/]";
            break;
        case SDLK_KP_MULTIPLY:
            SDL_keynames[i] = "[*]";
            break;
        case SDLK_KP_MINUS:
            SDL_keynames[i] = "[-]";
            break;
        case SDLK_KP_PLUS:
            SDL_keynames[i] = "[+]";
            break;
        case SDLK_KP_ENTER:
            SDL_keynames[i] = "enter";
            break;
        case SDLK_KP_EQUALS:
            SDL_keynames[i] = "equals";
            break;

        case SDLK_UP:
            SDL_keynames[i] = "up";
            break;
        case SDLK_DOWN:
            SDL_keynames[i] = "down";
            break;
        case SDLK_RIGHT:
            SDL_keynames[i] = "right";
            break;
        case SDLK_LEFT:
            SDL_keynames[i] = "left";
            break;
        case SDLK_INSERT:
            SDL_keynames[i] = "insert";
            break;
        case SDLK_HOME:
            SDL_keynames[i] = "home";
            break;
        case SDLK_END:
            SDL_keynames[i] = "end";
            break;
        case SDLK_PAGEUP:
            SDL_keynames[i] = "page up";
            break;
        case SDLK_PAGEDOWN:
            SDL_keynames[i] = "page down";
            break;

        case SDLK_F1:
            SDL_keynames[i] = "f1";
            break;
        case SDLK_F2:
            SDL_keynames[i] = "f2";
            break;
        case SDLK_F3:
            SDL_keynames[i] = "f3";
            break;
        case SDLK_F4:
            SDL_keynames[i] = "f4";
            break;
        case SDLK_F5:
            SDL_keynames[i] = "f5";
            break;
        case SDLK_F6:
            SDL_keynames[i] = "f6";
            break;
        case SDLK_F7:
            SDL_keynames[i] = "f7";
            break;
        case SDLK_F8:
            SDL_keynames[i] = "f8";
            break;
        case SDLK_F9:
            SDL_keynames[i] = "f9";
            break;
        case SDLK_F10:
            SDL_keynames[i] = "f10";
            break;
        case SDLK_F11:
            SDL_keynames[i] = "f11";
            break;
        case SDLK_F12:
            SDL_keynames[i] = "f12";
            break;
        case SDLK_F13:
            SDL_keynames[i] = "f13";
            break;
        case SDLK_F14:
            SDL_keynames[i] = "f14";
            break;
        case SDLK_F15:
            SDL_keynames[i] = "f15";
            break;

        case SDLK_NUMLOCK:
            SDL_keynames[i] = "numlock";
            break;
        case SDLK_CAPSLOCK:
            SDL_keynames[i] = "caps lock";
            break;
        case SDLK_SCROLLOCK:
            SDL_keynames[i] = "scroll lock";
            break;
        case SDLK_RSHIFT:
            SDL_keynames[i] = "right shift";
            break;
        case SDLK_LSHIFT:
            SDL_keynames[i] = "left shift";
            break;
        case SDLK_RCTRL:
            SDL_keynames[i] = "right ctrl";
            break;
        case SDLK_LCTRL:
            SDL_keynames[i] = "left ctrl";
            break;
        case SDLK_RALT:
            SDL_keynames[i] = "right alt";
            break;
        case SDLK_LALT:
            SDL_keynames[i] = "left alt";
            break;
        case SDLK_RMETA:
            SDL_keynames[i] = "right meta";
            break;
        case SDLK_LMETA:
            SDL_keynames[i] = "left meta";
            break;
        case SDLK_LSUPER:
            SDL_keynames[i] = "left super";     /* "Windows" keys */
            break;
        case SDLK_RSUPER:
            SDL_keynames[i] = "right super";
            break;
        case SDLK_MODE:
            SDL_keynames[i] = "alt gr";
            break;
        case SDLK_COMPOSE:
            SDL_keynames[i] = "compose";
            break;

        case SDLK_HELP:
            SDL_keynames[i] = "help";
            break;
        case SDLK_PRINT:
            SDL_keynames[i] = "print screen";
            break;
        case SDLK_SYSREQ:
            SDL_keynames[i] = "sys req";
            break;
        case SDLK_BREAK:
            SDL_keynames[i] = "break";
            break;
        case SDLK_MENU:
            SDL_keynames[i] = "menu";
            break;
        case SDLK_POWER:
            SDL_keynames[i] = "power";
            break;
        case SDLK_EURO:
            SDL_keynames[i] = "euro";
            break;
        case SDLK_UNDO:
            SDL_keynames[i] = "undo";
            break;

        default:
            SDL_keynames[i] = NULL;
            break;
        }
    }

    /* Done.  Whew. */
    return (0);
}

SDL_Keyboard *
SDL_GetKeyboard(int index)
{
    if (index < 0 || index >= SDL_num_keyboards) {
        return NULL;
    }
    return SDL_keyboards[index];
}

int
SDL_AddKeyboard(const SDL_Keyboard * keyboard, int index)
{
    SDL_Keyboard **keyboards;

    /* Add the keyboard to the list of keyboards */
    if (index < 0 || index >= SDL_num_keyboards || SDL_keyboards[index]) {
        keyboards =
            (SDL_Keyboard **) SDL_realloc(SDL_keyboards,
                                          (SDL_num_keyboards +
                                           1) * sizeof(*keyboards));
        if (!keyboards) {
            SDL_OutOfMemory();
            return -1;
        }

        SDL_keyboards = keyboards;
        index = SDL_num_keyboards++;
    }
    SDL_keyboards[index] =
        (SDL_Keyboard *) SDL_malloc(sizeof(*SDL_keyboards[index]));
    if (!SDL_keyboards[index]) {
        SDL_OutOfMemory();
        return -1;
    }
    *SDL_keyboards[index] = *keyboard;

    return index;
}

void
SDL_DelKeyboard(int index)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(index);

    if (!keyboard) {
        return;
    }

    if (keyboard->FreeKeyboard) {
        keyboard->FreeKeyboard(keyboard);
    }
    SDL_free(keyboard);

    SDL_keyboards[index] = NULL;
}

void
SDL_ResetKeyboard(int index)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(index);
    SDLKey key;

    if (!keyboard) {
        return;
    }

    for (key = SDLK_FIRST; key < SDLK_LAST; ++key) {
        if (keyboard->keystate[key] == SDL_PRESSED) {
            SDL_SendKeyboardKey(index, SDL_RELEASED, 0, key);
        }
    }
}

void
SDL_KeyboardQuit(void)
{
    int i;

    for (i = 0; i < SDL_num_keyboards; ++i) {
        SDL_DelKeyboard(i);
    }
    SDL_num_keyboards = 0;
    SDL_current_keyboard = 0;

    if (SDL_keyboards) {
        SDL_free(SDL_keyboards);
        SDL_keyboards = NULL;
    }
}

int
SDL_GetNumKeyboards(void)
{
    return SDL_num_keyboards;
}

int
SDL_SelectKeyboard(int index)
{
    if (index >= 0 && index < SDL_num_keyboards) {
        SDL_current_keyboard = index;
    }
    return SDL_current_keyboard;
}

int
SDL_EnableUNICODE(int enable)
{
    int old_mode;

    old_mode = SDL_TranslateUNICODE;
    if (enable >= 0) {
        SDL_TranslateUNICODE = enable;
    }
    return (old_mode);
}

Uint8 *
SDL_GetKeyState(int *numkeys)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(SDL_current_keyboard);

    if (numkeys != (int *) 0) {
        *numkeys = SDLK_LAST;
    }

    if (!keyboard) {
        return NULL;
    }
    return keyboard->keystate;
}

SDLMod
SDL_GetModState(void)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(SDL_current_keyboard);

    if (!keyboard) {
        return KMOD_NONE;
    }
    return keyboard->modstate;
}

void
SDL_SetModState(SDLMod modstate)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(SDL_current_keyboard);

    if (!keyboard) {
        return;
    }
    keyboard->modstate = modstate;
}

const char *
SDL_GetKeyName(SDLKey key)
{
    const char *keyname;

    if (key < SDL_arraysize(SDL_keynames)) {
        keyname = SDL_keynames[key];
    } else {
        keyname = NULL;
    }
    if (keyname == NULL) {
        if (key < 256) {
            static char temp[4];
            char *cvt;
            temp[0] = (char) key;
            temp[1] = '\0';
            cvt = SDL_iconv_string("UTF-8", "ISO-8859-1", temp, 1);
            if (cvt) {
                SDL_strlcpy(temp, cvt, SDL_arraysize(temp));
                SDL_free(cvt);
            }
            keyname = temp;
        } else {
            keyname = "unknown key";
        }
    }
    return keyname;
}

void
SDL_SetKeyboardFocus(int index, SDL_WindowID windowID)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(index);
    int i;
    SDL_bool focus;

    if (!keyboard || (keyboard->focus == windowID)) {
        return;
    }

    /* See if the current window has lost focus */
    if (keyboard->focus) {
        focus = SDL_FALSE;
        for (i = 0; i < SDL_num_keyboards; ++i) {
            SDL_Keyboard *check;
            if (i != index) {
                check = SDL_GetKeyboard(i);
                if (check && check->focus == keyboard->focus) {
                    focus = SDL_TRUE;
                    break;
                }
            }
        }
        if (!focus) {
            SDL_SendWindowEvent(keyboard->focus, SDL_WINDOWEVENT_FOCUS_LOST,
                                0, 0);
        }
    }

    keyboard->focus = windowID;

    if (keyboard->focus) {
        focus = SDL_FALSE;
        for (i = 0; i < SDL_num_keyboards; ++i) {
            SDL_Keyboard *check;
            if (i != index) {
                check = SDL_GetKeyboard(i);
                if (check && check->focus == keyboard->focus) {
                    focus = SDL_TRUE;
                    break;
                }
            }
        }
        if (!focus) {
            SDL_SendWindowEvent(keyboard->focus, SDL_WINDOWEVENT_FOCUS_GAINED,
                                0, 0);
        }
    }
}

int
SDL_SendKeyboardKey(int index, Uint8 state, Uint8 scancode, SDLKey key)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(index);
    int posted;
    Uint16 modstate;
    Uint8 type;

    if (!keyboard) {
        return 0;
    }
#if 0
    printf("The '%s' key has been %s\n", SDL_GetKeyName(key),
           state == SDL_PRESSED ? "pressed" : "released");
#endif
    if (state == SDL_PRESSED) {
        modstate = keyboard->modstate;
        switch (key) {
        case SDLK_UNKNOWN:
            break;
        case SDLK_NUMLOCK:
            keyboard->modstate ^= KMOD_NUM;
            break;
        case SDLK_CAPSLOCK:
            keyboard->modstate ^= KMOD_CAPS;
            break;
        case SDLK_LCTRL:
            keyboard->modstate |= KMOD_LCTRL;
            break;
        case SDLK_RCTRL:
            keyboard->modstate |= KMOD_RCTRL;
            break;
        case SDLK_LSHIFT:
            keyboard->modstate |= KMOD_LSHIFT;
            break;
        case SDLK_RSHIFT:
            keyboard->modstate |= KMOD_RSHIFT;
            break;
        case SDLK_LALT:
            keyboard->modstate |= KMOD_LALT;
            break;
        case SDLK_RALT:
            keyboard->modstate |= KMOD_RALT;
            break;
        case SDLK_LMETA:
            keyboard->modstate |= KMOD_LMETA;
            break;
        case SDLK_RMETA:
            keyboard->modstate |= KMOD_RMETA;
            break;
        case SDLK_MODE:
            keyboard->modstate |= KMOD_MODE;
            break;
        default:
            break;
        }
    } else {
        switch (key) {
        case SDLK_UNKNOWN:
            break;
        case SDLK_NUMLOCK:
        case SDLK_CAPSLOCK:
            break;
        case SDLK_LCTRL:
            keyboard->modstate &= ~KMOD_LCTRL;
            break;
        case SDLK_RCTRL:
            keyboard->modstate &= ~KMOD_RCTRL;
            break;
        case SDLK_LSHIFT:
            keyboard->modstate &= ~KMOD_LSHIFT;
            break;
        case SDLK_RSHIFT:
            keyboard->modstate &= ~KMOD_RSHIFT;
            break;
        case SDLK_LALT:
            keyboard->modstate &= ~KMOD_LALT;
            break;
        case SDLK_RALT:
            keyboard->modstate &= ~KMOD_RALT;
            break;
        case SDLK_LMETA:
            keyboard->modstate &= ~KMOD_LMETA;
            break;
        case SDLK_RMETA:
            keyboard->modstate &= ~KMOD_RMETA;
            break;
        case SDLK_MODE:
            keyboard->modstate &= ~KMOD_MODE;
            break;
        default:
            break;
        }
        modstate = keyboard->modstate;
    }

    /* Figure out what type of event this is */
    switch (state) {
    case SDL_PRESSED:
        type = SDL_KEYDOWN;
        break;
    case SDL_RELEASED:
        type = SDL_KEYUP;
        break;
    default:
        /* Invalid state -- bail */
        return 0;
    }

    if (key != SDLK_UNKNOWN) {
        /* Drop events that don't change state */
        if (keyboard->keystate[key] == state) {
#if 0
            printf("Keyboard event didn't change state - dropped!\n");
#endif
            return 0;
        }

        /* Update internal keyboard state */
        keyboard->keystate[key] = state;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_ProcessEvents[type] == SDL_ENABLE) {
        SDL_Event event;
        event.key.type = type;
        event.key.which = (Uint8) index;
        event.key.state = state;
        event.key.keysym.scancode = scancode;
        event.key.keysym.sym = (Uint16) key;
        event.key.keysym.mod = modstate;
        event.key.keysym.unicode = 0;
        event.key.windowID = keyboard->focus;
        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}

int
SDL_SendKeyboardText(int index, const char *text)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(index);
    int posted;

    if (!keyboard) {
        return 0;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_ProcessEvents[SDL_TEXTINPUT] == SDL_ENABLE) {
        SDL_Event event;
        event.text.type = SDL_TEXTINPUT;
        event.text.which = (Uint8) index;
        SDL_strlcpy(event.text.text, text, SDL_arraysize(event.text.text));
        event.text.windowID = keyboard->focus;
        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}

/* vi: set ts=4 sw=4 expandtab: */
