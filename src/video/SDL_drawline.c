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

#include "SDL_video.h"
#include "SDL_blit.h"

#define ABS(x) (x < 0 ? -x : x)

#define SWAP(x, y) (x ^= y ^= x ^= y)

#define BRESENHAM(x0, y0, x1, y1, op, color) \
{ \
    int deltax, deltay, steep, error, xstep, ystep, x, y; \
 \
    deltax = ABS(x1 - x0); \
    deltay = ABS(y1 - y0); \
    steep = deltay > deltax; \
    error = deltax / 2; \
    if (steep) { \
        SWAP(x0, y0); \
        SWAP(x1, y1); \
    } \
    y = y0; \
    if (x0 > x1) { \
        xstep = -1; \
        deltax = -deltax; \
    } else { \
        xstep = 1; \
    } \
    if (y0 < y1) { \
        ystep = 1; \
    } else { \
        ystep = -1; \
    } \
    if (!steep) { \
        for (x = x0; x != x1; x += xstep) { \
            op(x, y, color); \
            error -= deltay; \
            if (error < 0) { \
                y = y + ystep; \
                error += deltax; \
            } \
        } \
    } else { \
        for (x = x0; x != x1; x += xstep) { \
            op(y, x, color); \
            error -= deltay; \
            if (error < 0) { \
                y = y + ystep; \
                error += deltax; \
            } \
        } \
    } \
}

#define SETPIXEL(x, y, type, bpp, color) \
    *(type *)(dst->pixels + y * dst->pitch + x * bpp) = (type) color

#define SETPIXEL1(x, y, color) SETPIXEL(x, y, Uint8, 1, color);
#define SETPIXEL2(x, y, color) SETPIXEL(x, y, Uint16, 2, color);
#define SETPIXEL4(x, y, color) SETPIXEL(x, y, Uint32, 4, color);

SDL_DrawLine(SDL_Surface * dst, int x1, int y1, int x2, int y2, Uint32 color)
{
    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_DrawLine(): Unsupported surface format");
        return (-1);
    }

    /* Perform clipping */
    /* FIXME
       if (!SDL_IntersectRect(dstrect, &dst->clip_rect, dstrect)) {
       return (0);
       }
     */

    switch (dst->format->BytesPerPixel) {
    case 1:
        BRESENHAM(x1, y1, x2, y2, SETPIXEL1, color);
        break;
    case 2:
        BRESENHAM(x1, y1, x2, y2, SETPIXEL2, color);
        break;
    case 3:
        SDL_Unsupported();
        return -1;
    case 4:
        BRESENHAM(x1, y1, x2, y2, SETPIXEL4, color);
        break;
    }
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
