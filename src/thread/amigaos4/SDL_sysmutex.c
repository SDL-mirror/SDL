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


#include <stdio.h>
#include <stdlib.h>

#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_systhread_c.h"

#include <proto/exec.h>
#include <exec/semaphores.h>

struct SDL_mutex {
	struct SignalSemaphore *sem;
};

/* Create a mutex */
SDL_mutex *SDL_CreateMutex(void)
{
	SDL_mutex *mutex;

	/* Allocate mutex memory */
	mutex = (SDL_mutex *)SDL_malloc(sizeof(*mutex));
	if ( mutex )
	{
		/* Create the mutex semaphore, with initial value 1 */
		mutex->sem = (struct SignalSemaphore *)IExec->AllocSysObjectTags(ASOT_SEMAPHORE,
														ASO_NoTrack,		TRUE,
														TAG_DONE);
		if (!mutex->sem)
		{
			SDL_free(mutex);
			mutex = NULL;
		}
	}
	else
	{
		SDL_OutOfMemory();
	}

	return mutex;
}

/* Free the mutex */
void SDL_DestroyMutex(SDL_mutex *mutex)
{
	if (mutex)
	{
		if (mutex->sem)
		{
			IExec->FreeSysObject(ASOT_SEMAPHORE, mutex->sem);
		}
		SDL_free(mutex);
	}
}

/* Lock the semaphore */
int SDL_mutexP(SDL_mutex *mutex)
{
	if (mutex == NULL)
	{
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}

	IExec->ObtainSemaphore(mutex->sem);

	return 0;
}

/* Unlock the mutex */
int SDL_mutexV(SDL_mutex *mutex)
{
	if (mutex == NULL)
	{
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}

	IExec->ReleaseSemaphore(mutex->sem);

	return 0;
}
