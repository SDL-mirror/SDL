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

/* Timer start/reset time */
static Uint32 timerStart;
/* Timer running function */
void RISCOS_CheckTimer();

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

extern void DRenderer_FillBuffers();

void SDL_Delay (Uint32 ms)
{
    Uint32 now,then,elapsed;

	/* Set the timeout interval - Linux only needs to do this once */
	then = SDL_GetTicks();

	do {
		/* Do background tasks required while sleeping as we are not multithreaded */
		RISCOS_BackgroundTasks();
		/* Calculate the time interval left (in case of interrupt) */
		now = SDL_GetTicks();
		elapsed = (now-then);
		then = now;
		if ( elapsed >= ms ) {
			break;
		}
		ms -= elapsed;

	} while ( 1 );
}

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
