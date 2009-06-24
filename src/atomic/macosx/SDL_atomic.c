/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

#ifdef SDL_ATOMIC_LINUX

#include "SDL.h"
#include "SDL_config.h"
#include "SDL_atomic.h"

Uint32
SDL_AtomicExchange32(Uint32 * ptr, Uint32 value)
{
   return 0;
}

SDL_bool
SDL_AtomicCompareThenSet32(Uint32 * ptr, Uint32 oldvalue, Uint32 newvalue)
{
   return SDL_false;
}

SDL_bool
SDL_AtomicTestThenSet32(Uint32 * ptr)
{
   return SDL_false;
}

void
SDL_AtomicClear32(Uint32 * ptr)
{
}

Uint32
SDL_AtomicFetchThenIncrement32(Uint32 * ptr)
{
   return 0;
}

Uint32
SDL_AtomicFetchThenDecrement32(Uint32 * ptr)
{
   return 0;
}

Uint32
SDL_AtomicFetchThenAdd32(Uint32 * ptr, Uint32 value)
{
   return 0;
}

Uint32
SDL_AtomicFetchThenSubtract32(Uint32 * ptr, Uint32 value)
{
   return 0;
}

Uint32
SDL_AtomicIncrementThenFetch32(Uint32 * ptr)
{
   return 0;
}

Uint32
SDL_AtomicDecrementThenFetch32(Uint32 * ptr)
{
   return 0;
}

Uint32
SDL_AtomicAddThenFetch32(Uint32 * ptr, Uint32 value)
{
   return 0;
}

Uint32
SDL_AtomicSubtractThenFetch32(Uint32 * ptr, Uint32 value)
{
   return 0;
}


#ifdef SDL_HAS_64BIT_TYPE

Uint64
SDL_AtomicExchange64(Uint64 * ptr, Uint64 value)
{
   return 0;
}

SDL_bool
SDL_AtomicCompareThenSet64(Uint64 * ptr,
Uint64 oldvalue, Uint64 newvalue)
{
   return SDL_false;
}

SDL_bool
SDL_AtomicTestThenSet64(Uint64 * ptr)
{
   return SDL_false;
}

void
SDL_AtomicClear64(Uint64 * ptr)
{
}

Uint64
SDL_AtomicFetchThenIncrement64(Uint64 * ptr)
{
   return 0;
}

Uint64
SDL_AtomicFetchThenDecrement64(Uint64 * ptr)
{
   return 0;
}

Uint64
SDL_AtomicFetchThenAdd64(Uint64 * ptr, Uint64 value)
{
   return 0;
}

Uint64
SDL_AtomicFetchThenSubtract64(Uint64 * ptr, Uint64 value)
{
   return 0;
}

Uint64
SDL_AtomicIncrementThenFetch64(Uint64 * ptr)
{
   return 0;
}

Uint64
SDL_AtomicDecrementThenFetch64(Uint64 * ptr)
{
   return 0;
}

Uint64
SDL_AtomicAddThenFetch64(Uint64 * ptr, Uint64 value)
{
   return 0;
}

Uint64
SDL_AtomicSubtractThenFetch64(Uint64 * ptr, Uint64 value)
{
   return 0;
}

#endif
#endif
