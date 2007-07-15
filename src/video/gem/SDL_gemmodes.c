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

#include "SDL_gemvideo.h"


void
GEM_InitModes(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    Uint32 Rmask, Gmask, Bmask, Amask;
    SDL_VideoDisplay display;
    SDL_DisplayData *displaydata;
    SDL_DisplayMode mode;
    int bpp;

    /* read vdi bpp, rmask, gmask, bmask, amask */

    mode.format = SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
    mode.w = 0 /* vdi width */ ;
    mode.h = 0 /* vdi height */ ;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;

    displaydata = (SDL_DisplayData *) SDL_malloc(sizeof(*displaydata));
    if (!displaydata) {
        return;
    }
    /* fill display data */

    SDL_zero(display);
    display.desktop_mode = mode;
    display.current_mode = mode;
    display.driverdata = displaydata;
    SDL_AddVideoDisplay(&display);
}

void
GEM_GetDisplayModes(_THIS)
{
    SDL_DisplayData *data = (SDL_DisplayData *) SDL_CurrentDisplay.driverdata;
    SDL_DisplayMode mode;
    //SDL_AddDisplayMode(_this->current_display, &mode);
}

int
GEM_SetDisplayMode(_THIS, SDL_DisplayMode * mode)
{
    //SDL_DisplayModeData *data = (SDL_DisplayModeData *) mode->driverdata;
    return -1;
}

void
GEM_QuitModes(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
