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

#include "SDL_video.h"
#include "SDL_compat.h"
#include "SDL_sysvideo.h"
#include "SDL_blit.h"
#include "SDL_RLEaccel_c.h"
#include "SDL_pixels_c.h"
#include "SDL_leaks.h"


/* Public routines */
/*
 * Create an empty RGB surface of the appropriate depth
 */
SDL_Surface *
SDL_CreateRGBSurface(Uint32 flags,
                     int width, int height, int depth,
                     Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    SDL_Surface *surface;

    /* Allocate the surface */
    surface = (SDL_Surface *) SDL_calloc(1, sizeof(*surface));
    if (surface == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    surface->format = SDL_AllocFormat(depth, Rmask, Gmask, Bmask, Amask);
    if (!surface->format) {
        SDL_FreeSurface(surface);
        return NULL;
    }
    if (Amask) {
        surface->flags |= SDL_SRCALPHA;
    }
    surface->w = width;
    surface->h = height;
    surface->pitch = SDL_CalculatePitch(surface);
    SDL_SetClipRect(surface, NULL);

    if (surface->format->BitsPerPixel <= 8) {
        SDL_Palette *palette =
            SDL_AllocPalette((1 << surface->format->BitsPerPixel));
        if (!palette) {
            SDL_FreeSurface(surface);
            return NULL;
        }
        if (Rmask || Bmask || Gmask) {
            const SDL_PixelFormat *format = surface->format;

            /* create palette according to masks */
            int i;
            int Rm = 0, Gm = 0, Bm = 0;
            int Rw = 0, Gw = 0, Bw = 0;

            if (Rmask) {
                Rw = 8 - format->Rloss;
                for (i = format->Rloss; i > 0; i -= Rw)
                    Rm |= 1 << i;
            }
            if (Gmask) {
                Gw = 8 - format->Gloss;
                for (i = format->Gloss; i > 0; i -= Gw)
                    Gm |= 1 << i;
            }
            if (Bmask) {
                Bw = 8 - format->Bloss;
                for (i = format->Bloss; i > 0; i -= Bw)
                    Bm |= 1 << i;
            }
            for (i = 0; i < palette->ncolors; ++i) {
                int r, g, b;
                r = (i & Rmask) >> format->Rshift;
                r = (r << format->Rloss) | ((r * Rm) >> Rw);
                palette->colors[i].r = r;

                g = (i & Gmask) >> format->Gshift;
                g = (g << format->Gloss) | ((g * Gm) >> Gw);
                palette->colors[i].g = g;

                b = (i & Bmask) >> format->Bshift;
                b = (b << format->Bloss) | ((b * Bm) >> Bw);
                palette->colors[i].b = b;
            }
        } else if (palette->ncolors == 2) {
            /* Create a black and white bitmap palette */
            palette->colors[0].r = 0xFF;
            palette->colors[0].g = 0xFF;
            palette->colors[0].b = 0xFF;
            palette->colors[1].r = 0x00;
            palette->colors[1].g = 0x00;
            palette->colors[1].b = 0x00;
        }
        SDL_SetSurfacePalette(surface, palette);
        SDL_FreePalette(palette);
    }

    /* Get the pixels */
    if (surface->w && surface->h) {
        surface->pixels = SDL_malloc(surface->h * surface->pitch);
        if (!surface->pixels) {
            SDL_FreeSurface(surface);
            SDL_OutOfMemory();
            return NULL;
        }
        /* This is important for bitmaps */
        SDL_memset(surface->pixels, 0, surface->h * surface->pitch);
    }

    /* Allocate an empty mapping */
    surface->map = SDL_AllocBlitMap();
    if (!surface->map) {
        SDL_FreeSurface(surface);
        return NULL;
    }
    SDL_FormatChanged(surface);

    /* The surface is ready to go */
    surface->refcount = 1;
#ifdef CHECK_LEAKS
    ++surfaces_allocated;
#endif
    return surface;
}

/*
 * Create an RGB surface from an existing memory buffer
 */
SDL_Surface *
SDL_CreateRGBSurfaceFrom(void *pixels,
                         int width, int height, int depth, int pitch,
                         Uint32 Rmask, Uint32 Gmask, Uint32 Bmask,
                         Uint32 Amask)
{
    SDL_Surface *surface;

    surface =
        SDL_CreateRGBSurface(0, 0, 0, depth, Rmask, Gmask, Bmask, Amask);
    if (surface != NULL) {
        surface->flags |= SDL_PREALLOC;
        surface->pixels = pixels;
        surface->w = width;
        surface->h = height;
        surface->pitch = pitch;
        SDL_SetClipRect(surface, NULL);
    }
    return surface;
}

static int
SDL_SurfacePaletteChanged(void *userdata, SDL_Palette * palette)
{
    SDL_Surface *surface = (SDL_Surface *) userdata;

    SDL_FormatChanged(surface);

    return 0;
}

int
SDL_SetSurfacePalette(SDL_Surface * surface, SDL_Palette * palette)
{
    if (!surface || !surface->format) {
        SDL_SetError("SDL_SetSurfacePalette() passed a NULL surface");
        return -1;
    }

    if (palette && palette->ncolors != (1 << surface->format->BitsPerPixel)) {
        SDL_SetError
            ("SDL_SetSurfacePalette() passed a palette that doesn't match the surface format");
        return -1;
    }

    if (surface->format->palette == palette) {
        return 0;
    }

    if (surface->format->palette) {
        SDL_DelPaletteWatch(surface->format->palette,
                            SDL_SurfacePaletteChanged, surface);
    }

    surface->format->palette = palette;

    if (surface->format->palette) {
        SDL_AddPaletteWatch(surface->format->palette,
                            SDL_SurfacePaletteChanged, surface);
    }
    return 0;
}

/*
 * Set the color key in a blittable surface
 */
int
SDL_SetColorKey(SDL_Surface * surface, Uint32 flag, Uint32 key)
{
    /* Sanity check the flag as it gets passed in */
    if (flag & SDL_SRCCOLORKEY) {
        if (flag & (SDL_RLEACCEL | SDL_RLEACCELOK)) {
            flag = (SDL_SRCCOLORKEY | SDL_RLEACCELOK);
        } else {
            flag = SDL_SRCCOLORKEY;
        }
    } else {
        flag = 0;
    }

    /* Optimize away operations that don't change anything */
    if ((flag == (surface->flags & (SDL_SRCCOLORKEY | SDL_RLEACCELOK))) &&
        (key == surface->format->colorkey)) {
        return (0);
    }

    /* UnRLE surfaces before we change the colorkey */
    if (surface->flags & SDL_RLEACCEL) {
        SDL_UnRLESurface(surface, 1);
    }

    if (flag) {
        surface->flags |= SDL_SRCCOLORKEY;
        surface->format->colorkey = key;
        if (flag & SDL_RLEACCELOK) {
            surface->flags |= SDL_RLEACCELOK;
        } else {
            surface->flags &= ~SDL_RLEACCELOK;
        }
    } else {
        surface->flags &= ~(SDL_SRCCOLORKEY | SDL_RLEACCELOK);
        surface->format->colorkey = 0;
    }
    SDL_InvalidateMap(surface->map);
    return (0);
}

/* This function sets the alpha channel of a surface */
int
SDL_SetAlpha(SDL_Surface * surface, Uint32 flag, Uint8 value)
{
    Uint32 oldflags = surface->flags;
    Uint32 oldalpha = surface->format->alpha;

    /* Sanity check the flag as it gets passed in */
    if (flag & SDL_SRCALPHA) {
        if (flag & (SDL_RLEACCEL | SDL_RLEACCELOK)) {
            flag = (SDL_SRCALPHA | SDL_RLEACCELOK);
        } else {
            flag = SDL_SRCALPHA;
        }
    } else {
        flag = 0;
    }

    /* Optimize away operations that don't change anything */
    if ((flag == (surface->flags & (SDL_SRCALPHA | SDL_RLEACCELOK))) &&
        (!flag || value == oldalpha)) {
        return (0);
    }

    if (!(flag & SDL_RLEACCELOK) && (surface->flags & SDL_RLEACCEL))
        SDL_UnRLESurface(surface, 1);

    if (flag) {
        surface->flags |= SDL_SRCALPHA;
        surface->format->alpha = value;
        if (flag & SDL_RLEACCELOK) {
            surface->flags |= SDL_RLEACCELOK;
        } else {
            surface->flags &= ~SDL_RLEACCELOK;
        }
    } else {
        surface->flags &= ~SDL_SRCALPHA;
        surface->format->alpha = SDL_ALPHA_OPAQUE;
    }
    /*
     * The representation for software surfaces is independent of
     * per-surface alpha, so no need to invalidate the blit mapping
     * if just the alpha value was changed. (If either is 255, we still
     * need to invalidate.)
     */
    if (oldflags != surface->flags
        || (((oldalpha + 1) ^ (value + 1)) & 0x100)) {
        SDL_InvalidateMap(surface->map);
    }
    return (0);
}

int
SDL_SetAlphaChannel(SDL_Surface * surface, Uint8 value)
{
    int row, col;
    int offset;
    Uint8 *buf;

    if ((surface->format->Amask != 0xFF000000) &&
        (surface->format->Amask != 0x000000FF)) {
        SDL_SetError("Unsupported surface alpha mask format");
        return -1;
    }
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    if (surface->format->Amask == 0xFF000000) {
        offset = 3;
    } else {
        offset = 0;
    }
#else
    if (surface->format->Amask == 0xFF000000) {
        offset = 0;
    } else {
        offset = 3;
    }
#endif /* Byte ordering */

    /* Quickly set the alpha channel of an RGBA or ARGB surface */
    if (SDL_MUSTLOCK(surface)) {
        if (SDL_LockSurface(surface) < 0) {
            return -1;
        }
    }
    row = surface->h;
    while (row--) {
        col = surface->w;
        buf = (Uint8 *) surface->pixels + row * surface->pitch + offset;
        while (col--) {
            *buf = value;
            buf += 4;
        }
    }
    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }
    return 0;
}

