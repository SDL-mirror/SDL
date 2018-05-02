/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2015 Sam Lantinga <slouken@libsdl.org>

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

#ifdef SDL_TIMER_SWITCH

#include "SDL_thread.h"
#include "SDL_timer.h"
#include "../SDL_timer_c.h"
#include <switch.h>

static bool started = false;

static Uint64 start = 0;

void
SDL_TicksInit(void)
{
    if (started) {
        return;
    }

    start = SDL_GetPerformanceCounter();
    started = true;
}

void
SDL_TicksQuit(void)
{
    started = false;
}

Uint32 SDL_GetTicks(void)
{
    if (!started) {
        SDL_TicksInit();
    }

    return (Uint32) ((SDL_GetPerformanceCounter() - start) * 1000 / SDL_GetPerformanceFrequency());
}

Uint64
SDL_GetPerformanceCounter(void)
{
    return svcGetSystemTick();
}

Uint64
SDL_GetPerformanceFrequency(void)
{
    return 19200000;
}

void
SDL_Delay(Uint32 ms)
{
    svcSleepThread((Uint64) ms * 1000000);
}

#endif /* SDL_TIMER_SWITCH */
