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
#include "SDL_keynames.h"


/* Global keyboard information */
int SDL_TranslateUNICODE = 0;
static int SDL_num_keyboards;
static int SDL_current_keyboard;
static SDL_Keyboard **SDL_keyboards;

/* Public functions */
int
SDL_KeyboardInit(void)
{
    /* Set default mode of UNICODE translation */
    SDL_EnableUNICODE(DEFAULT_UNICODE_TRANSLATION);

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
    int key;

    if (!keyboard) {
        return;
    }

    for (key = SDLK_FIRST; key < SDLK_LAST; ++key) {
        if (keyboard->keystate[key] == SDL_PRESSED) {
            SDL_SendKeyboardKey(index, SDL_RELEASED, 0,
                                key | SDL_KEY_CAN_BE_PHYSICAL_BIT);
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

SDLKey
SDL_GetLayoutKey(SDLKey physicalKey)
{
    SDL_VideoDevice *_this = SDL_GetVideoDevice();
    if (_this && _this->GetLayoutKey) {
        return _this->GetLayoutKey(_this, physicalKey)
            | (physicalKey & SDL_KEY_KEYPAD_BIT);
    } else {
        return physicalKey;
    }
}

const char *
SDL_GetKeyName(SDLKey layoutKey)
{
    const char *keyname = NULL;

    if ((layoutKey & SDL_KEY_LAYOUT_SPECIAL_BIT) != 0) {
        SDL_VideoDevice *_this = SDL_GetVideoDevice();
        if (_this && _this->GetSpecialKeyName) {
            keyname = _this->GetSpecialKeyName(_this, layoutKey);
        }
    } else if ((layoutKey & SDL_KEY_CAN_BE_PHYSICAL_BIT) == 0) {
        /* SDLK_INDEX(layoutKey) is the unicode code point of the character generated by the key */
        static char buffer[9];  /* 6 (maximal UTF-8 char length) + 2 ([] for keypad) + 1 (null teminator) */
        char *bufferPtr = &buffer[1];
        SDL_iconv_t cd;
        size_t inbytesleft = 4, outbytesleft = 8;
        Uint32 codepoint = SDLK_INDEX(layoutKey);
        const char *codepointPtr = (const char *) &codepoint;

        /* Unaccented letter keys on latin keyboards are normally labeled in upper case (and probably on others like Greek or Cyrillic too, so if you happen to know for sure, please adapt this). */
        if (codepoint >= 'a' && codepoint <= 'z') {
            codepoint -= 32;
        }

        cd = SDL_iconv_open("UTF-8", "UCS-4");
        if (cd == (SDL_iconv_t) (-1))
            return "";
        SDL_iconv(cd, &codepointPtr, &inbytesleft, &bufferPtr, &outbytesleft);
        SDL_iconv_close(cd);
        *bufferPtr = '\0';

        if ((layoutKey & SDL_KEY_KEYPAD_BIT) != 0) {
            buffer[0] = '[';
            *bufferPtr++ = ']';
            *bufferPtr = '\0';
            keyname = buffer;
        } else {
            keyname = &buffer[1];
        }
    } else {
        /* SDLK_INDEX(layoutKey) is a physical key number */
        if (SDLK_INDEX(layoutKey) < SDL_arraysize(SDL_keynames)) {
            keyname = SDL_keynames[SDLK_INDEX(layoutKey)];
        }
    }

    if (keyname == NULL) {
        keyname = SDL_keynames[SDLK_INDEX(SDLK_UNKNOWN)];
    }

    return keyname;
}

void
SDL_SetKeyName(SDLKey physicalKey, const char *name)
{
    physicalKey = SDLK_INDEX(physicalKey);
    if (physicalKey < SDL_arraysize(SDL_keynames)) {
        SDL_keynames[physicalKey] = name;
    }
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
SDL_SendKeyboardKey(int index, Uint8 state, Uint8 scancode,
                    SDLKey physicalKey)
{
    SDL_Keyboard *keyboard = SDL_GetKeyboard(index);
    int posted;
    Uint16 modstate;
    Uint8 type;

    if (!keyboard || physicalKey == SDLK_NONE) {
        return 0;
    }
#if 0
    printf("The '%s' key has been %s\n", SDL_GetKeyName(physicalKey),
           state == SDL_PRESSED ? "pressed" : "released");
#endif
    if (state == SDL_PRESSED) {
        modstate = keyboard->modstate;
        switch (physicalKey) {
        case SDLK_UNKNOWN:
            break;
        case SDLK_KP_NUMLOCKCLEAR:
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
        switch (physicalKey) {
        case SDLK_UNKNOWN:
            break;
        case SDLK_KP_NUMLOCKCLEAR:
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

    if (physicalKey != SDLK_UNKNOWN) {
        /* Drop events that don't change state */
        if (keyboard->keystate[SDLK_INDEX(physicalKey)] == state) {
#if 0
            printf("Keyboard event didn't change state - dropped!\n");
#endif
            return 0;
        }

        /* Update internal keyboard state */
        keyboard->keystate[SDLK_INDEX(physicalKey)] = state;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_ProcessEvents[type] == SDL_ENABLE) {
        SDL_Event event;
        event.key.type = type;
        event.key.which = (Uint8) index;
        event.key.state = state;
        event.key.keysym.scancode = scancode;
        event.key.keysym.sym = physicalKey;
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
