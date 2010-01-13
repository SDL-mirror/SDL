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

/* This is an assert macro for SDL's internal use. Not for the public API! */

#ifndef _SDL_assert_h
#define _SDL_assert_h

#ifndef SDL_ASSERT_LEVEL
#error SDL_ASSERT_LEVEL is not defined. Please fix your SDL_config.h.
#endif

/*
sizeof (x) makes the compiler still parse the expression even without
assertions enabled, so the code is always checked at compile time, but
doesn't actually generate code for it, so there are no side effects or
expensive checks at run time, just the constant size of what x WOULD be,
which presumably gets optimized out as unused.
This also solves the problem of...

    int somevalue = blah();
    SDL_assert(somevalue == 1);

...which would cause compiles to complain that somevalue is unused if we
disable assertions.
*/

#define SDL_disabled_assert(condition) \
    do { (void) sizeof ((condition)); } while (0)

#if (SDL_ASSERT_LEVEL > 0)

/*
These are macros and not first class functions so that the debugger breaks
on the assertion line and not in some random guts of SDL, and so each
macro can have unique static variables associated with it.
*/

#if (defined(_MSC_VER) && ((_M_IX86) || (_M_X64)))
    #define SDL_TriggerBreakpoint() __asm { int 3 }
#elif (defined(__GNUC__) && ((__i386__) || (__x86_64__)))
    #define SDL_TriggerBreakpoint() __asm__ __volatile__ ( "int $3\n\t" )
#elif defined(unix)
    #include <signal.h>
    #define SDL_TriggerBreakpoint() raise(SIGTRAP)
#else
    #error Please define your platform or set SDL_ASSERT_LEVEL to 0.
#endif

#if (__STDC_VERSION__ >= 199901L) /* C99 supports __func__ as a standard. */
#   define SDL_FUNCTION __func__
#elif ((__GNUC__ >= 2) || defined(_MSC_VER))
#   define SDL_FUNCTION __FUNCTION__
#else
#   define SDL_FUNCTION "???"
#endif

typedef enum
{
    SDL_ASSERTION_RETRY,  /**< Retry the assert immediately. */
    SDL_ASSERTION_BREAK,  /**< Make the debugger trigger a breakpoint. */
    SDL_ASSERTION_ABORT,  /**< Terminate the program. */
    SDL_ASSERTION_IGNORE,  /**< Ignore the assert. */
    SDL_ASSERTION_ALWAYS_IGNORE,  /**< Ignore the assert from now on. */
} SDL_assert_state;

typedef struct SDL_assert_data
{
    int always_ignore;
    unsigned int trigger_count;
    const char *condition;
    const char *filename;
    int linenum;
    const char *function;
    struct SDL_assert_data *next;
} SDL_assert_data;

SDL_assert_state SDL_ReportAssertion(SDL_assert_data *, const char *, int);

/* the do {} while(0) avoids dangling else problems:
    if (x) SDL_assert(y); else blah();
       ... without the do/while, the "else" could attach to this macro's "if".
   We try to handle just the minimum we need here in a macro...the loop,
   the static vars, and break points. The heavy lifting is handled in
   SDL_ReportAssertion(), in SDL_assert.c.
*/
#define SDL_enabled_assert(condition) \
    do { \
        while ( !(condition) ) { \
			static struct SDL_assert_data assert_data = { \
                0, 0, #condition, __FILE__, 0, 0, 0 \
            }; \
			const SDL_assert_state state = SDL_ReportAssertion(&assert_data, \
                                                               SDL_FUNCTION, \
															   __LINE__); \
            if (state == SDL_ASSERTION_RETRY) { \
                continue; /* go again. */ \
            } else if (state == SDL_ASSERTION_BREAK) { \
                SDL_TriggerBreakpoint(); \
            } \
            break; /* not retrying. */ \
        } \
    } while (0)

#endif  /* enabled assertions support code */

/* Enable various levels of assertions. */
#if SDL_ASSERT_LEVEL == 0   /* assertions disabled */
#   define SDL_assert(condition) SDL_disabled_assert(condition)
#   define SDL_assert_release(condition) SDL_disabled_assert(condition)
#   define SDL_assert_paranoid(condition) SDL_disabled_assert(condition)
#elif SDL_ASSERT_LEVEL == 1  /* release settings. */
#   define SDL_assert(condition) SDL_enabled_assert(condition)
#   define SDL_assert_release(condition) SDL_enabled_assert(condition)
#   define SDL_assert_paranoid(condition) SDL_enabled_assert(condition)
#elif SDL_ASSERT_LEVEL == 2  /* normal settings. */
#   define SDL_assert(condition) SDL_enabled_assert(condition)
#   define SDL_assert_release(condition) SDL_enabled_assert(condition)
#   define SDL_assert_paranoid(condition) SDL_disabled_assert(condition)
#elif SDL_ASSERT_LEVEL == 3  /* paranoid settings. */
#   define SDL_assert(condition) SDL_enabled_assert(condition)
#   define SDL_assert_release(condition) SDL_enabled_assert(condition)
#   define SDL_assert_paranoid(condition) SDL_enabled_assert(condition)
#else
#   error Unknown assertion level. Please fix your SDL_config.h.
#endif

#endif /* _SDL_assert_h */

/* vi: set ts=4 sw=4 expandtab: */

