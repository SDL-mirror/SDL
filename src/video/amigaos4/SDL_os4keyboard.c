/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_AMIGAOS4

#include "SDL_os4video.h"
#include "SDL_os4keyboard.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_amiga.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static SDL_Keycode
OS4_MapRawKey(_THIS, int code)
{
    struct InputEvent ie;
    int res;
    char buffer[2] = {0, 0};

    ie.ie_Class = IECLASS_RAWKEY;
    ie.ie_SubClass = 0;
    ie.ie_Code = code;
    ie.ie_Qualifier = 0;
    ie.ie_EventAddress = NULL;

    res = IKeymap->MapRawKey(&ie, buffer, sizeof(buffer), NULL);
    if (res > 0) {
        return (buffer[0] + buffer[1] * 256);
    } else {
        return 0;
    }
}

static void
OS4_UpdateKeymap(_THIS)
{
    int i;
    SDL_Scancode scancode;
    SDL_Keycode keymap[SDL_NUM_SCANCODES];

    SDL_GetDefaultKeymap(keymap);

    for (i = 0; i < SDL_arraysize(amiga_scancode_table); i++) {
        /* Make sure this scancode is a valid character scancode */
        scancode = amiga_scancode_table[i];
        if (scancode == SDL_SCANCODE_UNKNOWN ) {
            continue;
        }

        /* If this key is one of the non-mappable keys, ignore it */
        /* Don't allow the number keys right above the qwerty row to translate or the top left key (grave/backquote) */
        /* Not mapping numbers fixes the French layout, giving numeric keycodes for the number keys, which is the expected behavior */
        if ((keymap[scancode] & SDLK_SCANCODE_MASK) ||
            scancode == SDL_SCANCODE_GRAVE /*||
            (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_0)*/ ) {
            continue;
        }

        keymap[scancode] = OS4_MapRawKey(_this, i);
    }

    SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
}

int
OS4_SetClipboardText(_THIS, const char *text)
{
    LONG result = ITextClip->WriteClipVector(text, SDL_strlen(text));

    //dprintf("Result %s\n", result ? "OK" : "NOK");

    return result ? 0 : -1;
}

char *
OS4_GetClipboardText(_THIS)
{
    STRPTR from;
    ULONG size;
    char *to = NULL;

    LONG result = ITextClip->ReadClipVector(&from, &size);

    //dprintf("Read '%s' (%d bytes) from clipboard\n", from, size);

    if (result) {

        if (size) {
            to = SDL_malloc( ++size );

            if (to) {
               SDL_strlcpy(to, from, size);
            } else {
                dprintf("Failed to allocate memory\n");
            }
        } else {
            to = SDL_strdup("");
        }

        ITextClip->DisposeClipVector(from);
    }

    return to;
}

SDL_bool
OS4_HasClipboardText(_THIS)
{
    /* This is silly but is there a better way to check? */
    char *to = OS4_GetClipboardText(_this);

    if (to) {
        size_t len = SDL_strlen(to);

        SDL_free(to);

        if (len > 0) {
            return SDL_TRUE;
        }
    }

    return SDL_FALSE;
}

void
OS4_InitKeyboard(_THIS)
{
    OS4_UpdateKeymap(_this);

    //SDL_SetScancodeName(SDL_SCANCODE_APPLICATION, "Menu");
    SDL_SetScancodeName(SDL_SCANCODE_LGUI, "Left Amiga");
    SDL_SetScancodeName(SDL_SCANCODE_RGUI, "Right Amiga");
    SDL_SetScancodeName(SDL_SCANCODE_LCTRL, "Control");
}

void
OS4_QuitKeyboard(_THIS)
{
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
