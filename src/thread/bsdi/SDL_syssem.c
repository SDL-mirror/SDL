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
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include <stdlib.h>
#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_timer.h"

#ifdef SDL_USE_PTHREADS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>			/* For getpid() */
#include <pthread.h>


/*
 * This is semaphore.h inlined here so that BSD/OS POSIX semaphore are
 * completely selfcontained without requiring any additional include files
 * or libraries not present in the stock system
*/

/* semaphore.h: POSIX 1003.1b semaphores */

/*-
 * Copyright (c) 1996, 1997
 *	HD Associates, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by HD Associates, Inc
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY HD ASSOCIATES AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL HD ASSOCIATES OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/posix4/semaphore.h,v 1.6 2000/01/20 07:55:42 jasone Exp $
 */

#include <machine/limits.h>

#include <sys/types.h>
#include <fcntl.h>

/* Opaque type definition. */
struct sem;
typedef struct sem *sem_t;

#define SEM_FAILED	((sem_t *)0)
#define SEM_VALUE_MAX	UINT_MAX

#include <sys/cdefs.h>

__BEGIN_DECLS
int	 sem_init __P((sem_t *, int, unsigned int));
int	 sem_destroy __P((sem_t *));
sem_t	*sem_open __P((const char *, int, ...));
int	 sem_close __P((sem_t *));
int	 sem_unlink __P((const char *));
int	 sem_wait __P((sem_t *));
int	 sem_trywait __P((sem_t *));
int	 sem_post __P((sem_t *));
int	 sem_getvalue __P((sem_t *, int *));
__END_DECLS

/* END of inlined semaphore.h */

/* Wrapper around POSIX 1003.1b semaphores */

struct SDL_semaphore {
	sem_t *sem;
	sem_t sem_data;
};

/* Create a semaphore, initialized with value */
SDL_sem *SDL_CreateSemaphore(Uint32 initial_value)
{
	SDL_sem *sem = (SDL_sem *) malloc(sizeof(SDL_sem));
	if ( sem ) {
		if ( sem_init(&sem->sem_data, 0, initial_value) < 0 ) {
			SDL_SetError("sem_init() failed");
			free(sem);
			sem = NULL;
		} else {
			sem->sem = &sem->sem_data;
		}
	} else {
		SDL_OutOfMemory();
	}
	return sem;
}

