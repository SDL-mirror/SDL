/*
 * Copyright (c) 2018 RISC OS Open Ltd
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/* Prevent the stack from becoming executable for no reason... */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

    .text
    .fpu neon
    .arch armv7a
    .object_arch armv4
    .eabi_attribute 10, 0 /* suppress Tag_FP_arch */
    .eabi_attribute 12, 0 /* suppress Tag_Advanced_SIMD_arch */
    .arm
    .altmacro
    .p2align 2

#include "pixman-arm-asm.h"
#include "pixman-arm-neon-asm.h"

/******************************************************************************/

.macro RGBtoRGBPixelAlpha_process_pixblock_head
    vmvn        d30, d3  /* get inverted source alpha */
    vmov        d31, d7  /* dest alpha is always unchanged */
    vmull.u8    q14, d0, d3
    vmlal.u8    q14, d4, d30
    vmull.u8    q0, d1, d3
    vmlal.u8    q0, d5, d30
    vmull.u8    q1, d2, d3
    vmlal.u8    q1, d6, d30
    vrshr.u16   q2, q14, #8
    vrshr.u16   q3, q0, #8
    vraddhn.u16 d28, q14, q2
    vrshr.u16   q2, q1, #8
    vraddhn.u16 d29, q0, q3
    vraddhn.u16 d30, q1, q2
.endm

.macro RGBtoRGBPixelAlpha_process_pixblock_tail
    /* nothing */
.endm

.macro RGBtoRGBPixelAlpha_process_pixblock_tail_head
    vld4.8      {d0-d3}, [SRC]!
                                    PF add PF_X, PF_X, #8
        vst4.8      {d28-d31}, [DST_W :128]!
                                    PF tst PF_CTL, #0xF
    vld4.8      {d4-d7}, [DST_R :128]!
                                    PF addne PF_X, PF_X, #8
    vmvn        d30, d3  /* get inverted source alpha */
    vmov        d31, d7  /* dest alpha is always unchanged */
    vmull.u8    q14, d0, d3
                                    PF subne PF_CTL, PF_CTL, #1
    vmlal.u8    q14, d4, d30
                                    PF cmp PF_X, ORIG_W
    vmull.u8    q0, d1, d3
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
    vmlal.u8    q0, d5, d30
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
    vmull.u8    q1, d2, d3
                                    PF subge PF_X, PF_X, ORIG_W
    vmlal.u8    q1, d6, d30
                                    PF subges PF_CTL, PF_CTL, #0x10
    vrshr.u16   q2, q14, #8
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
    vrshr.u16   q3, q0, #8
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
    vraddhn.u16 d28, q14, q2
    vrshr.u16   q2, q1, #8
    vraddhn.u16 d29, q0, q3
    vraddhn.u16 d30, q1, q2
.endm

generate_composite_function \
    BlitRGBtoRGBPixelAlphaARMNEONAsm, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    RGBtoRGBPixelAlpha_process_pixblock_head, \
    RGBtoRGBPixelAlpha_process_pixblock_tail, \
    RGBtoRGBPixelAlpha_process_pixblock_tail_head
