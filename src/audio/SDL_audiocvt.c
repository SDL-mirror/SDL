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

/* Functions for audio drivers to perform runtime conversion of audio format */

#include "SDL_audio.h"
#include "SDL_audio_c.h"

#include "../libm/math.h"

//#define DEBUG_CONVERT

/* These are fractional multiplication routines. That is, their inputs
   are two numbers in the range [-1, 1) and the result falls in that
   same range. The output is the same size as the inputs, i.e.
   32-bit x 32-bit = 32-bit.
 */

/* We hope here that the right shift includes sign extension */
#ifdef SDL_HAS_64BIT_Type
#define SDL_FixMpy32(a, b) ((((Sint64)a * (Sint64)b) >> 31) & 0xffffffff)
#else
/* If we don't have the 64-bit type, do something more complicated. See http://www.8052.com/mul16.phtml or http://www.cs.uaf.edu/~cs301/notes/Chapter5/node5.html */
#define SDL_FixMpy32(a, b) ((((Sint64)a * (Sint64)b) >> 31) & 0xffffffff)
#endif
#define SDL_FixMpy16(a, b) ((((Sint32)a * (Sint32)b) >> 14) & 0xffff)
#define SDL_FixMpy8(a, b) ((((Sint16)a * (Sint16)b) >> 7) & 0xff)
/* This macro just makes the floating point filtering code not have to be a special case. */
#define SDL_FloatMpy(a, b) (a * b)

/* These macros take floating point numbers in the range [-1.0f, 1.0f) and
   represent them as fixed-point numbers in that same range. There's no
   checking that the floating point argument is inside the appropriate range.
 */

#define SDL_Make_1_7(a) (Sint8)(a * 128.0f)
#define SDL_Make_1_15(a) (Sint16)(a * 32768.0f)
#define SDL_Make_1_31(a) (Sint32)(a * 2147483648.0f)
#define SDL_Make_2_6(a) (Sint8)(a * 64.0f)
#define SDL_Make_2_14(a) (Sint16)(a * 16384.0f)
#define SDL_Make_2_30(a) (Sint32)(a * 1073741824.0f)

/* Effectively mix right and left channels into a single channel */
static void SDLCALL
SDL_ConvertMono(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;
    Sint32 sample;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting to mono\n");
#endif
    switch (format & (SDL_AUDIO_MASK_SIGNED | SDL_AUDIO_MASK_BITSIZE)) {
    case AUDIO_U8:
        {
            Uint8 *src, *dst;

            src = cvt->buf;
            dst = cvt->buf;
            for (i = cvt->len_cvt / 2; i; --i) {
                sample = src[0] + src[1];
                *dst = (Uint8) (sample / 2);
                src += 2;
                dst += 1;
            }
        }
        break;

    case AUDIO_S8:
        {
            Sint8 *src, *dst;

            src = (Sint8 *) cvt->buf;
            dst = (Sint8 *) cvt->buf;
            for (i = cvt->len_cvt / 2; i; --i) {
                sample = src[0] + src[1];
                *dst = (Sint8) (sample / 2);
                src += 2;
                dst += 1;
            }
        }
        break;

    case AUDIO_U16:
        {
            Uint8 *src, *dst;

            src = cvt->buf;
            dst = cvt->buf;
            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    sample = (Uint16) ((src[0] << 8) | src[1]) +
                        (Uint16) ((src[2] << 8) | src[3]);
                    sample /= 2;
                    dst[1] = (sample & 0xFF);
                    sample >>= 8;
                    dst[0] = (sample & 0xFF);
                    src += 4;
                    dst += 2;
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    sample = (Uint16) ((src[1] << 8) | src[0]) +
                        (Uint16) ((src[3] << 8) | src[2]);
                    sample /= 2;
                    dst[0] = (sample & 0xFF);
                    sample >>= 8;
                    dst[1] = (sample & 0xFF);
                    src += 4;
                    dst += 2;
                }
            }
        }
        break;

    case AUDIO_S16:
        {
            Uint8 *src, *dst;

            src = cvt->buf;
            dst = cvt->buf;
            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    sample = (Sint16) ((src[0] << 8) | src[1]) +
                        (Sint16) ((src[2] << 8) | src[3]);
                    sample /= 2;
                    dst[1] = (sample & 0xFF);
                    sample >>= 8;
                    dst[0] = (sample & 0xFF);
                    src += 4;
                    dst += 2;
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    sample = (Sint16) ((src[1] << 8) | src[0]) +
                        (Sint16) ((src[3] << 8) | src[2]);
                    sample /= 2;
                    dst[0] = (sample & 0xFF);
                    sample >>= 8;
                    dst[1] = (sample & 0xFF);
                    src += 4;
                    dst += 2;
                }
            }
        }
        break;

    case AUDIO_S32:
        {
            const Uint32 *src = (const Uint32 *) cvt->buf;
            Uint32 *dst = (Uint32 *) cvt->buf;
            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i, src += 2) {
                    const Sint64 added =
                        (((Sint64) (Sint32) SDL_SwapBE32(src[0])) +
                         ((Sint64) (Sint32) SDL_SwapBE32(src[1])));
                    *(dst++) = SDL_SwapBE32((Uint32) ((Sint32) (added / 2)));
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i, src += 2) {
                    const Sint64 added =
                        (((Sint64) (Sint32) SDL_SwapLE32(src[0])) +
                         ((Sint64) (Sint32) SDL_SwapLE32(src[1])));
                    *(dst++) = SDL_SwapLE32((Uint32) ((Sint32) (added / 2)));
                }
            }
        }
        break;

    case AUDIO_F32:
        {
            const float *src = (const float *) cvt->buf;
            float *dst = (float *) cvt->buf;
            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i, src += 2) {
                    const float src1 = SDL_SwapFloatBE(src[0]);
                    const float src2 = SDL_SwapFloatBE(src[1]);
                    const double added = ((double) src1) + ((double) src2);
                    const float halved = (float) (added * 0.5);
                    *(dst++) = SDL_SwapFloatBE(halved);
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i, src += 2) {
                    const float src1 = SDL_SwapFloatLE(src[0]);
                    const float src2 = SDL_SwapFloatLE(src[1]);
                    const double added = ((double) src1) + ((double) src2);
                    const float halved = (float) (added * 0.5);
                    *(dst++) = SDL_SwapFloatLE(halved);
                }
            }
        }
        break;
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Discard top 4 channels */
static void SDLCALL
SDL_ConvertStrip(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting down from 6 channels to stereo\n");
#endif

#define strip_chans_6_to_2(type) \
    { \
        const type *src = (const type *) cvt->buf; \
        type *dst = (type *) cvt->buf; \
        for (i = cvt->len_cvt / (sizeof (type) * 6); i; --i) { \
            dst[0] = src[0]; \
            dst[1] = src[1]; \
            src += 6; \
            dst += 2; \
        } \
    }

    /* this function only cares about typesize, and data as a block of bits. */
    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        strip_chans_6_to_2(Uint8);
        break;
    case 16:
        strip_chans_6_to_2(Uint16);
        break;
    case 32:
        strip_chans_6_to_2(Uint32);
        break;
    }

#undef strip_chans_6_to_2

    cvt->len_cvt /= 3;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Discard top 2 channels of 6 */