/*
 * Set the clipping rectangle for a blittable surface
 */
SDL_bool
SDL_SetClipRect(SDL_Surface * surface, const SDL_Rect * rect)
{
    SDL_Rect full_rect;

    /* Don't do anything if there's no surface to act on */
    if (!surface) {
        return SDL_FALSE;
    }

    /* Set up the full surface rectangle */
    full_rect.x = 0;
    full_rect.y = 0;
    full_rect.w = surface->w;
    full_rect.h = surface->h;

    /* Set the clipping rectangle */
    if (!rect) {
        surface->clip_rect = full_rect;
        return 1;
    }
    return SDL_IntersectRect(rect, &full_rect, &surface->clip_rect);
}

void
SDL_GetClipRect(SDL_Surface * surface, SDL_Rect * rect)
{
    if (surface && rect) {
        *rect = surface->clip_rect;
    }
}

/* 
 * Set up a blit between two surfaces -- split into three parts:
 * The upper part, SDL_UpperBlit(), performs clipping and rectangle 
 * verification.  The lower part is a pointer to a low level
 * accelerated blitting function.
 *
 * These parts are separated out and each used internally by this 
 * library in the optimimum places.  They are exported so that if
 * you know exactly what you are doing, you can optimize your code
 * by calling the one(s) you need.
 */
