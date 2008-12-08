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

#include "../SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"

#include "SDL_gemvideo.h"

/* This is included after SDL_win32video.h, which includes windows.h */
#include "SDL_syswm.h"


int
GEM_CreateWindow(_THIS, SDL_Window * window)
{
    return -1;
}

int
GEM_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
    return -1;
}

void
GEM_SetWindowTitle(_THIS, SDL_Window * window)
{
}

void
GEM_SetWindowPosition(_THIS, SDL_Window * window)
{
}

void
GEM_SetWindowSize(_THIS, SDL_Window * window)
{
}

void
GEM_ShowWindow(_THIS, SDL_Window * window)
{
}

void
GEM_HideWindow(_THIS, SDL_Window * window)
{
}

void
GEM_RaiseWindow(_THIS, SDL_Window * window)
{
}

void
GEM_MaximizeWindow(_THIS, SDL_Window * window)
{
}

void
GEM_MinimizeWindow(_THIS, SDL_Window * window)
{
}

void
GEM_RestoreWindow(_THIS, SDL_Window * window)
{
}

void
GEM_SetWindowGrab(_THIS, SDL_Window * window)
{
}

void
GEM_DestroyWindow(_THIS, SDL_Window * window)
{
}

SDL_bool
GEM_GetWindowWMInfo(_THIS, SDL_Window * window, SDL_SysWMinfo * info)
{
    return SDL_FALSE;
}

/* vi: set ts=4 sw=4 expandtab: */