static void SDLCALL
SDL_ConvertStrip_2(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting 6 down to quad\n");
#endif

#define strip_chans_6_to_4(type) \
    { \
        const type *src = (const type *) cvt->buf; \
        type *dst = (type *) cvt->buf; \
        for (i = cvt->len_cvt / (sizeof (type) * 6); i; --i) { \
            dst[0] = src[0]; \
            dst[1] = src[1]; \
            dst[2] = src[2]; \
            dst[3] = src[3]; \
            src += 6; \
            dst += 4; \
        } \
    }

    /* this function only cares about typesize, and data as a block of bits. */
    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        strip_chans_6_to_4(Uint8);
        break;
    case 16:
        strip_chans_6_to_4(Uint16);
        break;
    case 32:
        strip_chans_6_to_4(Uint32);
        break;
    }

#undef strip_chans_6_to_4

    cvt->len_cvt /= 6;
    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

/* Duplicate a mono channel to both stereo channels */
static void SDLCALL
SDL_ConvertStereo(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting to stereo\n");
#endif

#define dup_chans_1_to_2(type) \
    { \
        const type *src = (const type *) (cvt->buf + cvt->len_cvt); \
        type *dst = (type *) (cvt->buf + cvt->len_cvt * 2); \
        for (i = cvt->len_cvt / 2; i; --i, --src) { \
            const type val = *src; \
            dst -= 2; \
            dst[0] = dst[1] = val; \
        } \
    }

    /* this function only cares about typesize, and data as a block of bits. */
    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        dup_chans_1_to_2(Uint8);
        break;
    case 16:
        dup_chans_1_to_2(Uint16);
        break;
    case 32:
        dup_chans_1_to_2(Uint32);
        break;
    }

#undef dup_chans_1_to_2

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Duplicate a stereo channel to a pseudo-5.1 stream */
static void SDLCALL
SDL_ConvertSurround(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting stereo to surround\n");
#endif

    switch (format & (SDL_AUDIO_MASK_SIGNED | SDL_AUDIO_MASK_BITSIZE)) {
    case AUDIO_U8:
        {
            Uint8 *src, *dst, lf, rf, ce;

            src = (Uint8 *) (cvt->buf + cvt->len_cvt);
            dst = (Uint8 *) (cvt->buf + cvt->len_cvt * 3);
            for (i = cvt->len_cvt; i; --i) {
                dst -= 6;
                src -= 2;
                lf = src[0];
                rf = src[1];
                ce = (lf / 2) + (rf / 2);
                dst[0] = lf;
                dst[1] = rf;
                dst[2] = lf - ce;
                dst[3] = rf - ce;
                dst[4] = ce;
                dst[5] = ce;
            }
        }
        break;

    case AUDIO_S8:
        {
            Sint8 *src, *dst, lf, rf, ce;

            src = (Sint8 *) cvt->buf + cvt->len_cvt;
            dst = (Sint8 *) cvt->buf + cvt->len_cvt * 3;
            for (i = cvt->len_cvt; i; --i) {
                dst -= 6;
                src -= 2;
                lf = src[0];
                rf = src[1];
                ce = (lf / 2) + (rf / 2);
                dst[0] = lf;
                dst[1] = rf;
                dst[2] = lf - ce;
                dst[3] = rf - ce;
                dst[4] = ce;
                dst[5] = ce;
            }
        }
        break;

    case AUDIO_U16:
        {
            Uint8 *src, *dst;
            Uint16 lf, rf, ce, lr, rr;

            src = cvt->buf + cvt->len_cvt;
            dst = cvt->buf + cvt->len_cvt * 3;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 12;
                    src -= 4;
                    lf = (Uint16) ((src[0] << 8) | src[1]);
                    rf = (Uint16) ((src[2] << 8) | src[3]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[1] = (lf & 0xFF);
                    dst[0] = ((lf >> 8) & 0xFF);
                    dst[3] = (rf & 0xFF);
                    dst[2] = ((rf >> 8) & 0xFF);

                    dst[1 + 4] = (lr & 0xFF);
                    dst[0 + 4] = ((lr >> 8) & 0xFF);
                    dst[3 + 4] = (rr & 0xFF);
                    dst[2 + 4] = ((rr >> 8) & 0xFF);

                    dst[1 + 8] = (ce & 0xFF);
                    dst[0 + 8] = ((ce >> 8) & 0xFF);
                    dst[3 + 8] = (ce & 0xFF);
                    dst[2 + 8] = ((ce >> 8) & 0xFF);
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 12;
                    src -= 4;
                    lf = (Uint16) ((src[1] << 8) | src[0]);
                    rf = (Uint16) ((src[3] << 8) | src[2]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[0] = (lf & 0xFF);
                    dst[1] = ((lf >> 8) & 0xFF);
                    dst[2] = (rf & 0xFF);
                    dst[3] = ((rf >> 8) & 0xFF);

                    dst[0 + 4] = (lr & 0xFF);
                    dst[1 + 4] = ((lr >> 8) & 0xFF);
                    dst[2 + 4] = (rr & 0xFF);
                    dst[3 + 4] = ((rr >> 8) & 0xFF);

                    dst[0 + 8] = (ce & 0xFF);
                    dst[1 + 8] = ((ce >> 8) & 0xFF);
                    dst[2 + 8] = (ce & 0xFF);
                    dst[3 + 8] = ((ce >> 8) & 0xFF);
                }
            }
        }
        break;

    case AUDIO_S16:
        {
            Uint8 *src, *dst;
            Sint16 lf, rf, ce, lr, rr;

            src = cvt->buf + cvt->len_cvt;
            dst = cvt->buf + cvt->len_cvt * 3;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 12;
                    src -= 4;
                    lf = (Sint16) ((src[0] << 8) | src[1]);
                    rf = (Sint16) ((src[2] << 8) | src[3]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[1] = (lf & 0xFF);
                    dst[0] = ((lf >> 8) & 0xFF);
                    dst[3] = (rf & 0xFF);
                    dst[2] = ((rf >> 8) & 0xFF);

                    dst[1 + 4] = (lr & 0xFF);
                    dst[0 + 4] = ((lr >> 8) & 0xFF);
                    dst[3 + 4] = (rr & 0xFF);
                    dst[2 + 4] = ((rr >> 8) & 0xFF);

                    dst[1 + 8] = (ce & 0xFF);
                    dst[0 + 8] = ((ce >> 8) & 0xFF);
                    dst[3 + 8] = (ce & 0xFF);
                    dst[2 + 8] = ((ce >> 8) & 0xFF);
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 12;
                    src -= 4;
                    lf = (Sint16) ((src[1] << 8) | src[0]);
                    rf = (Sint16) ((src[3] << 8) | src[2]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[0] = (lf & 0xFF);
                    dst[1] = ((lf >> 8) & 0xFF);
                    dst[2] = (rf & 0xFF);
                    dst[3] = ((rf >> 8) & 0xFF);

                    dst[0 + 4] = (lr & 0xFF);
                    dst[1 + 4] = ((lr >> 8) & 0xFF);
                    dst[2 + 4] = (rr & 0xFF);
                    dst[3 + 4] = ((rr >> 8) & 0xFF);

                    dst[0 + 8] = (ce & 0xFF);
                    dst[1 + 8] = ((ce >> 8) & 0xFF);
                    dst[2 + 8] = (ce & 0xFF);
                    dst[3 + 8] = ((ce >> 8) & 0xFF);
                }
            }
        }
        break;

    case AUDIO_S32:
        {
            Sint32 lf, rf, ce;
            const Uint32 *src = (const Uint32 *) cvt->buf + cvt->len_cvt;
            Uint32 *dst = (Uint32 *) cvt->buf + cvt->len_cvt * 3;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 6;
                    src -= 2;
                    lf = (Sint32) SDL_SwapBE32(src[0]);
                    rf = (Sint32) SDL_SwapBE32(src[1]);
                    ce = (lf / 2) + (rf / 2);
                    dst[0] = SDL_SwapBE32((Uint32) lf);
                    dst[1] = SDL_SwapBE32((Uint32) rf);
                    dst[2] = SDL_SwapBE32((Uint32) (lf - ce));
                    dst[3] = SDL_SwapBE32((Uint32) (rf - ce));
                    dst[4] = SDL_SwapBE32((Uint32) ce);
                    dst[5] = SDL_SwapBE32((Uint32) ce);
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 6;
                    src -= 2;
                    lf = (Sint32) SDL_SwapLE32(src[0]);
                    rf = (Sint32) SDL_SwapLE32(src[1]);
                    ce = (lf / 2) + (rf / 2);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapLE32((Uint32) (lf - ce));
                    dst[3] = SDL_SwapLE32((Uint32) (rf - ce));
                    dst[4] = SDL_SwapLE32((Uint32) ce);
                    dst[5] = SDL_SwapLE32((Uint32) ce);
                }
            }
        }
        break;

    case AUDIO_F32:
        {
            float lf, rf, ce;
            const float *src = (const float *) cvt->buf + cvt->len_cvt;
            float *dst = (float *) cvt->buf + cvt->len_cvt * 3;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 6;
                    src -= 2;
                    lf = SDL_SwapFloatBE(src[0]);
                    rf = SDL_SwapFloatBE(src[1]);
                    ce = (lf * 0.5f) + (rf * 0.5f);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapFloatBE(lf - ce);
                    dst[3] = SDL_SwapFloatBE(rf - ce);
                    dst[4] = dst[5] = SDL_SwapFloatBE(ce);
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 6;
                    src -= 2;
                    lf = SDL_SwapFloatLE(src[0]);
                    rf = SDL_SwapFloatLE(src[1]);
                    ce = (lf * 0.5f) + (rf * 0.5f);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapFloatLE(lf - ce);
                    dst[3] = SDL_SwapFloatLE(rf - ce);
                    dst[4] = dst[5] = SDL_SwapFloatLE(ce);
                }
            }
        }
        break;

    }
    cvt->len_cvt *= 3;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Duplicate a stereo channel to a pseudo-4.0 stream */