void SDL_DestroySemaphore(SDL_sem *sem)
{
	if ( sem ) {
		sem_destroy(sem->sem);
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
	if ( sem_trywait(sem->sem) == 0 )
		retval = 0;
	return retval;
}

int SDL_SemWait(SDL_sem *sem)
{
	int retval;

	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

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
		sem_getvalue(sem->sem, &ret);
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

	retval = sem_post(sem->sem);
	if ( retval < 0 ) {
		SDL_SetError("sem_post() failed");
	}
	return retval;
}

/* 
 * BEGIN inlined uthread_sem.c.  This is done here so that no extra libraries
 * or include files not present in BSD/OS are required
*/

/*
 * Copyright (C) 2000 Jason Evans <jasone@freebsd.org>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/libc_r/uthread/uthread_sem.c,v 1.3.2.1 2000/07/18 02:05:57 jasone Exp $
 */

#include <errno.h>
#include <pthread.h>

/* Begin thread_private.h kluge */
/*
 * These come out of (or should go into) thread_private.h - rather than have 
 * to copy (or symlink) the files from the source tree these definitions are 
 * inlined here.  Obviously these go away when this module is part of libc.
*/
struct sem {
#define SEM_MAGIC       ((u_int32_t) 0x09fa4012)
        u_int32_t       magic;
        pthread_mutex_t lock;
        pthread_cond_t  gtzero;
        u_int32_t       count;
        u_int32_t       nwaiters;
};

extern pthread_once_t _thread_init_once;
extern int _threads_initialized;
extern void  _thread_init __P((void));
#define THREAD_INIT() \
	(void) pthread_once(&_thread_init_once, _thread_init)
#define THREAD_SAFE() \
	(_threads_initialized != 0)

#define _SEM_CHECK_VALIDITY(sem)		\
	if ((*(sem))->magic != SEM_MAGIC) {	\
		errno = EINVAL;			\
		retval = -1;			\
		goto RETURN;			\
	}
/* End thread_private.h kluge */

int
sem_init(sem_t *sem, int pshared, unsigned int value)
{
	int	retval;

	if (!THREAD_SAFE())
		THREAD_INIT();

	/*
	 * Range check the arguments.
	 */
	if (pshared != 0) {
		/*
		 * The user wants a semaphore that can be shared among
		 * processes, which this implementation can't do.  Sounds like a
		 * permissions problem to me (yeah right).
		 */
		errno = EPERM;
		retval = -1;
		goto RETURN;
	}

	if (value > SEM_VALUE_MAX) {
		errno = EINVAL;
		retval = -1;
		goto RETURN;
	}

	*sem = (sem_t)malloc(sizeof(struct sem));
	if (*sem == NULL) {
		errno = ENOSPC;
		retval = -1;
		goto RETURN;
	}

	/*
	 * Initialize the semaphore.
	 */
	if (pthread_mutex_init(&(*sem)->lock, NULL) != 0) {
		free(*sem);
		errno = ENOSPC;
		retval = -1;
		goto RETURN;
	}

	if (pthread_cond_init(&(*sem)->gtzero, NULL) != 0) {
		pthread_mutex_destroy(&(*sem)->lock);
		free(*sem);
		errno = ENOSPC;
		retval = -1;
		goto RETURN;
	}
	
	(*sem)->count = (u_int32_t)value;
	(*sem)->nwaiters = 0;
	(*sem)->magic = SEM_MAGIC;

	retval = 0;
  RETURN:
	return retval;
}

int
sem_destroy(sem_t *sem)
{
	int	retval;
	
	_SEM_CHECK_VALIDITY(sem);

	/* Make sure there are no waiters. */
	pthread_mutex_lock(&(*sem)->lock);
	if ((*sem)->nwaiters > 0) {
		pthread_mutex_unlock(&(*sem)->lock);
		errno = EBUSY;
		retval = -1;
		goto RETURN;
	}
	pthread_mutex_unlock(&(*sem)->lock);
	
	pthread_mutex_destroy(&(*sem)->lock);
	pthread_cond_destroy(&(*sem)->gtzero);
	(*sem)->magic = 0;

	free(*sem);

	retval = 0;
  RETURN:
	return retval;
}

sem_t *
sem_open(const char *name, int oflag, ...)
{
	errno = ENOSYS;
	return SEM_FAILED;
}

int
sem_close(sem_t *sem)
{
	errno = ENOSYS;
	return -1;
}

int
sem_unlink(const char *name)
{
	errno = ENOSYS;
	return -1;
}

int
sem_wait(sem_t *sem)
{
	int	retval;

	pthread_testcancel();
	
	_SEM_CHECK_VALIDITY(sem);

	pthread_mutex_lock(&(*sem)->lock);

	while ((*sem)->count == 0) {
		(*sem)->nwaiters++;
		pthread_cond_wait(&(*sem)->gtzero, &(*sem)->lock);
		(*sem)->nwaiters--;
	}
	(*sem)->count--;

	pthread_mutex_unlock(&(*sem)->lock);

	retval = 0;
  RETURN:

	pthread_testcancel();
	return retval;
}

int
sem_trywait(sem_t *sem)
{
	int	retval;

	_SEM_CHECK_VALIDITY(sem);

	pthread_mutex_lock(&(*sem)->lock);

	if ((*sem)->count > 0) {
		(*sem)->count--;
		retval = 0;
	} else {
		errno = EAGAIN;
		retval = -1;
	}
	
	pthread_mutex_unlock(&(*sem)->lock);

  RETURN:
	return retval;
}

int
sem_post(sem_t *sem)
{
	int	retval;

	_SEM_CHECK_VALIDITY(sem);

	pthread_mutex_lock(&(*sem)->lock);

	(*sem)->count++;
	if ((*sem)->nwaiters > 0) {
		/*
		 * We must use pthread_cond_broadcast() rather than
		 * pthread_cond_signal() in order to assure that the highest
		 * priority thread is run by the scheduler, since
		 * pthread_cond_signal() signals waiting threads in FIFO order.
		 */
		pthread_cond_broadcast(&(*sem)->gtzero);
	}

	pthread_mutex_unlock(&(*sem)->lock);

	retval = 0;
  RETURN:
	return retval;
}

int
sem_getvalue(sem_t *sem, int *sval)
{
	int	retval;

	_SEM_CHECK_VALIDITY(sem);

	pthread_mutex_lock(&(*sem)->lock);
	*sval = (int)(*sem)->count;
	pthread_mutex_unlock(&(*sem)->lock);

	retval = 0;
  RETURN:
	return retval;
}

/* END of inlined uthread_sem.c */
#endif /* SDL_USE_PTHREADS */