int
SDL_LowerBlit(SDL_Surface * src, SDL_Rect * srcrect,
              SDL_Surface * dst, SDL_Rect * dstrect)
{
    /* Check to make sure the blit mapping is valid */
    if ((src->map->dst != dst) ||
        (src->map->dst->format_version != src->map->format_version)) {
        if (SDL_MapSurface(src, dst) < 0) {
            return (-1);
        }
    }
    return (src->map->sw_blit(src, srcrect, dst, dstrect));
}


int
SDL_UpperBlit(SDL_Surface * src, SDL_Rect * srcrect,
              SDL_Surface * dst, SDL_Rect * dstrect)
{
    SDL_Rect fulldst;
    int srcx, srcy, w, h;

    /* Make sure the surfaces aren't locked */
    if (!src || !dst) {
        SDL_SetError("SDL_UpperBlit: passed a NULL surface");
        return (-1);
    }
    if (src->locked || dst->locked) {
        SDL_SetError("Surfaces must not be locked during blit");
        return (-1);
    }

    /* If the destination rectangle is NULL, use the entire dest surface */
    if (dstrect == NULL) {
        fulldst.x = fulldst.y = 0;
        dstrect = &fulldst;
    }

    /* clip the source rectangle to the source surface */
    if (srcrect) {
        int maxw, maxh;

        srcx = srcrect->x;
        w = srcrect->w;
        if (srcx < 0) {
            w += srcx;
            dstrect->x -= srcx;
            srcx = 0;
        }
        maxw = src->w - srcx;
        if (maxw < w)
            w = maxw;

        srcy = srcrect->y;
        h = srcrect->h;
        if (srcy < 0) {
            h += srcy;
            dstrect->y -= srcy;
            srcy = 0;
        }
        maxh = src->h - srcy;
        if (maxh < h)
            h = maxh;

    } else {
        srcx = srcy = 0;
        w = src->w;
        h = src->h;
    }

    /* clip the destination rectangle against the clip rectangle */
    {
        SDL_Rect *clip = &dst->clip_rect;
        int dx, dy;

        dx = clip->x - dstrect->x;
        if (dx > 0) {
            w -= dx;
            dstrect->x += dx;
            srcx += dx;
        }
        dx = dstrect->x + w - clip->x - clip->w;
        if (dx > 0)
            w -= dx;

        dy = clip->y - dstrect->y;
        if (dy > 0) {
            h -= dy;
            dstrect->y += dy;
            srcy += dy;
        }
        dy = dstrect->y + h - clip->y - clip->h;
        if (dy > 0)
            h -= dy;
    }

    if (w > 0 && h > 0) {
        SDL_Rect sr;
        sr.x = srcx;
        sr.y = srcy;
        sr.w = dstrect->w = w;
        sr.h = dstrect->h = h;
        return SDL_LowerBlit(src, &sr, dst, dstrect);
    }
    dstrect->w = dstrect->h = 0;
    return 0;
}

