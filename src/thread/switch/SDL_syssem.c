/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2015 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_THREAD_SWITCH

/* Semaphore functions for the SWITCH. */

#include <stdio.h>

#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_timer.h"

struct SDL_semaphore
{
    Uint32 count;
    Uint32 waiters_count;
    SDL_mutex *count_lock;
    SDL_cond *count_nonzero;
};

/* Create a semaphore */
SDL_sem *SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_sem *sem;

    sem = (SDL_sem *) SDL_malloc(sizeof(*sem));
    if (!sem) {
        SDL_OutOfMemory();
        return NULL;
    }
    sem->count = initial_value;
    sem->waiters_count = 0;

    sem->count_lock = SDL_CreateMutex();
    sem->count_nonzero = SDL_CreateCond();
    if (!sem->count_lock || !sem->count_nonzero) {
        SDL_DestroySemaphore(sem);
        return NULL;
    }

    return sem;
}

/* Free the semaphore */
void SDL_DestroySemaphore(SDL_sem *sem)
{
    if (sem) {
        sem->count = 0xFFFFFFFF;
        while (sem->waiters_count > 0) {
            SDL_CondSignal(sem->count_nonzero);
            SDL_Delay(10);
        }
        SDL_DestroyCond(sem->count_nonzero);
        if (sem->count_lock) {
            SDL_mutexP(sem->count_lock);
            SDL_mutexV(sem->count_lock);
            SDL_DestroyMutex(sem->count_lock);
        }
        SDL_free(sem);
    }
}

/* TODO: This routine is a bit overloaded.
 * If the timeout is 0 then just poll the semaphore; if it's SDL_MUTEX_MAXWAIT, pass
 * NULL to sceKernelWaitSema() so that it waits indefinitely; and if the timeout
 * is specified, convert it to microseconds. */
int SDL_SemWaitTimeout(SDL_sem *sem, Uint32 timeout)
{
    int retval;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    /* A timeout of 0 is an easy case */
    if (timeout == 0) {
        return SDL_SemTryWait(sem);
    }

    SDL_LockMutex(sem->count_lock);
    ++sem->waiters_count;
    retval = 0;
    while ((sem->count == 0) && (retval != SDL_MUTEX_TIMEDOUT)) {
        retval = SDL_CondWaitTimeout(sem->count_nonzero, sem->count_lock, timeout);
    }
    --sem->waiters_count;
    if (retval == 0) {
        --sem->count;
    }
    SDL_UnlockMutex(sem->count_lock);

    return retval;
}

int SDL_SemTryWait(SDL_sem *sem)
{
    int retval;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    retval = SDL_MUTEX_TIMEDOUT;
    SDL_LockMutex(sem->count_lock);
    if (sem->count > 0) {
        --sem->count;
        retval = 0;
    }
    SDL_UnlockMutex(sem->count_lock);

    return retval;
}

int SDL_SemWait(SDL_sem *sem)
{
    return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
}

/* Returns the current count of the semaphore */
Uint32 SDL_SemValue(SDL_sem *sem)
{
    Uint32 value;

    value = 0;
    if (sem) {
        SDL_LockMutex(sem->count_lock);
        value = sem->count;
        SDL_UnlockMutex(sem->count_lock);
    }
    return value;
}

int SDL_SemPost(SDL_sem *sem)
{
    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    SDL_LockMutex(sem->count_lock);
    if (sem->waiters_count > 0) {
        SDL_CondSignal(sem->count_nonzero);
    }
    ++sem->count;
    SDL_UnlockMutex(sem->count_lock);

    return 0;
}

#endif /* SDL_THREAD_SWITCH */