static void SDLCALL
SDL_ConvertSurround_4(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting stereo to quad\n");
#endif

    switch (format & (SDL_AUDIO_MASK_SIGNED | SDL_AUDIO_MASK_BITSIZE)) {
    case AUDIO_U8:
        {
            Uint8 *src, *dst, lf, rf, ce;

            src = (Uint8 *) (cvt->buf + cvt->len_cvt);
            dst = (Uint8 *) (cvt->buf + cvt->len_cvt * 2);
            for (i = cvt->len_cvt; i; --i) {
                dst -= 4;
                src -= 2;
                lf = src[0];
                rf = src[1];
                ce = (lf / 2) + (rf / 2);
                dst[0] = lf;
                dst[1] = rf;
                dst[2] = lf - ce;
                dst[3] = rf - ce;
            }
        }
        break;

    case AUDIO_S8:
        {
            Sint8 *src, *dst, lf, rf, ce;

            src = (Sint8 *) cvt->buf + cvt->len_cvt;
            dst = (Sint8 *) cvt->buf + cvt->len_cvt * 2;
            for (i = cvt->len_cvt; i; --i) {
                dst -= 4;
                src -= 2;
                lf = src[0];
                rf = src[1];
                ce = (lf / 2) + (rf / 2);
                dst[0] = lf;
                dst[1] = rf;
                dst[2] = lf - ce;
                dst[3] = rf - ce;
            }
        }
        break;

    case AUDIO_U16:
        {
            Uint8 *src, *dst;
            Uint16 lf, rf, ce, lr, rr;

            src = cvt->buf + cvt->len_cvt;
            dst = cvt->buf + cvt->len_cvt * 2;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 8;
                    src -= 4;
                    lf = (Uint16) ((src[0] << 8) | src[1]);
                    rf = (Uint16) ((src[2] << 8) | src[3]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[1] = (lf & 0xFF);
                    dst[0] = ((lf >> 8) & 0xFF);
                    dst[3] = (rf & 0xFF);
                    dst[2] = ((rf >> 8) & 0xFF);

                    dst[1 + 4] = (lr & 0xFF);
                    dst[0 + 4] = ((lr >> 8) & 0xFF);
                    dst[3 + 4] = (rr & 0xFF);
                    dst[2 + 4] = ((rr >> 8) & 0xFF);
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 8;
                    src -= 4;
                    lf = (Uint16) ((src[1] << 8) | src[0]);
                    rf = (Uint16) ((src[3] << 8) | src[2]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[0] = (lf & 0xFF);
                    dst[1] = ((lf >> 8) & 0xFF);
                    dst[2] = (rf & 0xFF);
                    dst[3] = ((rf >> 8) & 0xFF);

                    dst[0 + 4] = (lr & 0xFF);
                    dst[1 + 4] = ((lr >> 8) & 0xFF);
                    dst[2 + 4] = (rr & 0xFF);
                    dst[3 + 4] = ((rr >> 8) & 0xFF);
                }
            }
        }
        break;

    case AUDIO_S16:
        {
            Uint8 *src, *dst;
            Sint16 lf, rf, ce, lr, rr;

            src = cvt->buf + cvt->len_cvt;
            dst = cvt->buf + cvt->len_cvt * 2;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 8;
                    src -= 4;
                    lf = (Sint16) ((src[0] << 8) | src[1]);
                    rf = (Sint16) ((src[2] << 8) | src[3]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[1] = (lf & 0xFF);
                    dst[0] = ((lf >> 8) & 0xFF);
                    dst[3] = (rf & 0xFF);
                    dst[2] = ((rf >> 8) & 0xFF);

                    dst[1 + 4] = (lr & 0xFF);
                    dst[0 + 4] = ((lr >> 8) & 0xFF);
                    dst[3 + 4] = (rr & 0xFF);
                    dst[2 + 4] = ((rr >> 8) & 0xFF);
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 8;
                    src -= 4;
                    lf = (Sint16) ((src[1] << 8) | src[0]);
                    rf = (Sint16) ((src[3] << 8) | src[2]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[0] = (lf & 0xFF);
                    dst[1] = ((lf >> 8) & 0xFF);
                    dst[2] = (rf & 0xFF);
                    dst[3] = ((rf >> 8) & 0xFF);

                    dst[0 + 4] = (lr & 0xFF);
                    dst[1 + 4] = ((lr >> 8) & 0xFF);
                    dst[2 + 4] = (rr & 0xFF);
                    dst[3 + 4] = ((rr >> 8) & 0xFF);
                }
            }
        }
        break;

    case AUDIO_S32:
        {
            const Uint32 *src = (const Uint32 *) (cvt->buf + cvt->len_cvt);
            Uint32 *dst = (Uint32 *) (cvt->buf + cvt->len_cvt * 2);
            Sint32 lf, rf, ce;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 4;
                    src -= 2;
                    lf = (Sint32) SDL_SwapBE32(src[0]);
                    rf = (Sint32) SDL_SwapBE32(src[1]);
                    ce = (lf / 2) + (rf / 2);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapBE32((Uint32) (lf - ce));
                    dst[3] = SDL_SwapBE32((Uint32) (rf - ce));
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 4;
                    src -= 2;
                    lf = (Sint32) SDL_SwapLE32(src[0]);
                    rf = (Sint32) SDL_SwapLE32(src[1]);
                    ce = (lf / 2) + (rf / 2);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapLE32((Uint32) (lf - ce));
                    dst[3] = SDL_SwapLE32((Uint32) (rf - ce));
                }
            }
        }
        break;
    }
    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