#ifdef __SSE__
/* *INDENT-OFF* */

#define SSE_BEGIN \
    DECLARE_ALIGNED(Uint32, cccc[4], 16); \
    cccc[0] = color; \
    cccc[1] = color; \
    cccc[2] = color; \
    cccc[3] = color; \
    __m128 c128 = *(__m128 *)cccc;

#define SSE_WORK \
    for (i = n / 64; i--;) { \
        _mm_stream_ps((float *)(p+0), c128); \
        _mm_stream_ps((float *)(p+16), c128); \
        _mm_stream_ps((float *)(p+32), c128); \
        _mm_stream_ps((float *)(p+48), c128); \
        p += 64; \
    }

#define SSE_END

#define DEFINE_SSE_FILLRECT(bpp, type) \
static void \
SDL_FillRect##bpp##SSE(Uint8 *pixels, int pitch, Uint32 color, int w, int h) \
{ \
    SSE_BEGIN; \
 \
    while (h--) { \
        int i, n = w * bpp; \
        Uint8 *p = pixels; \
 \
        if (n > 15) { \
            int adjust = 16 - ((uintptr_t)p & 15); \
            if (adjust < 16) { \
                n -= adjust; \
                adjust /= bpp; \
                while(adjust--) { \
                    *((type *)p) = (type)color; \
                    p += bpp; \
                } \
            } \
            SSE_WORK; \
        } \
        if (n & 63) { \
            int remainder = (n & 63); \
            remainder /= bpp; \
            while(remainder--) { \
                *((type *)p) = (type)color; \
                p += bpp; \
            } \
        } \
        pixels += pitch; \
    } \
 \
    SSE_END; \
}

DEFINE_SSE_FILLRECT(1, Uint8)
DEFINE_SSE_FILLRECT(2, Uint16)
DEFINE_SSE_FILLRECT(4, Uint32)

/* *INDENT-ON* */
#endif /* __SSE__ */

#ifdef __MMX__
/* *INDENT-OFF* */

#define MMX_BEGIN \
    __m64 c64 = _mm_set_pi32(color, color)

