/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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

/* Being a null driver, there's no event stream. We just define stubs for
   most of the API. */

#include <stdio.h>
#include <stdlib.h>

#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"

#include "SDL_androidevents.h"

void Android_InitEvents(){

    SDL_Keyboard keyboard;

    SDL_zero(keyboard);
    SDL_AddKeyboard(&keyboard, -1);

    SDLKey keymap[SDL_NUM_SCANCODES];

    /* Add default scancode to key mapping */
    SDL_GetDefaultKeymap(keymap);
    SDL_SetKeymap(0, 0, keymap, SDL_NUM_SCANCODES);


}

void
Android_PumpEvents(_THIS)
{

    //scanKeys();
    /* TODO: defer click-age */
    /*
    if (keysDown() & KEY_TOUCH) {
        SDL_SendMouseButton(0, SDL_PRESSED, 0);
    } else if (keysUp() & KEY_TOUCH) {
        SDL_SendMouseButton(0, SDL_RELEASED, 0);
    }
    if (keysHeld() & KEY_TOUCH) {
        touchPosition t = touchReadXY();
        SDL_SendMouseMotion(0, 0, t.px, t.py, 1);       
    }
    */
}

int
Android_OnKeyDown(int keycode){
    return SDL_SendKeyboardKey(0, SDL_PRESSED, (SDL_scancode)keycode);
}

int
Android_OnKeyUp(int keycode){
    return SDL_SendKeyboardKey(0, SDL_RELEASED, (SDL_scancode)keycode);
}

/* vi: set ts=4 sw=4 expandtab: */