/* Convert rate up by multiple of 2 */
static void SDLCALL
SDL_RateMUL2(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate * 2 (mono)\n");
#endif

#define mul2_mono(type) { \
        const type *src = (const type *) (cvt->buf + cvt->len_cvt); \
        type *dst = (type *) (cvt->buf + (cvt->len_cvt * 2)); \
        for (i = cvt->len_cvt / sizeof (type); i; --i) { \
            src--; \
            dst[-1] = dst[-2] = src[0]; \
            dst -= 2; \
        } \
    }

    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        mul2_mono(Uint8);
        break;
    case 16:
        mul2_mono(Uint16);
        break;
    case 32:
        mul2_mono(Uint32);
        break;
    }

#undef mul2_mono

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Convert rate up by multiple of 2, for stereo */
static void SDLCALL
SDL_RateMUL2_c2(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate * 2 (stereo)\n");
#endif

#define mul2_stereo(type) { \
        const type *src = (const type *) (cvt->buf + cvt->len_cvt); \
        type *dst = (type *) (cvt->buf + (cvt->len_cvt * 2)); \
        for (i = cvt->len_cvt / (sizeof (type) * 2); i; --i) { \
            const type r = src[-1]; \
            const type l = src[-2]; \
            src -= 2; \
            dst[-1] = r; \
            dst[-2] = l; \
            dst[-3] = r; \
            dst[-4] = l; \
            dst -= 4; \
        } \
    }

    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        mul2_stereo(Uint8);
        break;
    case 16:
        mul2_stereo(Uint16);
        break;
    case 32:
        mul2_stereo(Uint32);
        break;
    }

#undef mul2_stereo

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

/* Convert rate up by multiple of 2, for quad */
static void SDLCALL
SDL_RateMUL2_c4(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate * 2 (quad)\n");
#endif

#define mul2_quad(type) { \
        const type *src = (const type *) (cvt->buf + cvt->len_cvt); \
        type *dst = (type *) (cvt->buf + (cvt->len_cvt * 2)); \
        for (i = cvt->len_cvt / (sizeof (type) * 4); i; --i) { \
            const type c1 = src[-1]; \
            const type c2 = src[-2]; \
            const type c3 = src[-3]; \
            const type c4 = src[-4]; \
            src -= 4; \
            dst[-1] = c1; \
            dst[-2] = c2; \
            dst[-3] = c3; \
            dst[-4] = c4; \
            dst[-5] = c1; \
            dst[-6] = c2; \
            dst[-7] = c3; \
            dst[-8] = c4; \
            dst -= 8; \
        } \
    }

    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        mul2_quad(Uint8);
        break;
    case 16:
        mul2_quad(Uint16);
        break;
    case 32:
        mul2_quad(Uint32);
        break;
    }

#undef mul2_quad

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Convert rate up by multiple of 2, for 5.1 */
static void SDLCALL
SDL_RateMUL2_c6(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate * 2 (six channels)\n");
#endif

#define mul2_chansix(type) { \
        const type *src = (const type *) (cvt->buf + cvt->len_cvt); \
        type *dst = (type *) (cvt->buf + (cvt->len_cvt * 2)); \
        for (i = cvt->len_cvt / (sizeof (type) * 6); i; --i) { \
            const type c1 = src[-1]; \
            const type c2 = src[-2]; \
            const type c3 = src[-3]; \
            const type c4 = src[-4]; \
            const type c5 = src[-5]; \
            const type c6 = src[-6]; \
            src -= 6; \
            dst[-1] = c1; \
            dst[-2] = c2; \
            dst[-3] = c3; \
            dst[-4] = c4; \
            dst[-5] = c5; \
            dst[-6] = c6; \
            dst[-7] = c1; \
            dst[-8] = c2; \
            dst[-9] = c3; \
            dst[-10] = c4; \
            dst[-11] = c5; \
            dst[-12] = c6; \
            dst -= 12; \
        } \
    }

    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        mul2_chansix(Uint8);
        break;
    case 16:
        mul2_chansix(Uint16);
        break;
    case 32:
        mul2_chansix(Uint32);
        break;
    }

#undef mul2_chansix

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

/* Convert rate down by multiple of 2 */
static void SDLCALL
SDL_RateDIV2(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate / 2 (mono)\n");
#endif

#define div2_mono(type) { \
        const type *src = (const type *) cvt->buf; \
        type *dst = (type *) cvt->buf; \
        for (i = cvt->len_cvt / (sizeof (type) * 2); i; --i) { \
            dst[0] = src[0]; \
            src += 2; \
            dst++; \
        } \
    }

    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        div2_mono(Uint8);
        break;
    case 16:
        div2_mono(Uint16);
        break;
    case 32:
        div2_mono(Uint32);
        break;
    }

#undef div2_mono

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Convert rate down by multiple of 2, for stereo */
static void SDLCALL
SDL_RateDIV2_c2(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate / 2 (stereo)\n");
#endif

#define div2_stereo(type) { \
        const type *src = (const type *) cvt->buf; \
        type *dst = (type *) cvt->buf; \
        for (i = cvt->len_cvt / (sizeof (type) * 4); i; --i) { \
            dst[0] = src[0]; \
            dst[1] = src[1]; \
            src += 4; \
            dst += 2; \
        } \
    }

    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        div2_stereo(Uint8);
        break;
    case 16:
        div2_stereo(Uint16);
        break;
    case 32:
        div2_stereo(Uint32);
        break;
    }

#undef div2_stereo

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Convert rate down by multiple of 2, for quad */
static void SDLCALL
SDL_RateDIV2_c4(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate / 2 (quad)\n");
#endif

#define div2_quad(type) { \
        const type *src = (const type *) cvt->buf; \
        type *dst = (type *) cvt->buf; \
        for (i = cvt->len_cvt / (sizeof (type) * 8); i; --i) { \
            dst[0] = src[0]; \
            dst[1] = src[1]; \
            dst[2] = src[2]; \
            dst[3] = src[3]; \
            src += 8; \
            dst += 4; \
        } \
    }

    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        div2_quad(Uint8);
        break;
    case 16:
        div2_quad(Uint16);
        break;
    case 32:
        div2_quad(Uint32);
        break;
    }

#undef div2_quad

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

