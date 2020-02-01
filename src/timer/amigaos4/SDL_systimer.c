/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

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
#include "../../SDL_internal.h"

#if defined(SDL_TIMER_AMIGAOS4) || defined(SDL_TIMERS_DISABLED)

#include <devices/timer.h>

#include "SDL_timer.h"
#include "SDL_os4timer_c.h"

#undef DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static SDL_bool started = SDL_FALSE;

void
SDL_TicksInit(void)
{
    if (started) {
        return;
    }
    started = SDL_TRUE;
}

void
SDL_TicksQuit(void)
{
    started = SDL_FALSE;
}

Uint32
SDL_GetTicks(void)
{
    if (!started) {
        SDL_TicksInit();
    }

    struct TimeVal cur;
    OS4_TimerGetTime(&cur);

    return cur.Seconds * 1000 + cur.Microseconds / 1000;
}

Uint64
SDL_GetPerformanceCounter(void)
{
    return OS4_TimerGetCounter();
}

Uint64
SDL_GetPerformanceFrequency(void)
{
    return OS4_TimerGetFrequency();
}

void
SDL_Delay(Uint32 ms)
{
    OS4_TimerDelay(ms);
}

#endif /* SDL_TIMER_AMIGAOS4 || SDL_TIMERS_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
