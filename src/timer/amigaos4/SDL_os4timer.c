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

#include "SDL_types.h"

#include <proto/exec.h>
#include <proto/timer.h>
#include <exec/execbase.h>
#include <devices/timer.h>
#include <libraries/dos.h>

#include "SDL_os4timer_c.h"

#include "../../thread/amigaos4/SDL_systhread_c.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static struct TimeVal OS4_StartTime;

static struct TimerIFace* SDL2_ITimer;

static ULONG OS4_TimerFrequency;

typedef struct OS4_ClockVal {
    union {
        struct EClockVal cv;
        Uint64 ticks;
    } u;
} OS4_ClockVal;

// Initialized with the thread sub system
void OS4_InitTimerSubSystem(void)
{
    dprintf("Called\n");

    struct ExecBase* sysbase = (struct ExecBase *)IExec->Data.LibBase;
    struct Library* timerBase = (struct Library *)IExec->FindName(&sysbase->DeviceList, "timer.device");

    SDL2_ITimer = (struct TimerIFace *)IExec->GetInterface(timerBase, "main", 1, NULL);

    dprintf("ITimer %p\n", SDL2_ITimer);

    if (!SDL2_ITimer) {
        dprintf("Failed to get ITimer\n");
        return;
    }

    SDL2_ITimer->GetSysTime(&OS4_StartTime);

    struct EClockVal cv;
    OS4_TimerFrequency = SDL2_ITimer->ReadEClock(&cv);

    dprintf("Timer frequency %u Hz\n", OS4_TimerFrequency);
}

void OS4_QuitTimerSubSystem(void)
{
    dprintf("Called\n");

    IExec->DropInterface((struct Interface *)SDL2_ITimer);
    SDL2_ITimer = NULL;
}

static void
OS4_TimerCleanup(OS4_TimerInstance * timer)
{
    if (timer) {
        if (timer->timerRequest) {
            dprintf("Freeing timer request %p\n", timer->timerRequest);
            IExec->FreeSysObject(ASOT_IOREQUEST, timer->timerRequest);
            timer->timerRequest = NULL;
        }

        if (timer->timerPort) {
            dprintf("Freeing timer port %p\n", timer->timerPort);
            IExec->FreeSysObject(ASOT_PORT, timer->timerPort);
            timer->timerPort = NULL;
        }
    }
}

BOOL
OS4_TimerCreate(OS4_TimerInstance * timer)
{
    BOOL success = FALSE;

    dprintf("Creating timer %p for task %p\n", timer, IExec->FindTask(NULL));

    if (!timer) {
        return FALSE;
    }

    timer->timerPort = IExec->AllocSysObject(ASOT_PORT, NULL);

    if (timer->timerPort) {
    	timer->timerRequest = IExec->AllocSysObjectTags(ASOT_IOREQUEST,
                                                        ASOIOR_ReplyPort, timer->timerPort,
                                                        ASOIOR_Size, sizeof(struct TimeRequest),
                                                        TAG_DONE);

        if (timer->timerRequest) {
            if (!(IExec->OpenDevice("timer.device", UNIT_WAITUNTIL, (struct IORequest *)timer->timerRequest, 0))) {
                success = TRUE;
            } else {
                dprintf("Failed to open timer.device\n");
            }
        } else {
            dprintf("Failed to allocate timer request\n");
        }
    } else {
        dprintf("Failed to allocate timer port\n");
    }

    if (!success) {
        OS4_TimerCleanup(timer);
    }

    return success;
}

void
OS4_TimerDestroy(OS4_TimerInstance * timer)
{
    dprintf("Destroying timer %p for task %p\n", timer, IExec->FindTask(NULL));

    if (timer && timer->timerRequest) {
        if (!IExec->CheckIO((struct IORequest *)timer->timerRequest)) {
            IExec->AbortIO((struct IORequest *)timer->timerRequest);
            IExec->WaitIO((struct IORequest *)timer->timerRequest);
        }
    }

    OS4_TimerCleanup(timer);
}

ULONG
OS4_TimerSetAlarm(OS4_TimerInstance * timer, Uint32 alarmTicks)
{
    const ULONG seconds = alarmTicks / 1000;
    struct TimeVal now;

    if (!SDL2_ITimer) {
        dprintf("Timer subsystem not initialized\n");
        return 0;
    }

    //dprintf("Called for timer %p, ticks %u\n", timer, alarmTicks);

    timer->timerRequest->Request.io_Command = TR_ADDREQUEST;
    timer->timerRequest->Time.Seconds = seconds;
    timer->timerRequest->Time.Microseconds  = (alarmTicks - (seconds * 1000)) * 1000;

    SDL2_ITimer->GetSysTime(&now);
    SDL2_ITimer->AddTime(&timer->timerRequest->Time, &now);

    IExec->SetSignal(0, 1L << timer->timerPort->mp_SigBit);
    IExec->SendIO((struct IORequest *)timer->timerRequest);

    // Return the alarm signal for Wait() use
    return 1L << timer->timerPort->mp_SigBit;
}

void
OS4_TimerClearAlarm(OS4_TimerInstance * timer)
{
    if (!IExec->CheckIO((struct IORequest *)timer->timerRequest)) {
        IExec->AbortIO((struct IORequest *)timer->timerRequest);
    }

    IExec->WaitIO((struct IORequest *)timer->timerRequest);
}

BOOL
OS4_TimerDelay(Uint32 ticks)
{
	OS4_TimerInstance* timer = OS4_ThreadGetTimer();

	const ULONG alarmSig = OS4_TimerSetAlarm(timer, ticks);
	const ULONG sigsReceived = IExec->Wait(alarmSig | SIGBREAKF_CTRL_C);

	OS4_TimerClearAlarm(timer);

	return (sigsReceived & alarmSig) == alarmSig;
}

void
OS4_TimerGetTime(struct TimeVal * timeval)
{
    if (!SDL2_ITimer) {
        dprintf("Timer subsystem not initialized\n");
        timeval->Seconds = 0;
        timeval->Microseconds = 0;
        return;
    }

    SDL2_ITimer->GetSysTime(timeval);
    SDL2_ITimer->SubTime(timeval, &OS4_StartTime);
}

Uint64
OS4_TimerGetCounter(void)
{
    OS4_ClockVal value;

    if (!SDL2_ITimer) {
        dprintf("Timer subsystem not initialized\n");
        return 0;
    }

    SDL2_ITimer->ReadEClock(&value.u.cv);

    return value.u.ticks;
}

Uint64
OS4_TimerGetFrequency(void)
{
    return OS4_TimerFrequency;
}

#endif /* (SDL_TIMER_AMIGAOS4) || defined(SDL_TIMERS_DISABLED) */

