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

/* Mutex functions using the Win32 API */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "SDL_error.h"
#include "SDL_mutex.h"


struct SDL_mutex {
	HANDLE id;
};

/* Create a mutex */
SDL_mutex *SDL_CreateMutex(void)
{
	SDL_mutex *mutex;

	/* Allocate mutex memory */
	mutex = (SDL_mutex *)malloc(sizeof(*mutex));
	if ( mutex ) {
		/* Create the mutex, with initial value signaled */
		mutex->id = CreateMutex(NULL, FALSE, NULL);
		if ( ! mutex->id ) {
			SDL_SetError("Couldn't create mutex");
			free(mutex);
			mutex = NULL;
		}
	} else {
		SDL_OutOfMemory();
	}
	return(mutex);
}

/* Free the mutex */
void SDL_DestroyMutex(SDL_mutex *mutex)
{
	if ( mutex ) {
		if ( mutex->id ) {
			CloseHandle(mutex->id);
			mutex->id = 0;
		}
		free(mutex);
	}
}

/* Lock the mutex */
int SDL_mutexP(SDL_mutex *mutex)
{
	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}
	if ( WaitForSingleObject(mutex->id, INFINITE) == WAIT_FAILED ) {
		SDL_SetError("Couldn't wait on mutex");
		return -1;
	}
	return(0);
}

/* Unlock the mutex */
int SDL_mutexV(SDL_mutex *mutex)
{
	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}
	if ( ReleaseMutex(mutex->id) == FALSE ) {
		SDL_SetError("Couldn't release mutex");
		return -1;
	}
	return(0);
}
