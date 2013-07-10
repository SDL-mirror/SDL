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

#if SDL_THREAD_BEOS

#include <support/TLS.h>


#define TLS_ALLOC_CHUNKSIZE 8

typedef struct {
    int limit;
    void *data[1];
} SDL_TLSData;

static SDL_SpinLock tls_lock;
static int32 thread_local_storage = B_NO_MEMORY;
static SDL_atomic_t tls_id;


SDL_TLSID
SDL_TLSCreate()
{
    if (thread_local_storage == B_NO_MEMORY) {
        SDL_AtomicLock(&tls_lock);
        if (thread_local_storage == B_NO_MEMORY) {
            thread_local_storage = tls_allocate();
            if (thread_local_storage == B_NO_MEMORY) {
                SDL_SetError("tls_allocate() failed");
                SDL_AtomicUnlock(&tls_lock);
                return 0;
            }
        }
        SDL_AtomicUnlock(&tls_lock);
    }
    return SDL_AtomicIncRef(&tls_id)+1;
}

void *
SDL_TLSGet(SDL_TLSID id)
{
    SDL_TLSData *data;

    data = (SDL_TLSData *)tls_get(thread_local_storage);
    if (!data || id <= 0 || id > data->limit) {
        return NULL;
    }
    return data->data[id-1];
}

int
SDL_TLSSet(SDL_TLSID id, const void *value)
{
    SDL_TLSData *data;

    if (thread_local_storage == B_NO_MEMORY || id <= 0) {
        return SDL_InvalidParamError(id);
    }

    data = (SDL_TLSData *)tls_get(thread_local_storage);
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
        if (!tls_set(thread_local_storage, data)) {
            return SDL_SetError("TlsSetValue() failed");
        }
    }

    data->data[id-1] = SDL_const_cast(void*, value);
    return 0;
}

#endif /* SDL_THREAD_BEOS */

/* vi: set ts=4 sw=4 expandtab: */
