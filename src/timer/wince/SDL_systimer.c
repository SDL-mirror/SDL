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
#include "SDL_config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "SDL_timer.h"
#include "../SDL_timer_c.h"

static Uint64 start_date;
static Uint64 start_ticks;

static Uint64 wce_ticks(void)
{
  return((Uint64)GetTickCount());
}

static Uint64 wce_date(void)
{
  union
  {
	FILETIME ftime;
	Uint64 itime;
  } ftime;
  SYSTEMTIME stime;

  GetSystemTime(&stime);
  SystemTimeToFileTime(&stime,&ftime.ftime);
  ftime.itime/=10000; // Convert 100ns intervals to 1ms intervals
  // Remove ms portion, which can't be relied on
  ftime.itime -= (ftime.itime % 1000);
  return(ftime.itime);
}

static Sint32 wce_rel_ticks(void)
{
  return((Sint32)(wce_ticks()-start_ticks));
}

static Sint32 wce_rel_date(void)
{
  return((Sint32)(wce_date()-start_date));
}

/* Return time in ms relative to when SDL was started */
Uint32 SDL_GetTicks()
{
  Sint32 offset=wce_rel_date()-wce_rel_ticks();
  if((offset < -1000) || (offset > 1000))
  {
//    fprintf(stderr,"Time desync(%+d), resyncing\n",offset/1000);
	start_ticks-=offset;
  }

  return((Uint32)wce_rel_ticks());
}

/* Give up approx. givem milliseconds to the OS. */
void SDL_Delay(Uint32 ms)
{
  Sleep(ms);
}

/* Recard start-time of application for reference */
void SDL_StartTicks(void)
{
  start_date=wce_date();
  start_ticks=wce_ticks();
}

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

