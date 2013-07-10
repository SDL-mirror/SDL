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
#include "SDL_thread.h"

/* This is a generic implementation of thread-local storage which doesn't
   require additional OS support.

   It is not especially efficient and doesn't clean up thread-local storage
   as threads exit.  If there is a real OS that doesn't support thread-local
   storage this implementation should be improved to be production quality.
*/

#define TLS_ALLOC_CHUNKSIZE 8

typedef struct {
    int limit;
    void *data[1];
} SDL_TLSData;

typedef struct SDL_TLSEntry {
    SDL_threadID thread;
    SDL_TLSData *data;
    struct SDL_TLSEntry *next;
} SDL_TLSEntry;

static SDL_SpinLock tls_lock;
static SDL_mutex *tls_mutex;
static SDL_TLSEntry *thread_local_storage;
static SDL_atomic_t tls_id;


static SDL_TLSData *GetTLSData()
{
    SDL_threadID thread = SDL_ThreadID();
    SDL_TLSEntry *entry;
    SDL_TLSData *data = NULL;

    if (!tls_mutex) {
        SDL_AtomicLock(&tls_lock);
        if (!tls_mutex) {
            tls_mutex = SDL_CreateMutex();
            if (!tls_mutex) {
                SDL_AtomicUnlock(&tls_lock);
                return NULL;
            }
        }
        SDL_AtomicUnlock(&tls_lock);
    }

    SDL_LockMutex(tls_mutex);
    for (entry = thread_local_storage; entry; entry = entry->next) {
        if (entry->thread == thread) {
            data = entry->data;
            break;
        }
    }
    SDL_UnlockMutex(tls_mutex);

    return data;
}

static int SetTLSData(SDL_TLSData *data)
{
    SDL_threadID thread = SDL_ThreadID();
    SDL_TLSEntry *entry;

    /* GetTLSData() is always called first, so we can assume tls_mutex */
    SDL_LockMutex(tls_mutex);
    for (entry = thread_local_storage; entry; entry = entry->next) {
        if (entry->thread == thread) {
            entry->data = data;
            break;
        }
    }
    if (!entry) {
        entry = (SDL_TLSEntry *)SDL_malloc(sizeof(*entry));
        if (entry) {
            entry->thread = thread;
            entry->data = data;
            entry->next = thread_local_storage;
            thread_local_storage = entry;
        }
    }
    SDL_UnlockMutex(tls_mutex);

    if (!entry) {
        return SDL_OutOfMemory();
    }
    return 0;
}


SDL_TLSID
SDL_TLSCreate()
{
    return SDL_AtomicIncRef(&tls_id)+1;
}

void *
SDL_TLSGet(SDL_TLSID id)
{
    SDL_TLSData *data;

    data = GetTLSData();
    if (!data || id <= 0 || id > data->limit) {
        return NULL;
    }
    return data->data[id-1];
}

int
SDL_TLSSet(SDL_TLSID id, const void *value)
{
    SDL_TLSData *data;

    if (id <= 0) {
        return SDL_InvalidParamError(id);
    }

    data = GetTLSData();
    if (!data || id > data->limit) {
        int i, oldlimit, newlimit;

        oldlimit = data ? data->limit : 0;
        newlimit = (id + TLS_ALLOC_CHUNKSIZE);
        data = (SDL_TLSData *)SDL_realloc(data, sizeof(*data)+(newlimit-1)*sizeof(void*));
        if (!data) {
            return SDL_OutOfMemory();
        }
        data->limit = newlimit;
        for (i = oldlimit; i < newlimit; ++i) {
            data->data[i] = NULL;
        }
        if (SetTLSData(data) != 0) {
            return -1;
        }
    }

    data->data[id-1] = SDL_const_cast(void*, value);
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
