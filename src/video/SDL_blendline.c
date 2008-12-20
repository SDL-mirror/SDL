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

#define ABS(_x) ((_x) < 0 ? -(_x) : (_x))

#define SWAP(_x, _y) do { int tmp; tmp = _x; _x = _y; _y = tmp; } while (0)

#define BRESENHAM(x0, y0, x1, y1, op) \
{ \
    int deltax, deltay, steep, error, xstep, ystep, x, y; \
 \
    deltax = ABS(x1 - x0); \
    deltay = ABS(y1 - y0); \
    steep = (deltay > deltax); \
    if (steep) { \
        SWAP(x0, y0); \
        SWAP(x1, y1); \
        SWAP(deltax, deltay); \
    } \
    error = (x1 - x0) / 2; \
    y = y0; \
    if (x0 > x1) { \
        xstep = -1; \
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
            op(x, y); \
            error -= deltay; \
            if (error < 0) { \
                y += ystep; \
                error += deltax; \
            } \
        } \
    } else { \
        for (x = x0; x != x1; x += xstep) { \
            op(y, x); \
            error -= deltay; \
            if (error < 0) { \
                y += ystep; \
                error += deltax; \
            } \
        } \
    } \
}

#define MUL(_a, _b) (((Uint16)(_a)*(Uint16)(_b))/255)
#define SHIFTAND(_v, _s, _a) (((_v)>>(_s)) & _a)

#define SETPIXEL_MASK(x, y, type, bpp, rshift, gshift, bshift, rmask, gmask, bmask) \
do { \
	type *pixel = (type *)(dst->pixels + y * dst->pitch + x * bpp); \
	if (a) { \
		*pixel = (r<<rshift) | (g<<gshift) | (b<<bshift); \
	} \
} while (0)

#define SETPIXEL_BLEND(x, y, type, bpp, rshift, gshift, bshift, rmask, gmask, bmask) \
do { \
	type *pixel = (type *)(dst->pixels + y * dst->pitch + x * bpp); \
	Uint8 sr = MUL(inva, SHIFTAND(*pixel, rshift, rmask)) + (Uint16) r; \
	Uint8 sg = MUL(inva, SHIFTAND(*pixel, gshift, gmask)) + (Uint16) g; \
	Uint8 sb = MUL(inva, SHIFTAND(*pixel, bshift, bmask)) + (Uint16) b; \
	*pixel = (sr<<rshift) | (sg<<gshift) | (sb<<bshift);  \
} while (0)

#define SETPIXEL_ADD(x, y, type, bpp, rshift, gshift, bshift, rmask, gmask, bmask) \
do { \
	type *pixel = (type *)(dst->pixels + y * dst->pitch + x * bpp); \
	Uint16 sr = SHIFTAND(*pixel, rshift, rmask) + (Uint16) r; \
	Uint16 sg = SHIFTAND(*pixel, gshift, gmask) + (Uint16) g; \
	Uint16 sb = SHIFTAND(*pixel, bshift, bmask) + (Uint16) b; \
	if (sr>rmask) sr = rmask;  \
	if (sg>gmask) sg = gmask;  \
	if (sb>bmask) sb = bmask;  \
	*pixel = (sr<<rshift) | (sg<<gshift) | (sb<<bshift);  \
} while (0)

#define SETPIXEL_MOD(x, y, type, bpp, rshift, gshift, bshift, rmask, gmask, bmask) \
do { \
	type *pixel = (type *)(dst->pixels + y * dst->pitch + x * bpp); \
	Uint8 sr = MUL(SHIFTAND(*pixel, rshift, rmask), r); \
	Uint8 sg = MUL(SHIFTAND(*pixel, gshift, gmask), g); \
	Uint8 sb = MUL(SHIFTAND(*pixel, bshift, bmask), b); \
	*pixel = (sr<<rshift) | (sg<<gshift) | (sb<<bshift);  \
} while (0)


