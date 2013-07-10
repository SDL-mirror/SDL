/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

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
#include "SDL_config.h"

/* System independent thread management routines for SDL */

#include "SDL_thread.h"
#include "SDL_thread_c.h"
#include "SDL_systhread.h"
#include "../SDL_error_c.h"


/* Routine to get the thread-specific error variable */
SDL_error *
SDL_GetErrBuf(void)
{
    static SDL_SpinLock spinlock;
    static SDL_bool tls_being_created;
    static SDL_TLSID tls_errbuf;
    static SDL_error SDL_global_errbuf;
    SDL_error *errbuf;

    if (!tls_errbuf && !tls_being_created) {
        SDL_AtomicLock(&spinlock);
        if (!tls_errbuf) {
            /* SDL_TLSCreate() could fail and call SDL_SetError() */
            tls_being_created = SDL_TRUE;
            tls_errbuf = SDL_TLSCreate();
            tls_being_created = SDL_FALSE;
        }
        SDL_AtomicUnlock(&spinlock);
    }
    if (!tls_errbuf) {
        return &SDL_global_errbuf;
    }

    errbuf = SDL_TLSGet(tls_errbuf);
    if (!errbuf) {
        errbuf = (SDL_error *)SDL_malloc(sizeof(*errbuf));
        if (!errbuf) {
            return &SDL_global_errbuf;
        }
        SDL_zerop(errbuf);
        SDL_TLSSet(tls_errbuf, errbuf);
    }
    return errbuf;
}


/* Arguments and callback to setup and run the user thread function */
typedef struct
{
    int (SDLCALL * func) (void *);
    void *data;
    SDL_Thread *info;
    SDL_sem *wait;
} thread_args;

void
SDL_RunThread(void *data)
{
    thread_args *args = (thread_args *) data;
    int (SDLCALL * userfunc) (void *) = args->func;
    void *userdata = args->data;
    int *statusloc = &args->info->status;

    /* Perform any system-dependent setup
       - this function cannot fail, and cannot use SDL_SetError()
     */
    SDL_SYS_SetupThread(args->info->name);

    /* Get the thread id */
    args->info->threadid = SDL_ThreadID();

    /* Wake up the parent thread */
    SDL_SemPost(args->wait);

    /* Run the function */
    *statusloc = userfunc(userdata);
}

#ifdef SDL_PASSED_BEGINTHREAD_ENDTHREAD
#undef SDL_CreateThread
DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThread(int (SDLCALL * fn) (void *),
                 const char *name, void *data,
                 pfnSDL_CurrentBeginThread pfnBeginThread,
                 pfnSDL_CurrentEndThread pfnEndThread)
#else
DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThread(int (SDLCALL * fn) (void *),
                 const char *name, void *data)
#endif
{
    SDL_Thread *thread;
    thread_args *args;
    int ret;

    /* Allocate memory for the thread info structure */
    thread = (SDL_Thread *) SDL_malloc(sizeof(*thread));
    if (thread == NULL) {
        SDL_OutOfMemory();
        return (NULL);
    }
    SDL_memset(thread, 0, (sizeof *thread));
    thread->status = -1;

    /* Set up the arguments for the thread */
    if (name != NULL) {
        thread->name = SDL_strdup(name);
        if (thread->name == NULL) {
            SDL_OutOfMemory();
            SDL_free(thread);
            return (NULL);
        }
    }

    /* Set up the arguments for the thread */
    args = (thread_args *) SDL_malloc(sizeof(*args));
    if (args == NULL) {
        SDL_OutOfMemory();
        SDL_free(thread->name);
        SDL_free(thread);
        return (NULL);
    }
    args->func = fn;
    args->data = data;
    args->info = thread;
    args->wait = SDL_CreateSemaphore(0);
    if (args->wait == NULL) {
        SDL_free(thread->name);
        SDL_free(thread);
        SDL_free(args);
        return (NULL);
    }

    /* Create the thread and go! */
#ifdef SDL_PASSED_BEGINTHREAD_ENDTHREAD
    ret = SDL_SYS_CreateThread(thread, args, pfnBeginThread, pfnEndThread);
#else
    ret = SDL_SYS_CreateThread(thread, args);
#endif
    if (ret >= 0) {
        /* Wait for the thread function to use arguments */
        SDL_SemWait(args->wait);
    } else {
        /* Oops, failed.  Gotta free everything */
        SDL_free(thread->name);
        SDL_free(thread);
        thread = NULL;
    }
    SDL_DestroySemaphore(args->wait);
    SDL_free(args);

    /* Everything is running now */
    return (thread);
}

SDL_threadID
SDL_GetThreadID(SDL_Thread * thread)
{
    SDL_threadID id;

    if (thread) {
        id = thread->threadid;
    } else {
        id = SDL_ThreadID();
    }
    return id;
}

const char *
SDL_GetThreadName(SDL_Thread * thread)
{
    return thread->name;
}

int
SDL_SetThreadPriority(SDL_ThreadPriority priority)
{
    return SDL_SYS_SetThreadPriority(priority);
}

void
SDL_WaitThread(SDL_Thread * thread, int *status)
{
    if (thread) {
        SDL_SYS_WaitThread(thread);
        if (status) {
            *status = thread->status;
        }
        SDL_free(thread->name);
        SDL_free(thread);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
