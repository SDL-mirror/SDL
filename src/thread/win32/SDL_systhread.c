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

/* Win32 thread management routines for SDL */

#include "SDL_windows.h"

#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_stdlib.h"
#include "SDL_systhread.h"

typedef struct ThreadStartParms
{
  void *args;
  pfnSDL_CurrentEndThread pfnCurrentEndThread;
} tThreadStartParms, *pThreadStartParms;

static unsigned __stdcall RunThread(void *data)
{
  pThreadStartParms pThreadParms = (pThreadStartParms)data;
  pfnSDL_CurrentEndThread pfnCurrentEndThread = NULL;

  // Call the thread function!
  SDL_RunThread(pThreadParms->args);

  // Get the current endthread we have to use!
  if (pThreadParms)
  {
    pfnCurrentEndThread = pThreadParms->pfnCurrentEndThread;
    free(pThreadParms);
  }
  // Call endthread!
  if (pfnCurrentEndThread)
    (*pfnCurrentEndThread)(0);
  return(0);
}

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args, pfnSDL_CurrentBeginThread pfnBeginThread, pfnSDL_CurrentEndThread pfnEndThread)
{
	unsigned threadid;
    pThreadStartParms pThreadParms = (pThreadStartParms)malloc(sizeof(tThreadStartParms));
    if (!pThreadParms) {
		SDL_OutOfMemory();
        return(-1);
    }

    // Save the function which we will have to call to clear the RTL of calling app!
    pThreadParms->pfnCurrentEndThread = pfnEndThread;
    // Also save the real parameters we have to pass to thread function
    pThreadParms->args = args;

	if (pfnBeginThread) {
		thread->handle = (SYS_ThreadHandle) pfnBeginThread(NULL, 0, RunThread,
				pThreadParms, 0, &threadid);
	} else {
		thread->handle = CreateThread(NULL, 0, RunThread, pThreadParms, 0, &threadid);
	}
	if (thread->handle == NULL) {
		SDL_SetError("Not enough resources to create thread");
		return(-1);
	}
	return(0);
}

void SDL_SYS_SetupThread(void)
{
	return;
}

Uint32 SDL_ThreadID(void)
{
	return((Uint32)GetCurrentThreadId());
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
	WaitForSingleObject(thread->handle, INFINITE);
	CloseHandle(thread->handle);
}

/* WARNING: This function is really a last resort.
 * Threads should be signaled and then exit by themselves.
 * TerminateThread() doesn't perform stack and DLL cleanup.
 */
void SDL_SYS_KillThread(SDL_Thread *thread)
{
	TerminateThread(thread->handle, FALSE);
}