#define MMX_WORK \
    for (i = n / 64; i--;) { \
        _mm_stream_pi((__m64 *)(p+0), c64); \
        _mm_stream_pi((__m64 *)(p+8), c64); \
        _mm_stream_pi((__m64 *)(p+16), c64); \
        _mm_stream_pi((__m64 *)(p+24), c64); \
        _mm_stream_pi((__m64 *)(p+32), c64); \
        _mm_stream_pi((__m64 *)(p+40), c64); \
        _mm_stream_pi((__m64 *)(p+48), c64); \
        _mm_stream_pi((__m64 *)(p+56), c64); \
        p += 64; \
    }

#define MMX_END \
    _mm_empty()

#define DEFINE_MMX_FILLRECT(bpp, type) \
static void \
SDL_FillRect##bpp##MMX(Uint8 *pixels, int pitch, Uint32 color, int w, int h) \
{ \
    MMX_BEGIN; \
 \
    while (h--) { \
        int i, n = w * bpp; \
        Uint8 *p = pixels; \
 \
        if (n > 7) { \
            int adjust = 8 - ((uintptr_t)p & 7); \
            if (adjust < 8) { \
                n -= adjust; \
                adjust /= bpp; \
                while(adjust--) { \
                    *((type *)p) = (type)color; \
                    p += bpp; \
                } \
            } \
            MMX_WORK; \
        } \
        if (n & 63) { \
            int remainder = (n & 63); \
            remainder /= bpp; \
            while(remainder--) { \
                *((type *)p) = (type)color; \
                p += bpp; \
            } \
        } \
        pixels += pitch; \
    } \
 \
    MMX_END; \
}

DEFINE_MMX_FILLRECT(1, Uint8)
DEFINE_MMX_FILLRECT(2, Uint16)
DEFINE_MMX_FILLRECT(4, Uint32)

/* *INDENT-ON* */
#endif /* __MMX__ */

static void
SDL_FillRect1(Uint8 * pixels, int pitch, Uint32 color, int w, int h)
{
    while (h--) {
        int n = w;
        Uint8 *p = pixels;

        if (n > 3) {
            switch ((uintptr_t) p & 3) {
            case 1:
                *p++ = (Uint8) color;
                --n;
            case 2:
                *p++ = (Uint8) color;
                --n;
            case 3:
                *p++ = (Uint8) color;
                --n;
            }
            SDL_memset4(p, color, (n >> 2));
        }
        if (n & 3) {
            p += (n & ~3);
            switch (n & 3) {
            case 3:
                *p++ = (Uint8) color;
            case 2:
                *p++ = (Uint8) color;
            case 1:
                *p++ = (Uint8) color;
            }
        }
        pixels += pitch;
    }
}

static void
SDL_FillRect2(Uint8 * pixels, int pitch, Uint32 color, int w, int h)
{
    while (h--) {
        int n = w;
        Uint16 *p = (Uint16 *) pixels;

        if (n > 1) {
            if ((uintptr_t) p & 2) {
                *p++ = (Uint16) color;
                --n;
            }
            SDL_memset4(p, color, (n >> 1));
        }
        if (n & 1) {
            p[n - 1] = (Uint16) color;
        }
        pixels += pitch;
    }
}

static void
SDL_FillRect3(Uint8 * pixels, int pitch, Uint32 color, int w, int h)
{
    Uint8 r = (Uint8) (color & 0xFF);
    Uint8 g = (Uint8) ((color >> 8) & 0xFF);
    Uint8 b = (Uint8) ((color >> 16) & 0xFF);

    while (h--) {
        int n = w;
        Uint8 *p = pixels;

        while (n--) {
            *p++ = r;
            *p++ = g;
            *p++ = b;
        }
        pixels += pitch;
    }
}

static void
SDL_FillRect4(Uint8 * pixels, int pitch, Uint32 color, int w, int h)
{
    while (h--) {
        SDL_memset4(pixels, color, w);
        pixels += pitch;
    }
}

/* 
 * This function performs a fast fill of the given rectangle with 'color'
 */
