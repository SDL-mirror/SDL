/*
 *	GNU pth mutexes
 *
 *	Patrice Mandin
 */

#include <stdio.h>
#include <stdlib.h>
#include <pth.h>

#include "SDL_error.h"
#include "SDL_mutex.h"
#include "SDL_sysmutex_c.h"

/* Create a mutex */
SDL_mutex *SDL_CreateMutex(void)
{
	SDL_mutex *mutex;

	/* Allocate mutex memory */
	mutex = (SDL_mutex *)malloc(sizeof(*mutex));
	if ( mutex ) {
		/* Create the mutex, with initial value signaled */
	    if (!pth_mutex_init(&(mutex->mutexpth_p))) {
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

	pth_mutex_acquire(&(mutex->mutexpth_p), FALSE, NULL);

	return(0);
}

/* Unlock the mutex */
int SDL_mutexV(SDL_mutex *mutex)
{
	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}

    pth_mutex_release(&(mutex->mutexpth_p));

	return(0);
}
