/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

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

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "SDL_error.h"
#include "SDL_timer.h"
#include "SDL_timer_c.h"

#if _POSIX_THREAD_SYSCALL_SOFT
#include <pthread.h>
#endif

#if defined(DISABLE_THREADS) || defined(FORK_HACK)
#define USE_ITIMER
#endif

/* The following defines should really be determined at configure time */

#if defined(linux)
/* Linux select() changes its timeout parameter upon return to contain
   the remaining time. Most other unixen leave it unchanged or undefined. */
#define SELECT_SETS_REMAINING
#elif defined(__bsdi__) || defined(__FreeBSD__) || defined(__sun)
#define USE_NANOSLEEP
#endif


/* The first ticks value of the application */
static struct timeval start;

void SDL_StartTicks(void)
{
	/* Set first ticks value */
	gettimeofday(&start, NULL);
}

Uint32 SDL_GetTicks (void)
{
	struct timeval now;
	Uint32 ticks;

	gettimeofday(&now, NULL);
	ticks=(now.tv_sec-start.tv_sec)*1000+(now.tv_usec-start.tv_usec)/1000;
	return(ticks);
}

void SDL_Delay (Uint32 ms)
{
	int was_error;

#ifdef USE_NANOSLEEP
	struct timespec elapsed, tv;
#else
	struct timeval tv;
#ifndef SELECT_SETS_REMAINING
	Uint32 then, now, elapsed;
#endif
#endif

	/* Set the timeout interval - Linux only needs to do this once */
#ifdef SELECT_SETS_REMAINING
	tv.tv_sec = ms/1000;
	tv.tv_usec = (ms%1000)*1000;
#elif defined(USE_NANOSLEEP)
	elapsed.tv_sec = ms/1000;
	elapsed.tv_nsec = (ms%1000)*1000000;
#else
	then = SDL_GetTicks();
#endif
	do {
		errno = 0;

#if _POSIX_THREAD_SYSCALL_SOFT
		pthread_yield_np();
#endif
#ifdef USE_NANOSLEEP
		tv.tv_sec = elapsed.tv_sec;
		tv.tv_nsec = elapsed.tv_nsec;
		was_error = nanosleep(&tv, &elapsed);
#else
#ifndef SELECT_SETS_REMAINING
		/* Calculate the time interval left (in case of interrupt) */
		now = SDL_GetTicks();
		elapsed = (now-then);
		then = now;
		if ( elapsed >= ms ) {
			break;
		}
		ms -= elapsed;
		tv.tv_sec = ms/1000;
		tv.tv_usec = (ms%1000)*1000;
#endif
		was_error = select(0, NULL, NULL, NULL, &tv);
#endif /* USE_NANOSLEEP */
	} while ( was_error && (errno == EINTR) );
}

#ifdef USE_ITIMER

static void HandleAlarm(int sig)
{
	Uint32 ms;

	if ( SDL_alarm_callback ) {
		ms = (*SDL_alarm_callback)(SDL_alarm_interval);
		if ( ms != SDL_alarm_interval ) {
			SDL_SetTimer(ms, SDL_alarm_callback);
		}
	}
}

int SDL_SYS_TimerInit(void)
{
	struct sigaction action;

	/* Set the alarm handler (Linux specific) */
	memset(&action, 0, sizeof(action));
	action.sa_handler = HandleAlarm;
	action.sa_flags = SA_RESTART;
	sigemptyset(&action.sa_mask);
	sigaction(SIGALRM, &action, NULL);
	return(0);
}

void SDL_SYS_TimerQuit(void)
{
	SDL_SetTimer(0, NULL);
}

int SDL_SYS_StartTimer(void)
{
	struct itimerval timer;

	timer.it_value.tv_sec = (SDL_alarm_interval/1000);
	timer.it_value.tv_usec = (SDL_alarm_interval%1000)*1000;
	timer.it_interval.tv_sec = (SDL_alarm_interval/1000);
	timer.it_interval.tv_usec = (SDL_alarm_interval%1000)*1000;
	setitimer(ITIMER_REAL, &timer, NULL);
	return(0);
}

void SDL_SYS_StopTimer(void)
{
	struct itimerval timer;

	memset(&timer, 0, (sizeof timer));
	setitimer(ITIMER_REAL, &timer, NULL);
}

#else /* USE_ITIMER */

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
	SDL_SetError("Internal logic error: Linux uses threaded timer");
	return(-1);
}

void SDL_SYS_StopTimer(void)
{
	return;
}

#endif /* USE_ITIMER */
