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

#ifdef linux
/* Look to see if glibc is available, and if so, what version */
#include <features.h>

#if (__GLIBC__ == 2) && (__GLIBC_MINOR__ == 0)
#warning Working around a bug in glibc 2.0 pthreads
#undef SDL_USE_PTHREADS
/* The bug is actually a problem where threads are suspended, but don't
   wake up when the thread manager sends them a signal.  This is a problem
   with thread creation too, but it happens less often. :-/
   We avoid this by using System V IPC for mutexes.
 */
#endif /* glibc 2.0 */
#endif /* linux */

#ifdef SDL_USE_PTHREADS

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "SDL_error.h"
#include "SDL_thread.h"


struct SDL_mutex {
	pthread_mutex_t id;
#ifdef PTHREAD_NO_RECURSIVE_MUTEX
	int recursive;
	pthread_t owner;
#endif
};

SDL_mutex *SDL_CreateMutex (void)
{
	SDL_mutex *mutex;
	pthread_mutexattr_t attr;

	/* Allocate the structure */
	mutex = (SDL_mutex *)calloc(1, sizeof(*mutex));
	if ( mutex ) {
		pthread_mutexattr_init(&attr);
#ifdef PTHREAD_NO_RECURSIVE_MUTEX
		/* No extra attributes necessary */
#else
#ifdef linux
		pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#else
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#endif
#endif /* PTHREAD_NO_RECURSIVE_MUTEX */
		if ( pthread_mutex_init(&mutex->id, &attr) != 0 ) {
			SDL_SetError("pthread_mutex_init() failed");
			free(mutex);
			mutex = NULL;
		}
	} else {
		SDL_OutOfMemory();
	}
	return(mutex);
}

void SDL_DestroyMutex(SDL_mutex *mutex)
{
	if ( mutex ) {
		pthread_mutex_destroy(&mutex->id);
		free(mutex);
	}
}

/* Lock the mutex */
int SDL_mutexP(SDL_mutex *mutex)
{
	int retval;
#ifdef PTHREAD_NO_RECURSIVE_MUTEX
	pthread_t this_thread;
#endif

	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}

	retval = 0;
#ifdef PTHREAD_NO_RECURSIVE_MUTEX
	this_thread = pthread_self();
	if ( mutex->owner == this_thread ) {
		++mutex->recursive;
	} else {
		/* The order of operations is important.
		   We set the locking thread id after we obtain the lock
		   so unlocks from other threads will fail.
		*/
		if ( pthread_mutex_lock(&mutex->id) == 0 ) {
			mutex->owner = this_thread;
			mutex->recursive = 0;
		} else {
			SDL_SetError("pthread_mutex_lock() failed");
			retval = -1;
		}
	}
#else
	if ( pthread_mutex_lock(&mutex->id) < 0 ) {
		SDL_SetError("pthread_mutex_lock() failed");
		retval = -1;
	}
#endif
	return retval;
}

int SDL_mutexV(SDL_mutex *mutex)
{
	int retval;

	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}

	retval = 0;
#ifdef PTHREAD_NO_RECURSIVE_MUTEX
	/* We can only unlock the mutex if we own it */
	if ( pthread_self() == mutex->owner ) {
		if ( mutex->recursive ) {
			--mutex->recursive;
		} else {
			/* The order of operations is important.
			   First reset the owner so another thread doesn't lock
			   the mutex and set the ownership before we reset it,
			   then release the lock semaphore.
			 */
			mutex->owner = 0;
			pthread_mutex_unlock(&mutex->id);
		}
	} else {
		SDL_SetError("mutex not owned by this thread");
		retval = -1;
	}

#else
	if ( pthread_mutex_unlock(&mutex->id) < 0 ) {
		SDL_SetError("pthread_mutex_unlock() failed");
		retval = -1;
	}
#endif /* PTHREAD_NO_RECURSIVE_MUTEX */

	return retval;
}

#else /* Use semaphore implementation */

#include "generic/SDL_sysmutex.c"

#endif /* SDL_USE_PTHREADS */
