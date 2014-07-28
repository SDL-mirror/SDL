/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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
#include "SDL_config.h"

#ifdef SDL_TIMER_MINT

/*
 *	TOS/MiNT timer driver
 *	based on RISCOS backend
 *
 *	Patrice Mandin
 */

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <mint/cookie.h>
#include <mint/sysvars.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>

#include "SDL_timer.h"
#include "../SDL_timer_c.h"

#include "../../video/ataricommon/SDL_atarisuper.h"

/* from src/video/ataricommon/SDL_atarievents.c */
void SDL_AtariMint_BackgroundTasks(void);

static Uint32 readHz200Timer(void);

/* The first ticks value of the application */
static Uint32 start;

static int mint_present; /* can we use Syield() ? */

/* Timer  SDL_arraysize(Timer ),start/reset time */
static Uint32 timerStart;

void SDL_StartTicks(void)
{
	long dummy;

	/* Set first ticks value, one _hz_200 tic is 5ms */
	start = readHz200Timer() * 5;

	mint_present = (Getcookie(C_MiNT, &dummy) == C_FOUND);
}

Uint32 SDL_GetTicks (void)
{
	Uint32 now = readHz200Timer() * 5;

	return(now-start);
}

void SDL_Delay (Uint32 ms)
{
	Uint32 now;

	/* No need to loop for delay below resolution */
	if (ms<5) {
		SDL_AtariMint_BackgroundTasks();
		if (mint_present) {
			Syield();
		}
		return;
	}

	now = SDL_GetTicks();
	while ((SDL_GetTicks()-now)<ms){
		SDL_AtariMint_BackgroundTasks();
		if (mint_present) {
			Syield();
		}
	}
}

static Uint32 readHz200Timer(void)
{
	void *old_stack;
	Uint32 now;

	old_stack = (void *)Super(0);
	now = *((volatile long *)_hz_200);
	SuperToUser(old_stack);

	return now;
}

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


void SDL_AtariMint_CheckTimer(void)
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

#endif /* SDL_TIMER_MINT */
