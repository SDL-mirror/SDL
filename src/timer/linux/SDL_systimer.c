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

    RDTSC stuff by lompik (lompik@voila.fr) 20/03/2002 
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
#ifdef ENABLE_PTH
#include <pth.h>
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

#if defined(i386) || defined(__i386__)
/* This only works on pentium or newer x86 processors */
/* Actually, this isn't reliable on multi-cpu systems, so is disabled */
/*#define USE_RDTSC*/
#endif


#ifdef USE_RDTSC 

/* The first ticks value of the application */
static unsigned long long start;
static float cpu_mhz1000 = 0.0f;

#if 1
/* This is for old binutils version that don't recognize rdtsc mnemonics.
   But all binutils version supports this.
*/
#define rdtsc(t) asm __volatile__ (".byte 0x0f, 0x31; " : "=A" (t))
#else
#define rdtsc(t) asm __volatile__ ("rdtsc" : "=A" (t))
#endif

static float calc_cpu_mhz(void)
{
	float cpu_mhz;
	unsigned long long tsc_start;
	unsigned long long tsc_end;
	struct timeval tv_start, tv_end;
	long usec_delay;

	rdtsc(tsc_start);
	gettimeofday(&tv_start, NULL);
	sleep(1);
	rdtsc(tsc_end);
	gettimeofday(&tv_end, NULL);
	usec_delay = 1000000L * (tv_end.tv_sec - tv_start.tv_sec) +
	                        (tv_end.tv_usec - tv_start.tv_usec);
	cpu_mhz = (float)(tsc_end-tsc_start) / usec_delay;
#if 0
	printf("cpu MHz\t\t: %.3f\n", cpu_mhz);
#endif
	return cpu_mhz;
}

#else

/* The first ticks value of the application */
static struct timeval start;

#endif  /* USE_RDTSC */


void SDL_StartTicks(void)
{
	/* Set first ticks value */
#ifdef USE_RDTSC
	if ( ! cpu_mhz1000 ) {
		cpu_mhz1000 = calc_cpu_mhz() * 1000.0f;
	}
	rdtsc(start);
#else
	gettimeofday(&start, NULL);
#endif /* USE_RDTSC */
}

Uint32 SDL_GetTicks (void)
{
#ifdef USE_RDTSC 
	unsigned long long now;
	if ( ! cpu_mhz1000 ) {
		return 0; /* Shouldn't happen. BUG!! */
	}
	rdtsc(now);
	return (Uint32)((now-start)/cpu_mhz1000);
#else
	struct timeval now;
	Uint32 ticks;

	gettimeofday(&now, NULL);
	ticks=(now.tv_sec-start.tv_sec)*1000+(now.tv_usec-start.tv_usec)/1000;
	return(ticks);
#endif /* USE_RDTSC */
}

void SDL_Delay (Uint32 ms)
{
#ifdef ENABLE_PTH
	pth_time_t tv;
	tv.tv_sec  =  ms/1000;
	tv.tv_usec = (ms%1000)*1000;
	pth_nap(tv);
#else
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
#endif /* ENABLE_PTH */
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
