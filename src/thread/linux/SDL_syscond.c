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

#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_sysmutex_c.h"


#if defined(PTHREAD_NO_RECURSIVE_MUTEX) && !defined(__bsdi__)
#error You need to use the generic condition variable implementation
#endif

struct SDL_cond
{
	pthread_cond_t cond;
};

/* Create a condition variable */
SDL_cond * SDL_CreateCond(void)
{
	SDL_cond *cond;

	cond = (SDL_cond *) malloc(sizeof(SDL_cond));
	if ( cond ) {
		if ( pthread_cond_init(&cond->cond, NULL) < 0 ) {
			SDL_SetError("pthread_cond_init() failed");
			free(cond);
			cond = NULL;
		}
	}
	return(cond);
}

/* Destroy a condition variable */
void SDL_DestroyCond(SDL_cond *cond)
{
	if ( cond ) {
		pthread_cond_destroy(&cond->cond);
		free(cond);
	}
}

/* Restart one of the threads that are waiting on the condition variable */
int SDL_CondSignal(SDL_cond *cond)
{
	int retval;

	if ( ! cond ) {
		SDL_SetError("Passed a NULL condition variable");
		return -1;
	}

	retval = 0;
	if ( pthread_cond_signal(&cond->cond) != 0 ) {
		SDL_SetError("pthread_cond_signal() failed");
		retval = -1;
	}
	return retval;
}

/* Restart all threads that are waiting on the condition variable */
int SDL_CondBroadcast(SDL_cond *cond)
{
	int retval;

	if ( ! cond ) {
		SDL_SetError("Passed a NULL condition variable");
		return -1;
	}

	retval = 0;
	if ( pthread_cond_broadcast(&cond->cond) != 0 ) {
		SDL_SetError("pthread_cond_broadcast() failed");
		retval = -1;
	}
	return retval;
}

int SDL_CondWaitTimeout(SDL_cond *cond, SDL_mutex *mutex, Uint32 ms)
{
	int retval;
	struct timeval delta;
	struct timespec abstime;

	if ( ! cond ) {
		SDL_SetError("Passed a NULL condition variable");
		return -1;
	}

	gettimeofday(&delta, NULL);

	abstime.tv_sec = delta.tv_sec + (ms/1000);
	abstime.tv_nsec = (delta.tv_usec + (ms%1000) * 1000) * 1000;
        if ( abstime.tv_nsec > 1000000000 ) {
          abstime.tv_sec += 1;
          abstime.tv_nsec -= 1000000000;
        }

  tryagain:
	retval = pthread_cond_timedwait(&cond->cond, &mutex->id, &abstime);
	switch (retval) {
	    case EINTR:
		goto tryagain;
		break;
	    case ETIMEDOUT:
		retval = SDL_MUTEX_TIMEDOUT;
		break;
	    case 0:
		break;
	    default:
		SDL_SetError("pthread_cond_timedwait() failed");
		retval = -1;
		break;
	}
	return retval;
}

/* Wait on the condition variable, unlocking the provided mutex.
   The mutex must be locked before entering this function!
 */
int SDL_CondWait(SDL_cond *cond, SDL_mutex *mutex)
{
	int retval;

	if ( ! cond ) {
		SDL_SetError("Passed a NULL condition variable");
		return -1;
	}

	retval = 0;
	if ( pthread_cond_wait(&cond->cond, &mutex->id) != 0 ) {
		SDL_SetError("pthread_cond_wait() failed");
		retval = -1;
	}
	return retval;
}

#else /* Use semaphore implementation */

#include "generic/SDL_syscond.c"

#endif /* SDL_USE_PTHREADS */
