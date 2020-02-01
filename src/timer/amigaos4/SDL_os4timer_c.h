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

#ifndef SDL_OS4TIMER_C_H
#define SDL_OS4TIMER_C_H

#include <exec/types.h>

struct MsgPort;
struct TimeRequest;
struct TimeVal;

typedef struct OS4_TimerInstance
{
    struct MsgPort* timerPort;
    struct TimeRequest* timerRequest;
} OS4_TimerInstance;

void OS4_InitTimerSubSystem(void);
void OS4_QuitTimerSubSystem(void);

BOOL OS4_TimerCreate(OS4_TimerInstance * timer);
void OS4_TimerDestroy(OS4_TimerInstance * timer);
ULONG OS4_TimerSetAlarm(OS4_TimerInstance * timer, Uint32 alarmTicks);
void OS4_TimerClearAlarm(OS4_TimerInstance * timer);
BOOL OS4_TimerDelay(Uint32 ticks);
void OS4_TimerGetTime(struct TimeVal * timeval);
Uint64 OS4_TimerGetCounter(void);
Uint64 OS4_TimerGetFrequency(void);

#endif /* SDL_OS4TIMER_C_H */