/* Convert rate down by multiple of 2, for 5.1 */
static void SDLCALL
SDL_RateDIV2_c6(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate / 2 (six channels)\n");
#endif

#define div2_chansix(type) { \
        const type *src = (const type *) cvt->buf; \
        type *dst = (type *) cvt->buf; \
        for (i = cvt->len_cvt / (sizeof (type) * 12); i; --i) { \
            dst[0] = src[0]; \
            dst[1] = src[1]; \
            dst[2] = src[2]; \
            dst[3] = src[3]; \
            dst[4] = src[4]; \
            dst[5] = src[5]; \
            src += 12; \
            dst += 6; \
        } \
    }

    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        div2_chansix(Uint8);
        break;
    case 16:
        div2_chansix(Uint16);
        break;
    case 32:
        div2_chansix(Uint32);
        break;
    }

#undef div_chansix

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

/* Very slow rate conversion routine */
static void SDLCALL
SDL_RateSLOW(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    double ipos;
    int i, clen;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting audio rate * %4.4f\n", 1.0 / cvt->rate_incr);
#endif
    clen = (int) ((double) cvt->len_cvt / cvt->rate_incr);
    if (cvt->rate_incr > 1.0) {
        switch (SDL_AUDIO_BITSIZE(format)) {
        case 8:
            {
                Uint8 *output;

                output = cvt->buf;
                ipos = 0.0;
                for (i = clen; i; --i) {
                    *output = cvt->buf[(int) ipos];
                    ipos += cvt->rate_incr;
                    output += 1;
                }
            }
            break;

        case 16:
            {
                Uint16 *output;

                clen &= ~1;
                output = (Uint16 *) cvt->buf;
                ipos = 0.0;
                for (i = clen / 2; i; --i) {
                    *output = ((Uint16 *) cvt->buf)[(int) ipos];
                    ipos += cvt->rate_incr;
                    output += 1;
                }
            }
            break;

        case 32:
            {
                /* !!! FIXME: need 32-bit converter here! */
#ifdef DEBUG_CONVERT
                fprintf(stderr, "FIXME: need 32-bit converter here!\n");
#endif
            }
        }
    } else {
        switch (SDL_AUDIO_BITSIZE(format)) {
        case 8:
            {
                Uint8 *output;

                output = cvt->buf + clen;
                ipos = (double) cvt->len_cvt;
                for (i = clen; i; --i) {
                    ipos -= cvt->rate_incr;
                    output -= 1;
                    *output = cvt->buf[(int) ipos];
                }
            }
            break;

        case 16:
            {
                Uint16 *output;

                clen &= ~1;
                output = (Uint16 *) (cvt->buf + clen);
                ipos = (double) cvt->len_cvt / 2;
                for (i = clen / 2; i; --i) {
                    ipos -= cvt->rate_incr;
                    output -= 1;
                    *output = ((Uint16 *) cvt->buf)[(int) ipos];
                }
            }
            break;

        case 32:
            {
                /* !!! FIXME: need 32-bit converter here! */
#ifdef DEBUG_CONVERT
                fprintf(stderr, "FIXME: need 32-bit converter here!\n");
#endif
            }
        }
    }

    cvt->len_cvt = clen;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

int
SDL_ConvertAudio(SDL_AudioCVT * cvt)
{
    /* Make sure there's data to convert */
    if (cvt->buf == NULL) {
        SDL_SetError("No buffer allocated for conversion");
        return (-1);
    }
    /* Return okay if no conversion is necessary */
    cvt->len_cvt = cvt->len;
    if (cvt->filters[0] == NULL) {
        return (0);
    }

    /* Set up the conversion and go! */
    cvt->filter_index = 0;
    cvt->filters[0] (cvt, cvt->src_format);
    return (0);
}


static SDL_AudioFilter
SDL_HandTunedTypeCVT(SDL_AudioFormat src_fmt, SDL_AudioFormat dst_fmt)
{
    /*
     * Fill in any future conversions that are specialized to a
     *  processor, platform, compiler, or library here.
     */

    return NULL;                /* no specialized converter code available. */
}


/*
 * Find a converter between two data types. We try to select a hand-tuned
 *  asm/vectorized/optimized function first, and then fallback to an
 *  autogenerated function that is customized to convert between two
 *  specific data types.
 */
static int
SDL_BuildAudioTypeCVT(SDL_AudioCVT * cvt,
                      SDL_AudioFormat src_fmt, SDL_AudioFormat dst_fmt)
{
    if (src_fmt != dst_fmt) {
        const Uint16 src_bitsize = SDL_AUDIO_BITSIZE(src_fmt);
        const Uint16 dst_bitsize = SDL_AUDIO_BITSIZE(dst_fmt);
        SDL_AudioFilter filter = SDL_HandTunedTypeCVT(src_fmt, dst_fmt);

        /* No hand-tuned converter? Try the autogenerated ones. */
        if (filter == NULL) {
            int i;
            for (i = 0; sdl_audio_type_filters[i].filter != NULL; i++) {
                const SDL_AudioTypeFilters *filt = &sdl_audio_type_filters[i];
                if ((filt->src_fmt == src_fmt) && (filt->dst_fmt == dst_fmt)) {
                    filter = filt->filter;
                    break;
                }
            }

            if (filter == NULL) {
                return -1;      /* Still no matching converter?! */
            }
        }

        /* Update (cvt) with filter details... */
        cvt->filters[cvt->filter_index++] = filter;
        if (src_bitsize < dst_bitsize) {
            const int mult = (dst_bitsize / src_bitsize);
            cvt->len_mult *= mult;
            cvt->len_ratio *= mult;
        } else if (src_bitsize > dst_bitsize) {
            cvt->len_ratio /= (src_bitsize / dst_bitsize);
        }

        return 1;               /* added a converter. */
    }

    return 0;                   /* no conversion necessary. */
}

/* Generate the necessary IIR lowpass coefficients for resampling.
   Assume that the SDL_AudioCVT struct is already set up with
   the correct values for len_mult and len_div, and use the
   type of dst_format. Also assume the buffer is allocated.
   Note the buffer needs to be 6 units long.
   For now, use RBJ's cookbook coefficients. It might be more
   optimal to create a Butterworth filter, but this is more difficult.
*/
#if 0
int
SDL_BuildIIRLowpass(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    float fc;                   /* cutoff frequency */
    float coeff[6];             /* floating point iir coefficients b0, b1, b2, a0, a1, a2 */
    float scale;
    float w0, alpha, cosw0;
    int i;

    /* The higher Q is, the higher CUTOFF can be. Need to find a good balance to avoid aliasing */
    static const float Q = 5.0f;
    static const float CUTOFF = 0.4f;

    fc = (cvt->len_mult >
          cvt->len_div) ? CUTOFF / (float) cvt->len_mult : CUTOFF /
        (float) cvt->len_div;

    w0 = 2.0f * M_PI * fc;
    cosw0 = cosf(w0);
    alpha = sinf(w0) / (2.0f * Q);

    /* Compute coefficients, normalizing by a0 */
    scale = 1.0f / (1.0f + alpha);

    coeff[0] = (1.0f - cosw0) / 2.0f * scale;
    coeff[1] = (1.0f - cosw0) * scale;
    coeff[2] = coeff[0];

    coeff[3] = 1.0f;            /* a0 is normalized to 1 */
    coeff[4] = -2.0f * cosw0 * scale;
    coeff[5] = (1.0f - alpha) * scale;

    /* Copy the coefficients to the struct. If necessary, convert coefficients to fixed point, using the range (-2.0, 2.0) */
#define convert_fixed(type, fix) { \
            type *cvt_coeff = (type *)cvt->coeff; \
            for(i = 0; i < 6; ++i) { \
                cvt_coeff[i] = fix(coeff[i]); \
            } \
        }

    if (SDL_AUDIO_ISFLOAT(format) && SDL_AUDIO_BITSIZE(format) == 32) {
        float *cvt_coeff = (float *) cvt->coeff;
        for (i = 0; i < 6; ++i) {
            cvt_coeff[i] = coeff[i];
        }
    } else {
        switch (SDL_AUDIO_BITSIZE(format)) {
        case 8:
            convert_fixed(Uint8, SDL_Make_2_6);
            break;
        case 16:
            convert_fixed(Uint16, SDL_Make_2_14);
            break;
        case 32:
            convert_fixed(Uint32, SDL_Make_2_30);
            break;
        }
    }

#ifdef DEBUG_CONVERT
#define debug_iir(type) { \
            type *cvt_coeff = (type *)cvt->coeff; \
            for(i = 0; i < 6; ++i) { \
                printf("coeff[%u] = %f = 0x%x\n", i, coeff[i], cvt_coeff[i]); \
            } \
        }
    if (SDL_AUDIO_ISFLOAT(format) && SDL_AUDIO_BITSIZE(format) == 32) {
        float *cvt_coeff = (float *) cvt->coeff;
        for (i = 0; i < 6; ++i) {
            printf("coeff[%u] = %f = %f\n", i, coeff[i], cvt_coeff[i]);
        }
    } else {
        switch (SDL_AUDIO_BITSIZE(format)) {
        case 8:
            debug_iir(Uint8);
            break;
        case 16:
            debug_iir(Uint16);
            break;
        case 32:
            debug_iir(Uint32);
            break;
        }
    }