int
SDL_FillRect(SDL_Surface * dst, SDL_Rect * dstrect, Uint32 color)
{
    Uint8 *pixels;

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("Fill rect on unsupported surface format");
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

    /* Perform software fill */
    if (SDL_LockSurface(dst) != 0) {
        return (-1);
    }

    pixels =
        (Uint8 *) dst->pixels + dstrect->y * dst->pitch +
        dstrect->x * dst->format->BytesPerPixel;

    switch (dst->format->BytesPerPixel) {
    case 1:
        {
            color |= (color << 8);
            color |= (color << 16);
#ifdef __SSE__
            if (SDL_HasSSE()) {
                SDL_FillRect1SSE(pixels, dst->pitch, color, dstrect->w,
                                 dstrect->h);
                break;
            }
#endif
#ifdef __MMX__
            if (SDL_HasMMX()) {
                SDL_FillRect1MMX(pixels, dst->pitch, color, dstrect->w,
                                 dstrect->h);
                break;
            }
#endif
            SDL_FillRect1(pixels, dst->pitch, color, dstrect->w, dstrect->h);
            break;
        }

    case 2:
        {
            color |= (color << 16);
#ifdef __SSE__
            if (SDL_HasSSE()) {
                SDL_FillRect2SSE(pixels, dst->pitch, color, dstrect->w,
                                 dstrect->h);
                break;
            }
#endif
#ifdef __MMX__
            if (SDL_HasMMX()) {
                SDL_FillRect2MMX(pixels, dst->pitch, color, dstrect->w,
                                 dstrect->h);
                break;
            }
#endif
            SDL_FillRect2(pixels, dst->pitch, color, dstrect->w, dstrect->h);
            break;
        }

    case 3:
        /* 24-bit RGB is a slow path, at least for now. */
        {
            SDL_FillRect3(pixels, dst->pitch, color, dstrect->w, dstrect->h);
            break;
        }

    case 4:
        {
#ifdef __SSE__
            if (SDL_HasSSE()) {
                SDL_FillRect4SSE(pixels, dst->pitch, color, dstrect->w,
                                 dstrect->h);
                break;
            }
#endif
#ifdef __MMX__
            if (SDL_HasMMX()) {
                SDL_FillRect4MMX(pixels, dst->pitch, color, dstrect->w,
                                 dstrect->h);
                break;
            }
#endif
            SDL_FillRect4(pixels, dst->pitch, color, dstrect->w, dstrect->h);
            break;
        }
    }

    SDL_UnlockSurface(dst);

    /* We're done! */
    return (0);
}

/*
 * Lock a surface to directly access the pixels
 */
int
SDL_LockSurface(SDL_Surface * surface)
{
    if (!surface->locked) {
        /* Perform the lock */
        if (surface->flags & SDL_RLEACCEL) {
            SDL_UnRLESurface(surface, 1);
            surface->flags |= SDL_RLEACCEL;     /* save accel'd state */
        }
    }

    /* Increment the surface lock count, for recursive locks */
    ++surface->locked;

    /* Ready to go.. */
    return (0);
}

/*
 * Unlock a previously locked surface
 */
void
SDL_UnlockSurface(SDL_Surface * surface)
{
    /* Only perform an unlock if we are locked */
    if (!surface->locked || (--surface->locked > 0)) {
        return;
    }

    /* Update RLE encoded surface with new data */
    if ((surface->flags & SDL_RLEACCEL) == SDL_RLEACCEL) {
        surface->flags &= ~SDL_RLEACCEL;        /* stop lying */
        SDL_RLESurface(surface);
    }
}

/* 
 * Convert a surface into the specified pixel format.
 */
