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

#include <stdlib.h>
#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_timer.h"

#ifdef linux
/* Look to see if glibc is available, and if so, what version */
#include <features.h>

#if (__GLIBC__ == 2) && (__GLIBC_MINOR__ == 0)
#warning Working around a bug in glibc 2.0 pthreads
#undef SDL_USE_PTHREADS
/* The bug is actually a problem where threads are suspended, but don't
   wake up when the thread manager sends them a signal.  This is a problem
   with thread creation too, but it happens less often. :-/
   We avoid this by using System V IPC for semaphores.
 */
#endif /* glibc 2.0 */
#endif /* linux */

#ifdef SDL_USE_PTHREADS

#ifdef SDL_NO_PTHREAD_SEMAPHORES
#include "generic/SDL_syssem.c"
#else

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>			/* For getpid() */
#include <pthread.h>
#include <semaphore.h>

/* Wrapper around POSIX 1003.1b semaphores */

#ifdef MACOSX
#define USE_NAMED_SEMAPHORES
/* Broken sem_getvalue() in MacOS X Public Beta */
#define BROKEN_SEMGETVALUE
#endif /* MACOSX */

struct SDL_semaphore {
	sem_t *sem;
#ifndef USE_NAMED_SEMAPHORES
	sem_t sem_data;
#endif
#ifdef BROKEN_SEMGETVALUE
	/* This is a little hack for MacOS X -
	   It's not thread-safe, but it's better than nothing
	 */
	int sem_value;
#endif
};

/* Create a semaphore, initialized with value */
SDL_sem *SDL_CreateSemaphore(Uint32 initial_value)
{
	SDL_sem *sem = (SDL_sem *) malloc(sizeof(SDL_sem));
	if ( sem ) {
#ifdef USE_NAMED_SEMAPHORES
		static int semnum = 0;
		char name[32];

		sprintf(name, "/SDL_sem-%d-%4.4d", getpid(), semnum++);
		sem->sem = sem_open(name, O_CREAT, 0600, initial_value);
		if ( sem->sem == (sem_t *)SEM_FAILED ) {
			SDL_SetError("sem_open(%s) failed", name);
			free(sem);
			sem = NULL;
		} else {
			sem_unlink(name);
		}
#else
		if ( sem_init(&sem->sem_data, 0, initial_value) < 0 ) {
			SDL_SetError("sem_init() failed");
			free(sem);
			sem = NULL;
		} else {
			sem->sem = &sem->sem_data;
		}
#endif /* USE_NAMED_SEMAPHORES */

#ifdef BROKEN_SEMGETVALUE
		if ( sem ) {
			sem->sem_value = initial_value;
		}
#endif /* BROKEN_SEMGETVALUE */
	} else {
		SDL_OutOfMemory();
	}
	return sem;
}