#undef debug_iir
#endif

    /* Initialize the state buffer to all zeroes, and set initial position */
    SDL_memset(cvt->state_buf, 0, 4 * SDL_AUDIO_BITSIZE(format) / 4);
    cvt->state_pos = 0;
#undef convert_fixed

    return 0;
}
#endif

/* Apply the lowpass IIR filter to the given SDL_AudioCVT struct */
/* This was implemented because it would be much faster than the fir filter, 
   but it doesn't seem to have a steep enough cutoff so we'd need several
   cascaded biquads, which probably isn't a great idea. Therefore, this
   function can probably be discarded.
*/
static void
SDL_FilterIIR(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    Uint32 i, n;

    /* TODO: Check that n is calculated right */
    n = 8 * cvt->len_cvt / SDL_AUDIO_BITSIZE(format);

    /* Note that the coefficients are 2_x and the input is 1_x. Do we need to shift left at the end here? The right shift temp = buf[n] >> 1 needs to depend on whether the type is signed or not for sign extension. */
    /* cvt->state_pos = 1: state[0] = x_n-1, state[1] = x_n-2, state[2] = y_n-1, state[3] - y_n-2 */
#define iir_fix(type, mult) {\
            type *coeff = (type *)cvt->coeff; \
            type *state = (type *)cvt->state_buf; \
            type *buf = (type *)cvt->buf; \
            type temp; \
            for(i = 0; i < n; ++i) { \
                    temp = buf[i] >> 1; \
                    if(cvt->state_pos) { \
                        buf[i] = mult(coeff[0], temp) + mult(coeff[1], state[0]) + mult(coeff[2], state[1]) - mult(coeff[4], state[2]) - mult(coeff[5], state[3]); \
                        state[1] = temp; \
                        state[3] = buf[i]; \
                        cvt->state_pos = 0; \
                    } else { \
                        buf[i] = mult(coeff[0], temp) + mult(coeff[1], state[1]) + mult(coeff[2], state[0]) - mult(coeff[4], state[3]) - mult(coeff[5], state[2]); \
                        state[0] = temp; \
                        state[2] = buf[i]; \
                        cvt->state_pos = 1; \
                    } \
                } \
        }
/* Need to test to see if the previous method or this one is faster */
/*#define iir_fix(type, mult) {\
            type *coeff = (type *)cvt->coeff; \
            type *state = (type *)cvt->state_buf; \
            type *buf = (type *)cvt->buf; \
            type temp; \
            for(i = 0; i < n; ++i) { \
                    temp = buf[i] >> 1; \
                    buf[i] = mult(coeff[0], temp) + mult(coeff[1], state[0]) + mult(coeff[2], state[1]) - mult(coeff[4], state[2]) - mult(coeff[5], state[3]); \
                    state[1] = state[0]; \
                    state[0] = temp; \
                    state[3] = state[2]; \
                    state[2] = buf[i]; \
                } \
        }*/

    if (SDL_AUDIO_ISFLOAT(format) && SDL_AUDIO_BITSIZE(format) == 32) {
        float *coeff = (float *) cvt->coeff;
        float *state = (float *) cvt->state_buf;
        float *buf = (float *) cvt->buf;
        float temp;

        for (i = 0; i < n; ++i) {
            /* y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] - a1 * y[n-1] - a[2] * y[n-2] */
            temp = buf[i];
            if (cvt->state_pos) {
                buf[i] =
                    coeff[0] * buf[n] + coeff[1] * state[0] +
                    coeff[2] * state[1] - coeff[4] * state[2] -
                    coeff[5] * state[3];
                state[1] = temp;
                state[3] = buf[i];
                cvt->state_pos = 0;
            } else {
                buf[i] =
                    coeff[0] * buf[n] + coeff[1] * state[1] +
                    coeff[2] * state[0] - coeff[4] * state[3] -
                    coeff[5] * state[2];
                state[0] = temp;
                state[2] = buf[i];
                cvt->state_pos = 1;
            }
        }
    } else {
        /* Treat everything as signed! */
        switch (SDL_AUDIO_BITSIZE(format)) {
        case 8:
            iir_fix(Sint8, SDL_FixMpy8);
            break;
        case 16:
            iir_fix(Sint16, SDL_FixMpy16);
            break;
        case 32:
            iir_fix(Sint32, SDL_FixMpy32);
            break;
        }
    }
#undef iir_fix
}

