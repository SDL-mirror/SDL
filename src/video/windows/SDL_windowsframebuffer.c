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

#include "SDL_windowsvideo.h"


int WIN_CreateWindowFramebuffer(_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    BITMAPINFO info;

    /* Free the old framebuffer surface */
    if (data->mdc) {
        DeleteDC(data->mdc);
    }
    if (data->hbm) {
        DeleteObject(data->hbm);
    }

    /* We'll use RGB format for now */
    *format = SDL_PIXELFORMAT_RGB888;
    *pitch = (((window->w * SDL_BYTESPERPIXEL(*format)) + 3) & ~3);

    /* Create a new one */
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = window->w;
    info.bmiHeader.biHeight = -window->h;	/* negative for topdown bitmap */
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = window->h * (*pitch);
    info.bmiHeader.biXPelsPerMeter = 0;
    info.bmiHeader.biYPelsPerMeter = 0;
    info.bmiHeader.biClrUsed = 0;
    info.bmiHeader.biClrImportant = 0;

    data->mdc = CreateCompatibleDC(data->hdc);
    data->hbm = CreateDIBSection(data->hdc, &info, DIB_RGB_COLORS, pixels, NULL, 0);
    if (!data->hbm) {
        WIN_SetError("Unable to create DIB");
        return -1;
    }
    SelectObject(data->mdc, data->hbm);

    return 0;
}

int WIN_UpdateWindowFramebuffer(_THIS, SDL_Window * window, int numrects, SDL_Rect * rects)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;

    BitBlt(data->hdc, 0, 0, window->w, window->h, data->mdc, 0, 0, SRCCOPY);
    return 0;
}

void WIN_DestroyWindowFramebuffer(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;

    if (data->mdc) {
        DeleteDC(data->mdc);
        data->mdc = NULL;
    }
    if (data->hbm) {
        DeleteObject(data->hbm);
        data->hbm = NULL;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
