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

/*
    timer.device support routines for AmigaOS4.0

    These are collected here rather than in src/timer/amigaos4 since
    they are used in both the timer and thread layers.

    Richard Drummond.
    evilrich@rcdrummond.net
 */
#include "SDL_types.h"

#include <proto/exec.h>
#include <proto/timer.h>
#include <exec/execbase.h>
#include <devices/timer.h>

#include "SDL_os4timer_c.h"

//#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

/*
 * All SDL time is measured in milliseconds
 * relative to the time the library was initialized.
 *
 * This is initialized in th constructor below...
 */
struct TimeVal os4timer_starttime;


/*
 * Management of interface to timer.device
 */
struct TimerIFace *ITimer = 0;

void _INIT_os4timer_startup(void)  __attribute__((constructor));
void _EXIT_os4timer_shutdown(void) __attribute__((destructor));

void _INIT_os4timer_startup(void)
{
	struct ExecBase *sysbase = (struct ExecBase*) IExec->Data.LibBase;

	struct Library *timer = (struct Library *)IExec->FindName(&sysbase->DeviceList, "timer.device");
	ITimer = (struct TimerIFace *)IExec->GetInterface(timer, "main", 1, 0);

	dprintf("ITimer = %p\n", ITimer);

	/* Set up reference time. */
	ITimer->GetSysTime(&os4timer_starttime);
}

void _EXIT_os4timer_shutdown(void)
{
	IExec->DropInterface((struct Interface *)ITimer);
}


/*
 * Accessor to local timer instance.
 *
 * If we support multiple threads, then the thread layer instantiates
 * a timer for each thread. If we don't support threads, then we just
 * need a single timer instance.
 */
#ifndef DISABLE_THREADS

# include "../../thread/amigaos4/SDL_systhread_c.h"
# define GetTimerInstance(x) os4thread_GetTimer(x)

#else

static os4timer_Instance timerInstance;
# define GetTimerInstance(x) (&timerInstance)

#endif


/*
 * Allocate resources for a timer instance.
 */
BOOL os4timer_Init(os4timer_Instance *timer)
{
	BOOL success = FALSE;

	dprintf("Initializing timer for thread=%p.\n", IExec->FindTask(NULL));

	timer->timerport = IExec->AllocSysObject(ASOT_PORT, NULL);

	if (timer->timerport)
	{
		timer->timerrequest = IExec->AllocSysObjectTags(ASOT_IOREQUEST,
														ASOIOR_ReplyPort, timer->timerport,
														ASOIOR_Size,      sizeof(struct TimeRequest),
														TAG_DONE);

		if (timer->timerrequest)
		{
			if (!(IExec->OpenDevice("timer.device", UNIT_WAITUNTIL, (struct IORequest *)timer->timerrequest, 0)))
				success = TRUE;
			else
			{
				IExec->FreeSysObject(ASOT_IOREQUEST, timer->timerrequest);
				timer->timerrequest = 0;
	    	}
		}
		else
		{
			IExec->FreeSysObject(ASOT_PORT, timer->timerport);
			timer->timerport = 0;
		}
	}

	dprintf("%s\n", (success ? "Done.\n" : "Failed.\n"));

	return success;
}

/*
 * Free resources associated with a timer instance.
 */
void os4timer_Destroy(os4timer_Instance *timer)
{
	dprintf("Freeing timer for thread:%p.\n", IExec->FindTask(NULL));

	if (timer->timerrequest)
	{
		/* Check for any pending timer requests and abort them */
		if (!IExec->CheckIO((struct IORequest *)timer->timerrequest))
		{
			IExec->AbortIO((struct IORequest *)timer->timerrequest);
			IExec->WaitIO((struct IORequest *)timer->timerrequest);
		}
		IExec->FreeSysObject(ASOT_IOREQUEST, timer->timerrequest);
		timer->timerrequest = 0;
	}

	if (timer->timerport)
	{
		IExec->FreeSysObject(ASOT_PORT, timer->timerport);
		timer->timerport = 0;
	}
}

/*
 * Set up a timer device request to signal us at the specified time.
 *
 * timer:       The timer instance to use.
 * alarmTicks:  The time in milliseconds at which the alarm signal will be generated.
 * alarmSignal: An Exec signal mask which can be used with IExec->Wait() to wait for the
 *              alarm to occur.
 *
 * Returns: FALSE if the alarm could not be set, TRUE otherwise.
 */
BOOL os4timer_SetAlarm(os4timer_Instance *timer, Uint32 alarmTicks, ULONG *alarmSignal)
{
	/* Set up the timer request. */
	timer->timerrequest->Request.io_Command = TR_ADDREQUEST;
	timer->timerrequest->Time.Seconds       = alarmTicks / 1000;
	timer->timerrequest->Time.Microseconds  = (alarmTicks - (timer->timerrequest->Time.Seconds * 1000)) * 1000;
	ITimer->AddTime(&timer->timerrequest->Time, &os4timer_starttime);

	IExec->SetSignal(0, 1L << timer->timerport->mp_SigBit);

	/* Send the request. */
	IExec->SendIO((struct IORequest *)timer->timerrequest);

	/* Return alarm signal to caller. */
	*alarmSignal = 1L << timer->timerport->mp_SigBit;

	return TRUE;
}

/*
 * Clear a previous alarm request.
 *
 * timer: The timer instance to use.
 */
VOID os4timer_ClearAlarm(os4timer_Instance *timer)
{
	/* If the timer request did not complete, abort the request. */
	if (!IExec->CheckIO((struct IORequest *)timer->timerrequest))
		IExec->AbortIO((struct IORequest *)timer->timerrequest);

	/* Remove the complete or aborted time request. */
	IExec->WaitIO((struct IORequest *)timer->timerrequest);
}

/*
 * Wait until specified time using local timer instance.
 *
 * ticks: the time in SDL time to wait until.
 *
 * Returns: FALSE if the timer was interrupted, TRUE otherwise.
 */
BOOL os4timer_WaitUntil(Uint32 ticks)
{
	ULONG alarmSig;
	ULONG sigsReceived;

	os4timer_Instance *timer = (os4timer_Instance *)GetTimerInstance();

	os4timer_SetAlarm(timer, ticks, &alarmSig);

	sigsReceived = IExec->Wait(alarmSig | SIGBREAKF_CTRL_C);

	os4timer_ClearAlarm(timer);

	return (sigsReceived & alarmSig) == alarmSig;
}
