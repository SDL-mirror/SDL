/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include <windows.h>
#include <mmsystem.h>

#include "SDL_timer.h"
#include "SDL_timer_c.h"
#include "SDL_error.h"

#ifdef _WIN32_WCE
#define USE_GETTICKCOUNT
#define USE_SETTIMER
#endif

#define TIME_WRAP_VALUE	(~(DWORD)0)

/* The first (low-resolution) ticks value of the application */
static DWORD start;

#ifndef USE_GETTICKCOUNT
/* Store if a high-resolution performance counter exists on the system */
static BOOL hires_timer_available;
/* The first high-resolution ticks value of the application */
static LARGE_INTEGER hires_start_ticks;
/* The number of ticks per second of the high-resolution performance counter */
static LARGE_INTEGER hires_ticks_per_second;
#endif

void SDL_StartTicks(void)
{
	/* Set first ticks value */
#ifdef USE_GETTICKCOUNT
	start = GetTickCount();
#else
#if 0 /* Apparently there are problems with QPC on Win2K */
	if (QueryPerformanceFrequency(&hires_ticks_per_second) == TRUE)
	{
		hires_timer_available = TRUE;
		QueryPerformanceCounter(&hires_start_ticks);
	}
	else
#endif
	{
		hires_timer_available = FALSE;
		timeBeginPeriod(1);		/* use 1 ms timer precision */
		start = timeGetTime();
	}
#endif
}

Uint32 SDL_GetTicks(void)
{
	DWORD now, ticks;
#ifndef USE_GETTICKCOUNT
	LARGE_INTEGER hires_now;
#endif

#ifdef USE_GETTICKCOUNT
	now = GetTickCount();
#else
	if (hires_timer_available)
	{
		QueryPerformanceCounter(&hires_now);

		hires_now.QuadPart -= hires_start_ticks.QuadPart;
		hires_now.QuadPart *= 1000;
		hires_now.QuadPart /= hires_ticks_per_second.QuadPart;

		return (DWORD)hires_now.QuadPart;
	}
	else
	{
		now = timeGetTime();
	}
#endif

	if ( now < start ) {
		ticks = (TIME_WRAP_VALUE-start) + now;
	} else {
		ticks = (now - start);
	}
	return(ticks);
}

void SDL_Delay(Uint32 ms)
{
	Sleep(ms);
}

#ifdef USE_SETTIMER

static UINT WIN_timer;

int SDL_SYS_TimerInit(void)
{
	return(0);
}

void SDL_SYS_TimerQuit(void)
{
	return;
}

/* Forward declaration because this is called by the timer callback */
int SDL_SYS_StartTimer(void);

static VOID CALLBACK TimerCallbackProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	Uint32 ms;

	ms = SDL_alarm_callback(SDL_alarm_interval);
	if ( ms != SDL_alarm_interval ) {
		KillTimer(NULL, idEvent);
		if ( ms ) {
			SDL_alarm_interval = ROUND_RESOLUTION(ms);
			SDL_SYS_StartTimer();
		} else {
			SDL_alarm_interval = 0;
		}
	}
}

int SDL_SYS_StartTimer(void)
{
	int retval;

	WIN_timer = SetTimer(NULL, 0, SDL_alarm_interval, TimerCallbackProc);
	if ( WIN_timer ) {
		retval = 0;
	} else {
		retval = -1;
	}
	return retval;
}

void SDL_SYS_StopTimer(void)
{
	if ( WIN_timer ) {
		KillTimer(NULL, WIN_timer);
		WIN_timer = 0;
	}
}

#else /* !USE_SETTIMER */

/* Data to handle a single periodic alarm */
static UINT timerID = 0;

static void CALLBACK HandleAlarm(UINT uID,  UINT uMsg, DWORD dwUser,
						DWORD dw1, DWORD dw2)
{
	SDL_ThreadedTimerCheck();
}


int SDL_SYS_TimerInit(void)
{
	MMRESULT result;

	/* Set timer resolution */
	result = timeBeginPeriod(TIMER_RESOLUTION);
	if ( result != TIMERR_NOERROR ) {
		SDL_SetError("Warning: Can't set %d ms timer resolution",
							TIMER_RESOLUTION);
	}
	/* Allow 10 ms of drift so we don't chew on CPU */
	timerID = timeSetEvent(TIMER_RESOLUTION,1,HandleAlarm,0,TIME_PERIODIC);
	if ( ! timerID ) {
		SDL_SetError("timeSetEvent() failed");
		return(-1);
	}
	return(SDL_SetTimerThreaded(1));
}

void SDL_SYS_TimerQuit(void)
{
	if ( timerID ) {
		timeKillEvent(timerID);
	}
	timeEndPeriod(TIMER_RESOLUTION);
}

int SDL_SYS_StartTimer(void)
{
	SDL_SetError("Internal logic error: Win32 uses threaded timer");
	return(-1);
}

void SDL_SYS_StopTimer(void)
{
	return;
}

#endif /* USE_SETTIMER */
