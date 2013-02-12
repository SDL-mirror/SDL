/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL_bits.h
 *
 *  Functions for fiddling with bits and bitmasks.
 */

#ifndef _SDL_bits_h
#define _SDL_bits_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \file SDL_bits.h
 *
 *  Uses inline functions for compilers that support them, and static
 *  functions for those that do not.  Because these functions become
 *  static for compilers that do not support inline functions, this
 *  header should only be included in files that actually use them.
 */

/**
 *  Get the index of the most significant bit. Result is undefined when called
 *  with 0. This operation can also be stated as "count leading zeroes" and
 *  "log base 2".
 *
 *  \return Index of the most significant bit.
 */
static __inline__ Sint8
SDL_MostSignificantBitIndex32(Uint32 x)
{
#if defined(__GNUC__)
    /* Count Leading Zeroes builtin in GCC.
     * http://gcc.gnu.org/onlinedocs/gcc-4.3.4/gcc/Other-Builtins.html
     */
    return 31 - __builtin_clz(x);
#else
    /* Based off of Bit Twiddling Hacks by Sean Eron Anderson
     * <seander@cs.stanford.edu>, released in the public domain.
     * http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogLookup
     */
    static const Sint8 LogTable256[256] =
    {
    #define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
        -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
        LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
    #undef LT
    };

    register unsigned int t, tt;

    if (tt = x >> 16)
    {
      return ((t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt]);
    }
    else
    {
      return ((t = x >> 8) ? 8 + LogTable256[t] : LogTable256[x]);
    }
#endif
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_bits_h */

/* vi: set ts=4 sw=4 expandtab: */