SDL_Surface *
SDL_ConvertSurface(SDL_Surface * surface,
                   SDL_PixelFormat * format, Uint32 flags)
{
    SDL_Surface *convert;
    Uint32 colorkey = 0;
    Uint8 alpha = 0;
    Uint32 surface_flags;
    SDL_Rect bounds;

    /* Check for empty destination palette! (results in empty image) */
    if (format->palette != NULL) {
        int i;
        for (i = 0; i < format->palette->ncolors; ++i) {
            if ((format->palette->colors[i].r != 0xFF) ||
                (format->palette->colors[i].g != 0xFF) ||
                (format->palette->colors[i].b != 0xFF))
                break;
        }
        if (i == format->palette->ncolors) {
            SDL_SetError("Empty destination palette");
            return (NULL);
        }
    }

    /* Create a new surface with the desired format */
    convert = SDL_CreateRGBSurface(flags,
                                   surface->w, surface->h,
                                   format->BitsPerPixel, format->Rmask,
                                   format->Gmask, format->Bmask,
                                   format->Amask);
    if (convert == NULL) {
        return (NULL);
    }

    /* Copy the palette if any */
    if (format->palette && convert->format->palette) {
        SDL_memcpy(convert->format->palette->colors,
                   format->palette->colors,
                   format->palette->ncolors * sizeof(SDL_Color));
        convert->format->palette->ncolors = format->palette->ncolors;
    }

    /* Save the original surface color key and alpha */
    surface_flags = surface->flags;
    if ((surface_flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY) {
        /* Convert colourkeyed surfaces to RGBA if requested */
        if ((flags & SDL_SRCCOLORKEY) != SDL_SRCCOLORKEY && format->Amask) {
            surface_flags &= ~SDL_SRCCOLORKEY;
        } else {
            colorkey = surface->format->colorkey;
            SDL_SetColorKey(surface, 0, 0);
        }
    }
    if ((surface_flags & SDL_SRCALPHA) == SDL_SRCALPHA) {
        /* Copy over the alpha channel to RGBA if requested */
        if (format->Amask) {
            surface->flags &= ~SDL_SRCALPHA;
        } else {
            alpha = surface->format->alpha;
            SDL_SetAlpha(surface, 0, 0);
        }
    }

    /* Copy over the image data */
    bounds.x = 0;
    bounds.y = 0;
    bounds.w = surface->w;
    bounds.h = surface->h;
    SDL_LowerBlit(surface, &bounds, convert, &bounds);

    /* Clean up the original surface, and update converted surface */
    if (convert != NULL) {
        SDL_SetClipRect(convert, &surface->clip_rect);
    }
    if ((surface_flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY) {
        Uint32 cflags = surface_flags & (SDL_SRCCOLORKEY | SDL_RLEACCELOK);
        if (convert != NULL) {
            Uint8 keyR, keyG, keyB;

            SDL_GetRGB(colorkey, surface->format, &keyR, &keyG, &keyB);
            SDL_SetColorKey(convert, cflags | (flags & SDL_RLEACCELOK),
                            SDL_MapRGB(convert->format, keyR, keyG, keyB));
        }
        SDL_SetColorKey(surface, cflags, colorkey);
    }
    if ((surface_flags & SDL_SRCALPHA) == SDL_SRCALPHA) {
        Uint32 aflags = surface_flags & (SDL_SRCALPHA | SDL_RLEACCELOK);
        if (convert != NULL) {
            SDL_SetAlpha(convert, aflags | (flags & SDL_RLEACCELOK), alpha);
        }
        if (format->Amask) {
            surface->flags |= SDL_SRCALPHA;
        } else {
            SDL_SetAlpha(surface, aflags, alpha);
        }
    }

    /* We're ready to go! */
    return (convert);
}

/*
 * Free a surface created by the above function.
 */
void
SDL_FreeSurface(SDL_Surface * surface)
{
    if (surface == NULL) {
        return;
    }
    if (--surface->refcount > 0) {
        return;
    }
    while (surface->locked > 0) {
        SDL_UnlockSurface(surface);
    }
    if (surface->flags & SDL_RLEACCEL) {
        SDL_UnRLESurface(surface, 0);
    }
    if (surface->format) {
        SDL_SetSurfacePalette(surface, NULL);
        SDL_FreeFormat(surface->format);
        surface->format = NULL;
    }
    if (surface->map != NULL) {
        SDL_FreeBlitMap(surface->map);
        surface->map = NULL;
    }
    if (surface->pixels && ((surface->flags & SDL_PREALLOC) != SDL_PREALLOC)) {
        SDL_free(surface->pixels);
    }
    SDL_free(surface);
#ifdef CHECK_LEAKS
    --surfaces_allocated;
#endif
}

/* vi: set ts=4 sw=4 expandtab: */
