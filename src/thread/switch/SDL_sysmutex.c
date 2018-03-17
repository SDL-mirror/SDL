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

/* An implementation of mutexes using semaphores */

#include "SDL_thread.h"
#include "SDL_systhread_c.h"

struct SDL_mutex
{
    int recursive;
    Uint32 owner;
    Mutex mutex;
};

/* Create a mutex */
SDL_mutex *
SDL_CreateMutex(void)
{
    SDL_mutex *mutex;

    mutex = (SDL_mutex *) SDL_malloc(sizeof(*mutex));
    if (mutex) {
        mutexInit(&mutex->mutex);
        mutex->recursive = 0;
        mutex->owner = 0;
    }
    else {
        SDL_OutOfMemory();
    }
    return mutex;
}

/* Free the mutex */
void
SDL_DestroyMutex(SDL_mutex *mutex)
{
    if (mutex) {
        SDL_free(mutex);
    }
}

/* Lock the semaphore */
int
SDL_mutexP(SDL_mutex *mutex)
{
    Uint32 this_thread;

    if (mutex == NULL) {
        SDL_SetError("Passed a NULL mutex");
        return -1;
    }

    this_thread = (Uint32) SDL_ThreadID();
    if (mutex->owner == this_thread) {
        ++mutex->recursive;
    }
    else {
        /* The order of operations is important.
           We set the locking thread id after we obtain the lock
           so unlocks from other threads will fail.
        */
        mutexLock(&mutex->mutex);
        mutex->owner = this_thread;
        mutex->recursive = 0;
    }

    return 0;
}

/* Unlock the mutex */
int
SDL_mutexV(SDL_mutex *mutex)
{
    if (mutex == NULL) {
        SDL_SetError("Passed a NULL mutex");
        return -1;
    }

    /* If we don't own the mutex, we can't unlock it */
    if (SDL_ThreadID() != mutex->owner) {
        SDL_SetError("mutex not owned by this thread");
        return -1;
    }

    if (mutex->recursive) {
        --mutex->recursive;
    }
    else {
        /* The order of operations is important.
           First reset the owner so another thread doesn't lock
           the mutex and set the ownership before we reset it,
           then release the lock semaphore.
         */
        mutex->owner = 0;
        mutexUnlock(&mutex->mutex);
    }
    return 0;
}

#endif /* SDL_THREAD_SWITCH */
