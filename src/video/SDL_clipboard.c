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

#include "SDL_clipboard.h"
#include "SDL_sysvideo.h"

/* FOURCC values for text and image clipboard formats */
#define TEXT_DATA  SDL_FOURCC('T', 'E', 'X', 'T')
#define IMAGE_DATA SDL_FOURCC('B', 'M', 'P', ' ')

int
SDL_SetClipboardText(const char *text)
{
    return SDL_SetClipboard(TEXT_DATA, text, SDL_strlen(text)+1);
}

char *
SDL_GetClipboardText()
{
    void *data;
    size_t length;

    if (SDL_GetClipboard(TEXT_DATA, &data, &length) == 0) {
        return SDL_static_cast(char*, data);
    } else {
        return NULL;
    }
}

SDL_bool
SDL_HasClipboardText()
{
    return SDL_HasClipboardFormat(TEXT_DATA);
}

int
SDL_SetClipboardImage(SDL_Surface *image)
{
    SDL_Unsupported();
    return -1;
}

SDL_Surface *
SDL_GetClipboardImage()
{
    SDL_Unsupported();
    return NULL;
}

SDL_bool
SDL_HasClipboardImage()
{
    return SDL_FALSE;
}

int
SDL_SetClipboard(Uint32 format, void *data, size_t length)
{
    SDL_Unsupported();
    return -1;
}

int
SDL_GetClipboard(Uint32 format, void **data, size_t *length)
{
    SDL_Unsupported();
    return -1;
}

SDL_bool
SDL_HasClipboardFormat(Uint32 format)
{
    return SDL_FALSE;
}

void
SDL_ClearClipboard(void)
{
}

/* vi: set ts=4 sw=4 expandtab: */
