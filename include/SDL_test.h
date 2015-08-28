/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

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
 *  \file SDL_test.h
 *
 *  Include file for SDL test framework.
 *
 *  This code is a part of the SDL2_test library, not the main SDL library.
 */

#ifndef _SDL_test_h
#define _SDL_test_h

#include "SDL.h"
#include "SDL_test_common.h"
#include "SDL_test_font.h"
#include "SDL_test_random.h"
#include "SDL_test_fuzzer.h"
#include "SDL_test_crc32.h"
#include "SDL_test_md5.h"
#include "SDL_test_log.h"
#include "SDL_test_assert.h"
#include "SDL_test_harness.h"
#include "SDL_test_images.h"
#include "SDL_test_compare.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Global definitions */
#ifdef __amigaos4__
#   ifndef __LONG_MAX__
#     define __LONG_MAX__ 2147483647L
#   endif
#   undef LONG_MIN
#   define LONG_MIN (-LONG_MAX-1)
#   undef LONG_MAX
#   define LONG_MAX __LONG_MAX__

/* Maximum value an `unsigned long int' can hold.  (Minimum is 0).  */
#   undef ULONG_MAX
#   define ULONG_MAX (LONG_MAX * 2UL + 1)

#   ifndef __LONG_LONG_MAX__
#    define __LONG_LONG_MAX__ 9223372036854775807LL
#   endif

/* Minimum and maximum values a `signed long long int' can hold.  */
#    undef LLONG_MIN
#    define LLONG_MIN (-LLONG_MAX-1)
#    undef LLONG_MAX
#    define LLONG_MAX __LONG_LONG_MAX__

/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0).  */
#    undef ULLONG_MAX
#    define ULLONG_MAX (LLONG_MAX * 2ULL + 1)
#endif
/*
 * Note: Maximum size of SDLTest log message is less than SDLs limit
 * to ensure we can fit additional information such as the timestamp.
 */
#define SDLTEST_MAX_LOGMESSAGE_LENGTH   3584

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_test_h */

/* vi: set ts=4 sw=4 expandtab: */
