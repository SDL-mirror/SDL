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

#define MUL(_a, _b) (((Uint16)(_a)*(Uint16)(_b))/255)
#define SHIFTAND(_v, _s, _a) (((_v)>>(_s)) & (_a))

#define SETPIXEL_MASK(p, type, bpp, rshift, gshift, bshift, rmask, gmask, bmask) \
do { \
	if (a) { \
		p = (r<<rshift) | (g<<gshift) | (b<<bshift); \
	} \
} while (0)

#define SETPIXEL_BLEND(p, type, bpp, rshift, gshift, bshift, rmask, gmask, bmask) \
do { \
	Uint8 sr = MUL(inva, SHIFTAND(p, rshift, rmask)) + (Uint16) r; \
	Uint8 sg = MUL(inva, SHIFTAND(p, gshift, gmask)) + (Uint16) g; \
	Uint8 sb = MUL(inva, SHIFTAND(p, bshift, bmask)) + (Uint16) b; \
	p = (sr<<rshift) | (sg<<gshift) | (sb<<bshift);  \
} while (0)

#define SETPIXEL_ADD(p, type, bpp, rshift, gshift, bshift, rmask, gmask, bmask) \
do { \
	Uint16 sr = SHIFTAND(p, rshift, rmask) + (Uint16) r; \
	Uint16 sg = SHIFTAND(p, gshift, gmask) + (Uint16) g; \
	Uint16 sb = SHIFTAND(p, bshift, bmask) + (Uint16) b; \
	if (sr>rmask) sr = rmask;  \
	if (sg>gmask) sg = gmask;  \
	if (sb>bmask) sb = bmask;  \
	p = (sr<<rshift) | (sg<<gshift) | (sb<<bshift);  \
} while (0)

#define SETPIXEL_MOD(p, type, bpp, rshift, gshift, bshift, rmask, gmask, bmask) \
do { \
	Uint8 sr = MUL(SHIFTAND(p, rshift, rmask), r); \
	Uint8 sg = MUL(SHIFTAND(p, gshift, gmask), g); \
	Uint8 sb = MUL(SHIFTAND(p, bshift, bmask), b); \
	p = (sr<<rshift) | (sg<<gshift) | (sb<<bshift);  \
} while (0)


#define SETPIXEL15_MASK(p) SETPIXEL_MASK(p, Uint16, 2, 10, 5, 0, 0x1f, 0x1f, 0x1f);
#define SETPIXEL15_BLEND(p) SETPIXEL_BLEND(p, Uint16, 2, 10, 5, 0, 0x1f, 0x1f, 0x1f);
#define SETPIXEL15_ADD(p) SETPIXEL_ADD(p, Uint16, 2, 10, 5, 0, 0x1f, 0x1f, 0x1f);
#define SETPIXEL15_MOD(p) SETPIXEL_MOD(p, Uint16, 2, 10, 5, 0, 0x1f, 0x1f, 0x1f);

#define SETPIXEL16_MASK(p) SETPIXEL_MASK(p, Uint16, 2, 11, 5, 0, 0x1f, 0x3f, 0x1f);
#define SETPIXEL16_BLEND(p) SETPIXEL_BLEND(p, Uint16, 2, 11, 5, 0, 0x1f, 0x3f, 0x1f);
#define SETPIXEL16_ADD(p) SETPIXEL_ADD(p, Uint16, 2, 11, 5, 0, 0x1f, 0x3f, 0x1f);
#define SETPIXEL16_MOD(p) SETPIXEL_MOD(p, Uint16, 2, 11, 5, 0, 0x1f, 0x3f, 0x1f);

#define SETPIXEL32_MASK(p) SETPIXEL_MASK(p, Uint32, 4, 16, 8, 0, 0xff, 0xff, 0xff);
#define SETPIXEL32_BLEND(p) SETPIXEL_BLEND(p, Uint32, 4, 16, 8, 0, 0xff, 0xff, 0xff);
#define SETPIXEL32_ADD(p) SETPIXEL_ADD(p, Uint32, 4, 16, 8, 0, 0xff, 0xff, 0xff);
#define SETPIXEL32_MOD(p) SETPIXEL_MOD(p, Uint32, 4, 16, 8, 0, 0xff, 0xff, 0xff);

#define BLENDRECT(type, op) \
do { \
	int y = dstrect->y; \
    int h = dstrect->h; \
    while (h--) { \
        type *pixel = (type *)(dst->pixels + y * dst->pitch + dstrect->x * dst->format->BytesPerPixel); \
    	int w = dstrect->w; \
    	while (w--) { \
    		op(*pixel); \
    		pixel++; \
    	} \
    	y++; \
    } \
} while (0)

int
SDL_BlendRect(SDL_Surface * dst, SDL_Rect * dstrect, int blendMode, Uint8 r,
              Uint8 g, Uint8 b, Uint8 a)
{
    Uint16 inva = 0xff - a;
    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_BlendRect(): Unsupported surface format");
        return (-1);
    }

    /* If 'dstrect' == NULL, then fill the whole surface */
    if (dstrect) {
        /* Perform clipping */
        if (!SDL_IntersectRect(dstrect, &dst->clip_rect, dstrect)) {
            return (0);
        }
    } else {
        dstrect = &dst->clip_rect;
    }

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
            BLENDRECT(Uint16, SETPIXEL15_MASK);
            break;
        case SDL_BLENDMODE_BLEND:
            BLENDRECT(Uint16, SETPIXEL15_BLEND);
            break;
        case SDL_BLENDMODE_ADD:
            BLENDRECT(Uint16, SETPIXEL15_ADD);
            break;
        case SDL_BLENDMODE_MOD:
            BLENDRECT(Uint16, SETPIXEL15_MOD);
            break;
        }
        break;
    case 16:
        switch (blendMode) {
        case SDL_BLENDMODE_MASK:
            BLENDRECT(Uint16, SETPIXEL16_MASK);
            break;
        case SDL_BLENDMODE_BLEND:
            BLENDRECT(Uint16, SETPIXEL16_BLEND);
            break;
        case SDL_BLENDMODE_ADD:
            BLENDRECT(Uint16, SETPIXEL16_ADD);
            break;
        case SDL_BLENDMODE_MOD:
            BLENDRECT(Uint16, SETPIXEL16_MOD);
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
            BLENDRECT(Uint32, SETPIXEL32_MASK);
            break;
        case SDL_BLENDMODE_BLEND:
            BLENDRECT(Uint32, SETPIXEL32_BLEND);
            break;
        case SDL_BLENDMODE_ADD:
            BLENDRECT(Uint32, SETPIXEL32_ADD);
            break;
        case SDL_BLENDMODE_MOD:
            BLENDRECT(Uint32, SETPIXEL32_MOD);
            break;
        }
        break;
    default:
        SDL_Unsupported();
        return -1;
    }
    return 0;
    return -1;
}

/* vi: set ts=4 sw=4 expandtab: */
