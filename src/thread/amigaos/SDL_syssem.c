/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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

/* An native implementation of semaphores on AmigaOS */

#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_systhread_c.h"


struct SDL_semaphore
{
	struct SignalSemaphore Sem;
};

#undef D(x)

#define D(x)

SDL_sem *SDL_CreateSemaphore(Uint32 initial_value)
{
	SDL_sem *sem;

	sem = (SDL_sem *)malloc(sizeof(*sem));
	if ( ! sem ) {
		SDL_OutOfMemory();
		return(0);
	}
	memset(sem, 0, sizeof(*sem));

	D(bug("Creating semaphore %lx...\n",sem));

	InitSemaphore(&sem->Sem);
#if 1 // Allow multiple obtainings of the semaphore
        while ( initial_value-- ) {
		ReleaseSemaphore(&sem->Sem);
	}
#endif
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
	int retval;

	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	D(bug("TryWait semaphore...%lx\n",sem));

	retval = SDL_MUTEX_TIMEDOUT;
	if ( AttemptSemaphore(&sem->Sem) ) {
		retval = 0;
	}
	return retval;
}

int SDL_SemWaitTimeout(SDL_sem *sem, Uint32 timeout)
{
	int retval;

	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	D(bug("WaitTimeout (%ld) semaphore...%lx\n",timeout,sem));

#if 1 // We need to keep trying the semaphore until the timeout expires
	retval = SDL_MUTEX_TIMEDOUT;
	then = SDL_GetTicks();
	do {
		if ( AttemptSemaphore(&sem->Sem) ) {
			retval = 0;
		}
		now = SDL_GetTicks();
	} while ( (retval == SDL_MUTEX_TIMEDOUT) && ((now-then) < timeout) );
#else
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
#endif
	return retval;
}

int SDL_SemWait(SDL_sem *sem)
{
	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}
#if 1 // This should be an infinite wait - FIXME, what is the return value?
	ObtainSemaphore(&sem->Sem);
        return 0;
#else
	return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
#endif
}

Uint32 SDL_SemValue(SDL_sem *sem)
{
	Uint32 value;
	
	value = 0;
	if ( sem ) {
		value = sem->Sem.ss_NestCount;
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
	return 0;
}