#define SETPIXEL15_MASK(x, y) SETPIXEL_MASK(x, y, Uint16, 2, 10, 5, 0, 0x1f, 0x1f, 0x1f);
#define SETPIXEL15_BLEND(x, y) SETPIXEL_BLEND(x, y, Uint16, 2, 10, 5, 0, 0x1f, 0x1f, 0x1f);
#define SETPIXEL15_ADD(x, y) SETPIXEL_ADD(x, y, Uint16, 2, 10, 5, 0, 0x1f, 0x1f, 0x1f);
#define SETPIXEL15_MOD(x, y) SETPIXEL_MOD(x, y, Uint16, 2, 10, 5, 0, 0x1f, 0x1f, 0x1f);

#define SETPIXEL16_MASK(x, y) SETPIXEL_MASK(x, y, Uint16, 2, 11, 5, 0, 0x1f, 0x3f, 0x1f);
#define SETPIXEL16_BLEND(x, y) SETPIXEL_BLEND(x, y, Uint16, 2, 11, 5, 0, 0x1f, 0x3f, 0x1f);
#define SETPIXEL16_ADD(x, y) SETPIXEL_ADD(x, y, Uint16, 2, 11, 5, 0, 0x1f, 0x3f, 0x1f);
#define SETPIXEL16_MOD(x, y) SETPIXEL_MOD(x, y, Uint16, 2, 11, 5, 0, 0x1f, 0x3f, 0x1f);

#define SETPIXEL32_MASK(x, y) SETPIXEL_MASK(x, y, Uint32, 4, 16, 8, 0, 0xff, 0xff, 0xff);
#define SETPIXEL32_BLEND(x, y) SETPIXEL_BLEND(x, y, Uint32, 4, 16, 8, 0, 0xff, 0xff, 0xff);
#define SETPIXEL32_ADD(x, y) SETPIXEL_ADD(x, y, Uint32, 4, 16, 8, 0, 0xff, 0xff, 0xff);
#define SETPIXEL32_MOD(x, y) SETPIXEL_MOD(x, y, Uint32, 4, 16, 8, 0, 0xff, 0xff, 0xff);

int
SDL_BlendLine(SDL_Surface * dst, int x1, int y1, int x2, int y2,
              int blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    Uint8 inva = 0xff - a;
    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_BlendLine(): Unsupported surface format");
        return (-1);
    }

    /* Perform clipping */
    /* FIXME
       if (!SDL_IntersectRect(dstrect, &dst->clip_rect, dstrect)) {
       return (0);
       }
     */

    if ((blendMode == SDL_BLENDMODE_BLEND)
        || (blendMode == SDL_BLENDMODE_ADD)) {
        r = MUL(r, a);
        g = MUL(g, a);
        b = MUL(b, a);
    }
    switch (dst->format->BitsPerPixel) {
    case 15:
        switch (blendMode) {
        case SDL_BLENDMODE_MASK:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL15_MASK);
            break;
        case SDL_BLENDMODE_BLEND:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL15_BLEND);
            break;
        case SDL_BLENDMODE_ADD:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL15_ADD);
            break;
        case SDL_BLENDMODE_MOD:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL15_MOD);
            break;
        }
        break;
    case 16:
        switch (blendMode) {
        case SDL_BLENDMODE_MASK:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL16_MASK);
            break;
        case SDL_BLENDMODE_BLEND:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL16_BLEND);
            break;
        case SDL_BLENDMODE_ADD:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL16_ADD);
            break;
        case SDL_BLENDMODE_MOD:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL16_MOD);
            break;
        }
        break;
    case 24:
    case 32:
        if (dst->format->BytesPerPixel != 4) {
            SDL_Unsupported();
            return -1;
        }
        switch (blendMode) {
        case SDL_BLENDMODE_MASK:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL32_MASK);
            break;
        case SDL_BLENDMODE_BLEND:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL32_BLEND);
            break;
        case SDL_BLENDMODE_ADD:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL32_ADD);
            break;
        case SDL_BLENDMODE_MOD:
            BRESENHAM(x1, y1, x2, y2, SETPIXEL32_MOD);
            break;
        }
        break;
    default:
        SDL_Unsupported();
        return -1;
    }
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
