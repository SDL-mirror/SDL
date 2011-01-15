/*
  SDL - Simple DirectMedia Layer
  Copyright (C) 1997-2010 Sam Lantinga

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Sam Lantinga
  slouken@libsdl.org
*/
#include "SDL_stdinc.h"

#include "SDL_atomic.h"

/* 
  If any of the operations are not provided then we must emulate some
  of them. That means we need a nice implementation of spin locks
  that avoids the "one big lock" problem. We use a vector of spin
  locks and pick which one to use based on the address of the operand
  of the function.

  To generate the index of the lock we first shift by 3 bits to get
  rid on the zero bits that result from 32 and 64 bit allignment of
  data. We then mask off all but 5 bits and use those 5 bits as an
  index into the table. 

  Picking the lock this way insures that accesses to the same data at
  the same time will go to the same lock. OTOH, accesses to different
  data have only a 1/32 chance of hitting the same lock. That should
  pretty much eliminate the chances of several atomic operations on
  different data from waiting on the same "big lock". If it isn't
  then the table of locks can be expanded to a new size so long as
  the new size is a power of two.

  Contributed by Bob Pendleton, bob@pendleton.com
*/

static SDL_SpinLock locks[32];

static __inline__ void
enterLock(void *a)
{
   uintptr_t index = ((((uintptr_t)a) >> 3) & 0x1f);

   SDL_AtomicLock(&locks[index]);
}

static __inline__ void
leaveLock(void *a)
{
   uintptr_t index = ((((uintptr_t)a) >> 3) & 0x1f);

   SDL_AtomicUnlock(&locks[index]);
}

#ifndef SDL_AtomicSet
int
SDL_AtomicSet(SDL_atomic_t *a, int value)
{
    int oldvalue;

    enterLock(a);
    oldvalue = a->value;
    a->value = value;
    leaveLock(a);

    return oldvalue;
}
#endif

#ifndef SDL_AtomicGet
int
SDL_AtomicGet(SDL_atomic_t *a)
{
    /* Assuming integral reads on this platform, we're safe here since the
       functions that set the variable have the necessary memory barriers.
    */
    return a->value;
}
#endif

#ifndef SDL_AtomicAdd
int
SDL_AtomicAdd(SDL_atomic_t *a, int value)
{
    int oldvalue;

    enterLock(a);
    oldvalue = a->value;
    a->value += value;
    leaveLock(a);

    return oldvalue;
}
#endif

#ifndef SDL_AtomicIncRef
void
SDL_AtomicIncRef(SDL_atomic_t *a)
{
    SDL_AtomicAdd(a, 1);
}
#endif

#ifndef SDL_AtomicDecRef
SDL_bool
SDL_AtomicDecRef(SDL_atomic_t *a)
{
    return SDL_AtomicAdd(a, -1) == 1;
}
#endif

#ifndef SDL_AtomicCAS
int
SDL_AtomicCAS(SDL_atomic_t *a, int oldval, int newval)
{
    int prevval;

    enterLock(a);
    prevval = a->value;
    if (prevval == oldval) {
        a->value = newval;
    }
    leaveLock(a);

    return prevval;
}
#endif

#ifndef SDL_AtomicSetPtr
void
SDL_AtomicSetPtr(void** a, void* value)
{
    void *prevval;
    do {
        prevval = *a;
    } while (SDL_AtomicCASPtr(a, prevval, value) != prevval);
}
#endif

#ifndef SDL_AtomicGetPtr
void*
SDL_AtomicGetPtr(void** a)
{
    /* Assuming integral reads on this platform, we're safe here since the
       functions that set the pointer have the necessary memory barriers.
    */
    return *a;
}
#endif

#ifndef SDL_AtomicCASPtr
void* SDL_AtomicCASPtr(void **a, void *oldval, void *newval)
{
    void *prevval;

    enterLock(a);
    prevval = *a;
    if (*a == oldval) {
        *a = newval;
    }
    leaveLock(a);

    return prevval;
}
#endif

/* vi: set ts=4 sw=4 expandtab: */