/* Apply the windowed sinc FIR filter to the given SDL_AudioCVT struct.
*/
static void
SDL_FilterFIR(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int n = 8 * cvt->len_cvt / SDL_AUDIO_BITSIZE(format);
    int m = cvt->len_sinc;
    int i, j;

    /* 
       Note: We can make a big optimization here by taking advantage
       of the fact that the signal is zero stuffed, so we can do
       significantly fewer multiplications and additions. However, this
       depends on the zero stuffing ratio, so it may not pay off. This would
       basically be a polyphase filter.
     */
    /* One other way to do this fast is to look at the fir filter from a different angle:
       After we zero stuff, we have input of all zeroes, except for every len_mult
       sample. If we choose a sinc length equal to len_mult, then the fir filter becomes
       much more simple: we're just taking a windowed sinc, shifting it to start at each
       len_mult sample, and scaling it by the value of that sample. If we do this, then
       we don't even need to worry about the sample histories, and the inner loop here is
       unnecessary. This probably sacrifices some quality but could really speed things up as well.
     */
    /* We only calculate the values of samples which are 0 (mod len_div) because
       those are the only ones used. All the other ones are discarded in the
       third step of resampling. This is a huge speedup. As a warning, though,
       if for some reason this is used elsewhere where there are no samples discarded,
       the output will not be corrrect if len_div is not 1. To make this filter a
       generic FIR filter, simply remove the if statement "if(i % cvt->len_div == 0)"
       around the inner loop so that every sample is processed.
     */
    /* This is basically just a FIR filter. i.e. for input x_n and m coefficients,
       y_n = x_n*sinc_0 + x_(n-1)*sinc_1 +  x_(n-2)*sinc_2 + ... + x_(n-m+1)*sinc_(m-1)
     */
#define filter_sinc(type, mult) { \
            type *sinc = (type *)cvt->coeff; \
            type *state = (type *)cvt->state_buf; \
            type *buf = (type *)cvt->buf; \
            for(i = 0; i < n; ++i) { \
                state[cvt->state_pos] = buf[i]; \
                buf[i] = 0; \
                if( i % cvt->len_div == 0 ) { \
                    for(j = 0; j < m;  ++j) { \
                        buf[i] += mult(sinc[j], state[(cvt->state_pos + j) % m]); \
                    } \
                }\
                cvt->state_pos = (cvt->state_pos + 1) % m; \
            } \
        }

    if (SDL_AUDIO_ISFLOAT(format) && SDL_AUDIO_BITSIZE(format) == 32) {
        filter_sinc(float, SDL_FloatMpy);
    } else {
        switch (SDL_AUDIO_BITSIZE(format)) {
        case 8:
            filter_sinc(Sint8, SDL_FixMpy8);
            break;
        case 16:
            filter_sinc(Sint16, SDL_FixMpy16);
            break;
        case 32:
            filter_sinc(Sint32, SDL_FixMpy32);
            break;
        }
    }

#undef filter_sinc

}

/* Generate the necessary windowed sinc filter for resampling.
   Assume that the SDL_AudioCVT struct is already set up with
   the correct values for len_mult and len_div, and use the
   type of dst_format. Also assume the buffer is allocated.
   Note the buffer needs to be m+1 units long.
*/
int
SDL_BuildWindowedSinc(SDL_AudioCVT * cvt, SDL_AudioFormat format,
                      unsigned int m)
{
    float *fSinc;               /* floating point sinc buffer, to be converted to fixed point */
    float fc;                   /* cutoff frequency */
    float two_pi_fc, two_pi_over_m, four_pi_over_m, m_over_two;
    float norm_sum, norm_fact;
    unsigned int i;

    /* Check that the buffer is allocated */
    if (cvt->coeff == NULL) {
        return -1;
    }

    /* Set the length */
    cvt->len_sinc = m + 1;

    /* Allocate the floating point windowed sinc. */
    fSinc = SDL_stack_alloc(float, (m + 1));
    if (fSinc == NULL) {
        return -1;
    }

    /* Set up the filter parameters */
    fc = (cvt->len_mult >
          cvt->len_div) ? 0.5f / (float) cvt->len_mult : 0.5f /
        (float) cvt->len_div;
#ifdef DEBUG_CONVERT
    printf("Lowpass cutoff frequency = %f\n", fc);
#endif
    two_pi_fc = 2.0f * M_PI * fc;
    two_pi_over_m = 2.0f * M_PI / (float) m;
    four_pi_over_m = 2.0f * two_pi_over_m;
    m_over_two = (float) m / 2.0f;
    norm_sum = 0.0f;

    for (i = 0; i <= m; ++i) {
        if (i == m / 2) {
            fSinc[i] = two_pi_fc;
        } else {
            fSinc[i] =
                sinf(two_pi_fc * ((float) i - m_over_two)) / ((float) i -
                                                              m_over_two);
            /* Apply blackman window */
            fSinc[i] *=
                0.42f - 0.5f * cosf(two_pi_over_m * (float) i) +
                0.08f * cosf(four_pi_over_m * (float) i);
        }
        norm_sum += fSinc[i] < 0 ? -fSinc[i] : fSinc[i];        /* fabs(fSinc[i]); */
    }

    norm_fact = 1.0f / norm_sum;

#define convert_fixed(type, fix) { \
        type *dst = (type *)cvt->coeff; \
        for( i = 0; i <= m; ++i ) { \
            dst[i] = fix(fSinc[i] * norm_fact); \
        } \
    }

    /* If we're using floating point, we only need to normalize */
    if (SDL_AUDIO_ISFLOAT(format) && SDL_AUDIO_BITSIZE(format) == 32) {
        float *fDest = (float *) cvt->coeff;
        for (i = 0; i <= m; ++i) {
            fDest[i] = fSinc[i] * norm_fact;
        }
    } else {
        switch (SDL_AUDIO_BITSIZE(format)) {
        case 8:
            convert_fixed(Uint8, SDL_Make_1_7);
            break;
        case 16:
            convert_fixed(Uint16, SDL_Make_1_15);
            break;
        case 32:
            convert_fixed(Uint32, SDL_Make_1_31);
            break;
        }
    }

    /* Initialize the state buffer to all zeroes, and set initial position */
    SDL_memset(cvt->state_buf, 0,
               cvt->len_sinc * SDL_AUDIO_BITSIZE(format) / 4);
    cvt->state_pos = 0;

    /* Clean up */
#undef convert_fixed
    SDL_stack_free(fSinc);

    return 0;
}

/* This is used to reduce the resampling ratio */
static __inline__ int
SDL_GCD(int a, int b)
{
    int temp;
    while (b != 0) {
        temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

/* Perform proper resampling. This is pretty slow but it's the best-sounding method. */
static void SDLCALL
SDL_Resample(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i, j;

#ifdef DEBUG_CONVERT
    printf("Converting audio rate via proper resampling (mono)\n");
#endif

#define zerostuff_mono(type) { \
        const type *src = (const type *) (cvt->buf + cvt->len_cvt); \
        type *dst = (type *) (cvt->buf + (cvt->len_cvt * cvt->len_mult)); \
        for (i = cvt->len_cvt / sizeof (type); i; --i) { \
            src--; \
            dst[-1] = src[0]; \
            for( j = -cvt->len_mult; j < -1; ++j ) { \
                dst[j] = 0; \
            } \
            dst -= cvt->len_mult; \
        } \
    }

#define discard_mono(type) { \
        const type *src = (const type *) (cvt->buf); \
        type *dst = (type *) (cvt->buf); \
        for (i = 0; i < (cvt->len_cvt / sizeof(type)) / cvt->len_div; ++i) { \
            dst[0] = src[0]; \
            src += cvt->len_div; \
            ++dst; \
        } \
    }

    /* Step 1: Zero stuff the conversion buffer. This upsamples by a factor of len_mult,
       creating aliasing at frequencies above the original nyquist frequency.
     */
#ifdef DEBUG_CONVERT
    printf("Zero-stuffing by a factor of %u\n", cvt->len_mult);
#endif
    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        zerostuff_mono(Uint8);
        break;
    case 16:
        zerostuff_mono(Uint16);
        break;
    case 32:
        zerostuff_mono(Uint32);
        break;
    }

    cvt->len_cvt *= cvt->len_mult;

    /* Step 2: Use a windowed sinc FIR filter (lowpass filter) to remove the alias
       frequencies. This is the slow part.
     */
    SDL_FilterFIR(cvt, format);

    /* Step 3: Now downsample by discarding samples. */

#ifdef DEBUG_CONVERT
    printf("Discarding samples by a factor of %u\n", cvt->len_div);
