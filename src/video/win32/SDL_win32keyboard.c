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

#include "SDL_win32video.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_win32.h"

void
WIN_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_Keyboard keyboard;

    data->key_layout = win32_scancode_table;

    SDL_zero(keyboard);
    data->keyboard = SDL_AddKeyboard(&keyboard, -1);
    WIN_UpdateKeymap(_this);

    SDL_SetScancodeName(SDL_SCANCODE_APPLICATION, "Menu");
    SDL_SetScancodeName(SDL_SCANCODE_LGUI, "Left Windows");
    SDL_SetScancodeName(SDL_SCANCODE_RGUI, "Right Windows");
}

void
WIN_UpdateKeymap(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int i;
    SDL_scancode scancode;
    SDLKey keymap[SDL_NUM_SCANCODES];

    SDL_GetDefaultKeymap(keymap);

    for (i = 0; i < SDL_arraysize(win32_scancode_table); i++) {

        /* Make sure this scancode is a valid character scancode */
        scancode = win32_scancode_table[i];
        if (scancode == SDL_SCANCODE_UNKNOWN ||
            (keymap[scancode] & SDLK_SCANCODE_MASK)) {
            continue;
        }
#ifndef MAPVK_VK_TO_CHAR
#define MAPVK_VK_TO_CHAR    2
#endif
        keymap[scancode] = (MapVirtualKey(i, MAPVK_VK_TO_CHAR) & 0x7FFF);
    }
    SDL_SetKeymap(data->keyboard, 0, keymap, SDL_NUM_SCANCODES);
}

void
WIN_QuitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    SDL_DelKeyboard(data->keyboard);
}

/* vi: set ts=4 sw=4 expandtab: */