void SDL_DestroySemaphore(SDL_sem *sem)
{
	if ( sem ) {
#ifdef USE_NAMED_SEMAPHORES
		sem_close(sem->sem);
#else
		sem_destroy(sem->sem);
#endif
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
	retval = SDL_MUTEX_TIMEDOUT;
	if ( sem_trywait(sem->sem) == 0 ) {
#ifdef BROKEN_SEMGETVALUE
		--sem->sem_value;
#endif
		retval = 0;
	}
	return retval;
}

int SDL_SemWait(SDL_sem *sem)
{
	int retval;

	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

#ifdef BROKEN_SEMGETVALUE
	--sem->sem_value;
#endif
	retval = sem_wait(sem->sem);
	if ( retval < 0 ) {
		SDL_SetError("sem_wait() failed");
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

	/* Try the easy cases first */
	if ( timeout == 0 ) {
		return SDL_SemTryWait(sem);
	}
	if ( timeout == SDL_MUTEX_MAXWAIT ) {
		return SDL_SemWait(sem);
	}

	/* Ack!  We have to busy wait... */
	timeout += SDL_GetTicks();
	do {
		retval = SDL_SemTryWait(sem);
		if ( retval == 0 ) {
			break;
		}
		SDL_Delay(1);
	} while ( SDL_GetTicks() < timeout );

	return retval;
}

Uint32 SDL_SemValue(SDL_sem *sem)
{
	int ret = 0;
	if ( sem ) {
#ifdef BROKEN_SEMGETVALUE
		ret = sem->sem_value;
#else
		sem_getvalue(sem->sem, &ret);
#endif
		if ( ret < 0 ) {
			ret = 0;
		}
	}
	return (Uint32)ret;
}

int SDL_SemPost(SDL_sem *sem)
{
	int retval;

	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

#ifdef BROKEN_SEMGETVALUE
	++sem->sem_value;
#endif
	retval = sem_post(sem->sem);
	if ( retval < 0 ) {
		SDL_SetError("sem_post() failed");
	}
	return retval;
}

#endif /* NO_PTHREAD_SEMAPHORES */

#else /* System V IPC implementation */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "SDL_error.h"
#include "SDL_thread.h"


struct SDL_semaphore {
	int id;
};

/* Not defined by many operating systems, use configure to detect */
#if !defined(HAVE_SEMUN)
union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
};
#endif

static struct sembuf op_trywait[2] = {
	{ 0, -1, (IPC_NOWAIT|SEM_UNDO) } /* Decrement semaphore, no block */
};
static struct sembuf op_wait[2] = {
	{ 0, -1, SEM_UNDO }		/* Decrement semaphore */
};
static struct sembuf op_post[1] = {
	{ 0, 1, (IPC_NOWAIT|SEM_UNDO) }	/* Increment semaphore */
};

/* Create a blockable semaphore */
SDL_sem *SDL_CreateSemaphore(Uint32 initial_value)
{
	extern int _creating_thread_lock;	/* SDL_threads.c */
	SDL_sem *sem;
	union semun init;
	key_t key;

	sem = (SDL_sem *)malloc(sizeof(*sem));
	if ( sem == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}
	/* This flag is true if we are creating the thread manager sem,
	   which is never freed.  This allows us to reuse the same sem.
	*/
	if ( _creating_thread_lock ) {
		key = 'S'+'D'+'L';
	} else {
		key = IPC_PRIVATE;
	}
	/* Keep trying to create sem while we don't own the requested key */
	do {
		if ( key != IPC_PRIVATE ) {
			++key;
		}
		sem->id = semget(key, 1, (0600|IPC_CREAT));
	} while ((sem->id < 0) && (key != IPC_PRIVATE) && (errno == EACCES));

	/* Report the error if we eventually failed */
	if ( sem->id < 0 ) {
		SDL_SetError("Couldn't create semaphore");
		free(sem);
		return(NULL);
	}
	init.val = initial_value;	/* Initialize semaphore */
	semctl(sem->id, 0, SETVAL, init);
	return(sem);
}

void SDL_DestroySemaphore(SDL_sem *sem)
{
	if ( sem ) {
#ifdef _SGI_SOURCE
		semctl(sem->id, 0, IPC_RMID);
#else
		union semun dummy;
		dummy.val = 0;
		semctl(sem->id, 0, IPC_RMID, dummy);
#endif
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

	retval = 0;
  tryagain:
	if ( semop(sem->id, op_trywait, 1) < 0 ) {
		if ( errno == EINTR ) {
			goto tryagain;
		}
		retval = SDL_MUTEX_TIMEDOUT;
	}
	return retval;
}

int SDL_SemWait(SDL_sem *sem)
{
	int retval;

	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	retval = 0;
  tryagain:
	if ( semop(sem->id, op_wait, 1) < 0 ) {
		if ( errno == EINTR ) {
			goto tryagain;
		}
		SDL_SetError("Semaphore operation error");
		retval = -1;
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

	/* Try the easy cases first */
	if ( timeout == 0 ) {
		return SDL_SemTryWait(sem);
	}
	if ( timeout == SDL_MUTEX_MAXWAIT ) {
		return SDL_SemWait(sem);
	}

	/* Ack!  We have to busy wait... */
	timeout += SDL_GetTicks();
	do {
		retval = SDL_SemTryWait(sem);
		if ( retval == 0 ) {
			break;
		}
		SDL_Delay(1);
	} while ( SDL_GetTicks() < timeout );

	return retval;
}

Uint32 SDL_SemValue(SDL_sem *sem)
{
	int semval;
	Uint32 value;
	
	value = 0;
	if ( sem ) {
	  tryagain:
#ifdef _SGI_SOURCE
		semval = semctl(sem->id, 0, GETVAL);
#else
		{
		union semun arg;
		arg.val = 0;
		semval = semctl(sem->id, 0, GETVAL, arg);
		}
#endif
		if ( semval < 0 ) {
			if ( errno == EINTR ) {
				goto tryagain;
			}
		} else {
			value = (Uint32)semval;
		}
	}
	return value;
}

int SDL_SemPost(SDL_sem *sem)
{
	int retval;

	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	retval = 0;
  tryagain:
	if ( semop(sem->id, op_post, 1) < 0 ) {
		if ( errno == EINTR ) {
			goto tryagain;
		}
		SDL_SetError("Semaphore operation error");
		retval = -1;
	}
	return retval;
}

#endif /* SDL_USE_PTHREADS */