#endif
    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        discard_mono(Uint8);
        break;
    case 16:
        discard_mono(Uint16);
        break;
    case 32:
        discard_mono(Uint32);
        break;
    }

#undef zerostuff_mono
#undef discard_mono

    cvt->len_cvt /= cvt->len_div;

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Creates a set of audio filters to convert from one format to another.
   Returns -1 if the format conversion is not supported, 0 if there's
   no conversion needed, or 1 if the audio filter is set up.
*/

int
SDL_BuildAudioCVT(SDL_AudioCVT * cvt,
                  SDL_AudioFormat src_fmt, Uint8 src_channels, int src_rate,
                  SDL_AudioFormat dst_fmt, Uint8 dst_channels, int dst_rate)
{
    /* there are no unsigned types over 16 bits, so catch this upfront. */
    if ((SDL_AUDIO_BITSIZE(src_fmt) > 16) && (!SDL_AUDIO_ISSIGNED(src_fmt))) {
        return -1;
    }
    if ((SDL_AUDIO_BITSIZE(dst_fmt) > 16) && (!SDL_AUDIO_ISSIGNED(dst_fmt))) {
        return -1;
    }
#ifdef DEBUG_CONVERT
    printf("Build format %04x->%04x, channels %u->%u, rate %d->%d\n",
           src_fmt, dst_fmt, src_channels, dst_channels, src_rate, dst_rate);
#endif

    /* Start off with no conversion necessary */

    cvt->src_format = src_fmt;
    cvt->dst_format = dst_fmt;
    cvt->needed = 0;
    cvt->filter_index = 0;
    cvt->filters[0] = NULL;
    cvt->len_mult = 1;
    cvt->len_ratio = 1.0;

    /* Convert data types, if necessary. Updates (cvt). */
    if (SDL_BuildAudioTypeCVT(cvt, src_fmt, dst_fmt) == -1)
        return -1;              /* shouldn't happen, but just in case... */

    /* Channel conversion */
    if (src_channels != dst_channels) {
        if ((src_channels == 1) && (dst_channels > 1)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertStereo;
            cvt->len_mult *= 2;
            src_channels = 2;
            cvt->len_ratio *= 2;
        }
        if ((src_channels == 2) && (dst_channels == 6)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertSurround;
            src_channels = 6;
            cvt->len_mult *= 3;
            cvt->len_ratio *= 3;
        }
        if ((src_channels == 2) && (dst_channels == 4)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertSurround_4;
            src_channels = 4;
            cvt->len_mult *= 2;
            cvt->len_ratio *= 2;
        }
        while ((src_channels * 2) <= dst_channels) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertStereo;
            cvt->len_mult *= 2;
            src_channels *= 2;
            cvt->len_ratio *= 2;
        }
        if ((src_channels == 6) && (dst_channels <= 2)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertStrip;
            src_channels = 2;
            cvt->len_ratio /= 3;
        }
        if ((src_channels == 6) && (dst_channels == 4)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertStrip_2;
            src_channels = 4;
            cvt->len_ratio /= 2;
        }
        /* This assumes that 4 channel audio is in the format:
           Left {front/back} + Right {front/back}
           so converting to L/R stereo works properly.
         */
        while (((src_channels % 2) == 0) &&
               ((src_channels / 2) >= dst_channels)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertMono;
            src_channels /= 2;
            cvt->len_ratio /= 2;
        }
        if (src_channels != dst_channels) {
            /* Uh oh.. */ ;
        }
    }

    /* Do rate conversion */
    if (src_rate != dst_rate) {
        int rate_gcd;
        rate_gcd = SDL_GCD(src_rate, dst_rate);
        cvt->len_mult = dst_rate / rate_gcd;
        cvt->len_div = src_rate / rate_gcd;
        cvt->len_ratio = (double) cvt->len_mult / (double) cvt->len_div;
        cvt->filters[cvt->filter_index++] = SDL_Resample;
        SDL_BuildWindowedSinc(cvt, dst_fmt, 768);
    }

/*
    cvt->rate_incr = 0.0;
    if ((src_rate / 100) != (dst_rate / 100)) {
        Uint32 hi_rate, lo_rate;
        int len_mult;
        double len_ratio;
        SDL_AudioFilter rate_cvt = NULL;

        if (src_rate > dst_rate) {
            hi_rate = src_rate;
            lo_rate = dst_rate;
            switch (src_channels) {
            case 1:
                rate_cvt = SDL_RateDIV2;
                break;
            case 2:
                rate_cvt = SDL_RateDIV2_c2;
                break;
            case 4:
                rate_cvt = SDL_RateDIV2_c4;
                break;
            case 6:
                rate_cvt = SDL_RateDIV2_c6;
                break;
            default:
                return -1;
            }
            len_mult = 1;
            len_ratio = 0.5;
        } else {
            hi_rate = dst_rate;
            lo_rate = src_rate;
            switch (src_channels) {
            case 1:
                rate_cvt = SDL_RateMUL2;
                break;
            case 2:
                rate_cvt = SDL_RateMUL2_c2;
                break;
            case 4:
                rate_cvt = SDL_RateMUL2_c4;
                break;
            case 6:
                rate_cvt = SDL_RateMUL2_c6;
                break;
            default:
                return -1;
            }
            len_mult = 2;
            len_ratio = 2.0;
        }*/
    /* If hi_rate = lo_rate*2^x then conversion is easy */
    /*   while (((lo_rate * 2) / 100) <= (hi_rate / 100)) {
       cvt->filters[cvt->filter_index++] = rate_cvt;
       cvt->len_mult *= len_mult;
       lo_rate *= 2;
       cvt->len_ratio *= len_ratio;
       } */
    /* We may need a slow conversion here to finish up */
    /*    if ((lo_rate / 100) != (hi_rate / 100)) {
       #if 1 */
    /* The problem with this is that if the input buffer is
       say 1K, and the conversion rate is say 1.1, then the
       output buffer is 1.1K, which may not be an acceptable
       buffer size for the audio driver (not a power of 2)
     */
    /* For now, punt and hope the rate distortion isn't great.
     */
/*#else
            if (src_rate < dst_rate) {
                cvt->rate_incr = (double) lo_rate / hi_rate;
                cvt->len_mult *= 2;
                cvt->len_ratio /= cvt->rate_incr;
            } else {
                cvt->rate_incr = (double) hi_rate / lo_rate;
                cvt->len_ratio *= cvt->rate_incr;
            }
            cvt->filters[cvt->filter_index++] = SDL_RateSLOW;
#endif
        }
    }*/

    /* Set up the filter information */
    if (cvt->filter_index != 0) {
        cvt->needed = 1;
        cvt->src_format = src_fmt;
        cvt->dst_format = dst_fmt;
        cvt->len = 0;
        cvt->buf = NULL;
        cvt->filters[cvt->filter_index] = NULL;
    }
    return (cvt->needed);
}

#undef SDL_FixMpy8
#undef SDL_FixMpy16
#undef SDL_FixMpy32
#undef SDL_FloatMpy
#undef SDL_Make_1_7
#undef SDL_Make_1_15
#undef SDL_Make_1_31
#undef SDL_Make_2_6
#undef SDL_Make_2_14
#undef SDL_Make_2_30

/* vi: set ts=4 sw=4 expandtab: */
