/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "SDL_error.h"
#include "SDL_timer.h"
#include "SDL_timer_c.h"

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
#ifndef linux	/* Non-Linux implementations need to calculate time left */
	Uint32 then, now, elapsed;
#endif
	struct timeval tv;

	/* Set the timeout interval - Linux only needs to do this once */
#ifdef linux
	tv.tv_sec = ms/1000;
	tv.tv_usec = (ms%1000)*1000;
#else
	then = SDL_GetTicks();
#endif
	do {
		errno = 0;
#ifndef linux
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
	} while ( was_error && (errno == EINTR) );
}

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
#ifdef NO_AMIGADEBUG
	fprintf(stderr,"Creo il thread per il timer (NOITMER)...\n");
#endif
	timer_alive = 1;
	timer = SDL_CreateThread(RunTimer, NULL);
	if ( timer == NULL )
	{
#ifdef NO_AMIGADEBUG
		fprintf(stderr,"Creazione del thread fallita...\n");
#endif

		return(-1);
	}
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
