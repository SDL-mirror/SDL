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
#include "SDL_sysvideo.h"
#include "SDL_blit.h"
#include "SDL_blit_copy.h"
#include "SDL_RLEaccel_c.h"
#include "SDL_pixels_c.h"

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)) && SDL_ASSEMBLY_ROUTINES
#define MMX_ASMBLIT
#endif

#if defined(MMX_ASMBLIT)
#include "SDL_cpuinfo.h"
#include "mmx.h"
#endif

/* The general purpose software blit routine */
static int
SDL_SoftBlit(SDL_Surface * src, SDL_Rect * srcrect,
             SDL_Surface * dst, SDL_Rect * dstrect)
{
    int okay;
    int src_locked;
    int dst_locked;

    /* Everything is okay at the beginning...  */
    okay = 1;

    /* Lock the destination if it's in hardware */
    dst_locked = 0;
    if (SDL_MUSTLOCK(dst)) {
        if (SDL_LockSurface(dst) < 0) {
            okay = 0;
        } else {
            dst_locked = 1;
        }
    }
    /* Lock the source if it's in hardware */
    src_locked = 0;
    if (SDL_MUSTLOCK(src)) {
        if (SDL_LockSurface(src) < 0) {
            okay = 0;
        } else {
            src_locked = 1;
        }
    }

    /* Set up source and destination buffer pointers, and BLIT! */
    if (okay && srcrect->w && srcrect->h) {
        SDL_BlitInfo info;
        SDL_loblit RunBlit;

        /* Set up the blit information */
        info.s_pixels = (Uint8 *) src->pixels +
            (Uint16) srcrect->y * src->pitch +
            (Uint16) srcrect->x * src->format->BytesPerPixel;
        info.s_width = srcrect->w;
        info.s_height = srcrect->h;
        info.s_skip = src->pitch - info.s_width * src->format->BytesPerPixel;
        info.d_pixels = (Uint8 *) dst->pixels +
            (Uint16) dstrect->y * dst->pitch +
            (Uint16) dstrect->x * dst->format->BytesPerPixel;
        info.d_width = dstrect->w;
        info.d_height = dstrect->h;
        info.d_skip = dst->pitch - info.d_width * dst->format->BytesPerPixel;
        info.aux_data = src->map->sw_data->aux_data;
        info.src = src->format;
        info.table = src->map->table;
        info.dst = dst->format;
        RunBlit = src->map->sw_data->blit;

        /* Run the actual software blit */
        RunBlit(&info);
    }

    /* We need to unlock the surfaces if they're locked */
    if (dst_locked) {
        SDL_UnlockSurface(dst);
    }
    if (src_locked) {
        SDL_UnlockSurface(src);
    }
    /* Blit is done! */
    return (okay ? 0 : -1);
}

#ifdef __MACOSX__
#include <sys/sysctl.h>

static SDL_bool
SDL_UseAltivecPrefetch()
{
    const char key[] = "hw.l3cachesize";
    u_int64_t result = 0;
    size_t typeSize = sizeof(result);

    if (sysctlbyname(key, &result, &typeSize, NULL, 0) == 0 && result > 0) {
        return SDL_TRUE;
    } else {
        return SDL_FALSE;
    }
}
#else
static SDL_bool
SDL_UseAltivecPrefetch()
{
    /* Just guess G4 */
    return SDL_TRUE;
}
#endif /* __MACOSX__ */

static SDL_loblit
SDL_ChooseBlitFunc(SDL_BlitEntry * entries, int count)
{
    int i;
    static Uint32 features = 0xffffffff;

    if (features == 0xffffffff) {
        const char *override = SDL_getenv("SDL_BLIT_FEATURES");

        features = SDL_BLIT_ANY;

        /* Allow an override for testing .. */
        if (override) {
            SDL_sscanf(override, "%u", &features);
        } else {
            if (SDL_HasMMX()) {
                features |= SDL_BLIT_MMX;
            }
            if (SDL_HasSSE()) {
                features |= SDL_BLIT_SSE;
            }
            if (SDL_HasAltiVec()) {
                if (SDL_UseAltivecPrefetch()) {
                    features |= SDL_BLIT_ALTIVEC_PREFETCH;
                } else {
                    features |= SDL_BLIT_ALTIVEC_NOPREFETCH;
                }
            }
        }
    }

    for (i = count; i > 0; --i) {
        if (features & entries[i].features) {
            return entries[i].blit;
        }
    }
    return entries[0].blit;
}

/* Figure out which of many blit routines to set up on a surface */
int
SDL_CalculateBlit(SDL_Surface * surface)
{
    int blit_index;

    /* Clean everything out to start */
    if ((surface->flags & SDL_RLEACCEL) == SDL_RLEACCEL) {
        SDL_UnRLESurface(surface, 1);
    }
    surface->map->sw_blit = NULL;

    /* Get the blit function index, based on surface mode */
    /* { 0 = nothing, 1 = colorkey, 2 = alpha, 3 = colorkey+alpha } */
    blit_index = 0;
    blit_index |= (!!(surface->flags & SDL_SRCCOLORKEY)) << 0;
    if (surface->flags & SDL_SRCALPHA
        && (surface->format->alpha != SDL_ALPHA_OPAQUE
            || surface->format->Amask)) {
        blit_index |= 2;
    }

    /* Check for special "identity" case -- copy blit */
    if (surface->map->identity && blit_index == 0) {
        /* Handle overlapping blits on the same surface */
        if (surface == surface->map->dst) {
            surface->map->sw_data->blit = SDL_BlitCopyOverlap;
        } else {
            surface->map->sw_data->blit = SDL_BlitCopy;
        }
    } else {
        if (surface->format->BitsPerPixel < 8) {
            surface->map->sw_data->blit =
                SDL_CalculateBlit0(surface, blit_index);
        } else {
            switch (surface->format->BytesPerPixel) {
            case 1:
                surface->map->sw_data->blit =
                    SDL_CalculateBlit1(surface, blit_index);
                break;
            case 2:
            case 3:
            case 4:
                surface->map->sw_data->blit =
                    SDL_CalculateBlitN(surface, blit_index);
                break;
            default:
                surface->map->sw_data->blit = NULL;
                break;
            }
        }
    }
    /* Make sure we have a blit function */
    if (surface->map->sw_data->blit == NULL) {
        SDL_InvalidateMap(surface->map);
        SDL_SetError("Blit combination not supported");
        return (-1);
    }

    /* Choose software blitting function */
    if (surface->flags & SDL_RLEACCELOK) {
        if (surface->map->identity
            && (blit_index == 1
                || (blit_index == 3 && !surface->format->Amask))) {
            if (SDL_RLESurface(surface) == 0)
                surface->map->sw_blit = SDL_RLEBlit;
        } else if (blit_index == 2 && surface->format->Amask) {
            if (SDL_RLESurface(surface) == 0)
                surface->map->sw_blit = SDL_RLEAlphaBlit;
        }
    }

    if (surface->map->sw_blit == NULL) {
        surface->map->sw_blit = SDL_SoftBlit;
    }
    return (0);
}

/* vi: set ts=4 sw=4 expandtab: */
