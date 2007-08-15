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
#include "SDL_blit.h"

/* The MMX/SSE intrinsics don't give access to specific registers for
   the most memory parallelism, so we'll use GCC inline assembly here...
*/
#ifndef __GNUC__
#undef __MMX__
#undef __SSE__
#endif

#ifdef __MMX__
static __inline__ void
SDL_memcpyMMX(Uint8 *dst, const Uint8 *src, int len)
{
    int i;

    for (i = len / 64; i--;) {
        __asm__ __volatile__ (
        "prefetchnta (%0)\n"
        "movq (%0), %%mm0\n"
        "movq 8(%0), %%mm1\n"
        "movq 16(%0), %%mm2\n"
        "movq 24(%0), %%mm3\n"
        "movq 32(%0), %%mm4\n"
        "movq 40(%0), %%mm5\n"
        "movq 48(%0), %%mm6\n"
        "movq 56(%0), %%mm7\n"
        "movntq %%mm0, (%1)\n"
        "movntq %%mm1, 8(%1)\n"
        "movntq %%mm2, 16(%1)\n"
        "movntq %%mm3, 24(%1)\n"
        "movntq %%mm4, 32(%1)\n"
        "movntq %%mm5, 40(%1)\n"
        "movntq %%mm6, 48(%1)\n"
        "movntq %%mm7, 56(%1)\n"
        :: "r" (src), "r" (dst) : "memory");
        src += 64;
        dst += 64;
    }
    if (len & 63)
        SDL_memcpy(dst, src, len & 63);
}
#endif /* __MMX__ */

#ifdef __SSE__
static __inline__ void
SDL_memcpySSE(Uint8 *dst, const Uint8 *src, int len)
{
    int i;

    for (i = len / 64; i--;) {
        __asm__ __volatile__ (
        "prefetchnta (%0)\n"
        "movaps (%0), %%xmm0\n"
        "movaps 16(%0), %%xmm1\n"
        "movaps 32(%0), %%xmm2\n"
        "movaps 48(%0), %%xmm3\n"
        "movntps %%xmm0, (%1)\n"
        "movntps %%xmm1, 16(%1)\n"
        "movntps %%xmm2, 32(%1)\n"
        "movntps %%xmm3, 48(%1)\n"
        :: "r" (src), "r" (dst) : "memory");
        src += 64;
        dst += 64;
    }
    if (len & 63)
        SDL_memcpy(dst, src, len & 63);
}
#endif /* __SSE__ */

void
SDL_BlitCopy(SDL_BlitInfo * info)
{
    Uint8 *src, *dst;
    int w, h;
    int srcskip, dstskip;

    w = info->d_width * info->dst->BytesPerPixel;
    h = info->d_height;
    src = info->s_pixels;
    dst = info->d_pixels;
    srcskip = w + info->s_skip;
    dstskip = w + info->d_skip;

#ifdef __SSE__
    if (SDL_HasSSE() && !((uintptr_t)src & 15) && !((uintptr_t)dst & 15)) {
        while (h--) {
            SDL_memcpySSE(dst, src, w);
            src += srcskip;
            dst += dstskip;
        }
        return;
    }
#endif

#ifdef __MMX__
    if (SDL_HasMMX() && !((uintptr_t)src & 7) && !((uintptr_t)dst & 7)) {
        while (h--) {
            SDL_memcpyMMX(dst, src, w);
            src += srcskip;
            dst += dstskip;
        }
        __asm__ __volatile__("	emms\n"::);
        return;
    }
#endif

    while (h--) {
        SDL_memcpy(dst, src, w);
        src += srcskip;
        dst += dstskip;
    }
}

void
SDL_BlitCopyOverlap(SDL_BlitInfo * info)
{
    Uint8 *src, *dst;
    int w, h;
    int skip;

    w = info->d_width * info->dst->BytesPerPixel;
    h = info->d_height;
    src = info->s_pixels;
    dst = info->d_pixels;
    skip = w + info->s_skip;
    if ((dst < src) || (dst >= (src + h*skip))) {
        SDL_BlitCopy(info);
    } else {
        src += ((h - 1) * skip);
        dst += ((h - 1) * skip);
        while (h--) {
            SDL_revcpy(dst, src, w);
            src -= skip;
            dst -= skip;
        }
    }
}

/* vi: set ts=4 sw=4 expandtab: */
