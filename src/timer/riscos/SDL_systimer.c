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
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "SDL_error.h"
#include "SDL_timer.h"
#include "SDL_timer_c.h"

#ifdef DISABLE_THREADS
/* Timer start/reset time */
static Uint32 timerStart;
/* Timer running function */
void RISCOS_CheckTimer();
#else
#include <pthread.h>
extern Uint32 riscos_main_thread;
extern int riscos_using_threads;
extern Uint32 SDL_ThreadID();
extern Uint32 SDL_EventThreadID(void);
#endif


extern void RISCOS_BackgroundTasks(void);

/* The first ticks value of the application */
clock_t start;

void SDL_StartTicks(void)
{
	/* Set first ticks value */
	start = clock();
}

Uint32 SDL_GetTicks (void)
{
	clock_t ticks;

	ticks=clock()-start;


#if CLOCKS_PER_SEC == 1000

	return(ticks);

#elif CLOCKS_PER_SEC == 100

	return (ticks * 10);

#else

	return ticks*(1000/CLOCKS_PER_SEC);

#endif

}

void SDL_Delay (Uint32 ms)
{
    Uint32 now,then,elapsed;
#ifndef DISABLE_THREADS
    int is_event_thread;
    if (riscos_using_threads)
    {
       is_event_thread = 0;
       if (SDL_EventThreadID())
       {
          if (SDL_EventThreadID() == SDL_ThreadID()) is_event_thread = 1;
       } else if (SDL_ThreadID() == riscos_main_thread) is_event_thread = 1;
    } else is_event_thread = 1;
#endif

/*TODO: Next version of unixlib may allow us to use usleep here */
/*      for non event threads */

	/* Set the timeout interval - Linux only needs to do this once */
	then = SDL_GetTicks();

	do {
		/* Do background tasks required while sleeping as we are not multithreaded */
#ifdef DISABLE_THREADS
		RISCOS_BackgroundTasks();
#else
		/* For threaded build only run background tasks in event thread */
		if (is_event_thread) RISCOS_BackgroundTasks();
#endif

		/* Calculate the time interval left (in case of interrupt) */
		now = SDL_GetTicks();
		elapsed = (now-then);
		then = now;
		if ( elapsed >= ms ) {
			break;
		}
		ms -= elapsed;
#ifndef DISABLE_THREADS
            /* Need to yield to let other threads have a go */
            if (riscos_using_threads) pthread_yield();
#endif

	} while ( 1 );
}

#ifdef DISABLE_THREADS

/* Non-threaded version of timer */

int SDL_SYS_TimerInit(void)
{
	return(0);
}

void SDL_SYS_TimerQuit(void)
{
	SDL_SetTimer(0, NULL);
}

int SDL_SYS_StartTimer(void)
{
	timerStart = SDL_GetTicks();

	return(0);
}

void SDL_SYS_StopTimer(void)
{
	/* Don't need to do anything as we use SDL_timer_running
	   to detect if we need to check the timer */
}


void RISCOS_CheckTimer()
{
	if (SDL_timer_running && SDL_GetTicks() - timerStart >= SDL_alarm_interval)
	{
		Uint32 ms;

		ms = SDL_alarm_callback(SDL_alarm_interval);
		if ( ms != SDL_alarm_interval )
		{
			if ( ms )
			{
				SDL_alarm_interval = ROUND_RESOLUTION(ms);
			} else
			{
				SDL_alarm_interval = 0;
				SDL_timer_running = 0;
			}
		}
		if (SDL_alarm_interval) timerStart = SDL_GetTicks();
	}
}

#else

/* Threaded version of timer - based on code for linux */

#include "SDL_thread.h"

/* Data to handle a single periodic alarm */
static int timer_alive = 0;
static SDL_Thread *timer = NULL;

static int RunTimer(void *unused)
{
	while ( timer_alive ) {
		if ( SDL_timer_running ) {
			SDL_ThreadedTimerCheck();
		}
		SDL_Delay(1);
	}
	return(0);
}

/* This is only called if the event thread is not running */
int SDL_SYS_TimerInit(void)
{
	timer_alive = 1;
	timer = SDL_CreateThread(RunTimer, NULL);
	if ( timer == NULL )
		return(-1);
	return(SDL_SetTimerThreaded(1));
}

void SDL_SYS_TimerQuit(void)
{
	timer_alive = 0;
	if ( timer ) {
		SDL_WaitThread(timer, NULL);
		timer = NULL;
	}
}

int SDL_SYS_StartTimer(void)
{
	SDL_SetError("Internal logic error: RISCOS uses threaded timer");
	return(-1);
}

void SDL_SYS_StopTimer(void)
{
	return;
}

#endif /* DISABLE_THREADS */
