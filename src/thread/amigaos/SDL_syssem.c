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

/* An implementation of semaphores using mutexes and condition variables */

#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_systhread_c.h"


struct SDL_semaphore
{
	struct SignalSemaphore Sem;
	Uint32 count;
	Uint32 waiters_count;
	SDL_mutex *count_lock;
	SDL_cond *count_nonzero;
};

#undef D

#define D(x)

SDL_sem *SDL_CreateSemaphore(Uint32 initial_value)
{
	SDL_sem *sem;

	sem = (SDL_sem *)malloc(sizeof(*sem));

	if ( ! sem ) {
		SDL_OutOfMemory();
		return(0);
	}

	D(bug("Creating semaphore %lx...\n",sem));

	memset(sem,0,sizeof(*sem));

	InitSemaphore(&sem->Sem);

	return(sem);
}

void SDL_DestroySemaphore(SDL_sem *sem)
{
	D(bug("Destroying semaphore %lx...\n",sem));

	if ( sem ) {
// Condizioni per liberare i task in attesa?
		free(sem);
	}
}

int SDL_SemTryWait(SDL_sem *sem)
{
	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	D(bug("TryWait semaphore...%lx\n",sem));

	ObtainSemaphore(&sem->Sem);
//	ReleaseSemaphore(&sem->Sem);

	return 1;
}

int SDL_SemWaitTimeout(SDL_sem *sem, Uint32 timeout)
{
	int retval;


	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	D(bug("WaitTimeout (%ld) semaphore...%lx\n",timeout,sem));

	/* A timeout of 0 is an easy case */
	if ( timeout == 0 ) {
		return SDL_SemTryWait(sem);
	}
/*
	SDL_LockMutex(sem->count_lock);
	++sem->waiters_count;
	retval = 0;
	while ( (sem->count == 0) && (retval != SDL_MUTEX_TIMEDOUT) ) {
		retval = SDL_CondWaitTimeout(sem->count_nonzero,
		                             sem->count_lock, timeout);
	}
	--sem->waiters_count;
	--sem->count;
	SDL_UnlockMutex(sem->count_lock);
*/
	if(!(retval=AttemptSemaphore(&sem->Sem)))
	{
		SDL_Delay(timeout);
		retval=AttemptSemaphore(&sem->Sem);
	}

	if(retval==TRUE)
	{
//		ReleaseSemaphore(&sem->Sem);
		retval=1;
	}

	return retval;
}

int SDL_SemWait(SDL_sem *sem)
{
	ObtainSemaphore(&sem->Sem);
	return 0;
//	return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
}

Uint32 SDL_SemValue(SDL_sem *sem)
{
	Uint32 value;

	value = 0;
	if ( sem ) {
		#ifdef STORMC4_WOS
		value = sem->Sem.ssppc_SS.ss_NestCount;
		#else
		value = sem->Sem.ss_NestCount;
		#endif
//		SDL_UnlockMutex(sem->count_lock);
	}
	return value;
}

int SDL_SemPost(SDL_sem *sem)
{
	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}
	D(bug("SemPost semaphore...%lx\n",sem));

	ReleaseSemaphore(&sem->Sem);
#if 0
	SDL_LockMutex(sem->count_lock);
	if ( sem->waiters_count > 0 ) {
		SDL_CondSignal(sem->count_nonzero);
	}
	++sem->count;
	SDL_UnlockMutex(sem->count_lock);
#endif
	return 0;
}

