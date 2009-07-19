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

#include "SDL_stdinc.h"
#include "SDL_atomic.h"

#include <atomic.h>

/*
  This file provides 8, 16, 32, and 64 bit atomic operations. If the
  operations are provided by the native hardware and operating system
  they are used. If they are not then the operations are emulated
  using the SDL mutex operations. 
 */

/* 
  First, detect whether the operations are supported and create
  #defines that indicate that they do exist. The goal is to have all
  the system dependent code in the top part of the file so that the
  bottom can be use unchanged across all platforms.

  Second, #define all the operations in each size class that are
  supported. Doing this allows supported operations to be used along
  side of emulated operations.
*/

/* 
   Emmulated version.

   Assume there is no support for atomic operations. All such
   operations are implemented using SDL mutex operations.
 */

#ifdef EMULATED_ATOMIC_OPERATIONS
#undef EMULATED_ATOMIC_OPERATIONS
#endif

#ifdef EMULATED_ATOMIC_OPERATIONS
#define HAVE_ALL_8_BIT_OPS

#define nativeExchange8(ptr, value)			()
#define nativeCompareThenSet8(ptr, oldvalue, newvalue) 	()
#define nativeTestThenSet8(ptr)    	     		()
#define nativeClear8(ptr)				()
#define nativeFetchThenIncrement8(ptr)   		()
#define nativeFetchThenDecrement8(ptr) 			()
#define nativeFetchThenAdd8(ptr, value) 		()
#define nativeFetchThenSubtract8(ptr, value) 		()
#define nativeIncrementThenFetch8(ptr) 			()
#define nativeDecrementThenFetch8(ptr) 			()
#define nativeAddThenFetch8(ptr, value) 		()
#define nativeSubtractThenFetch8(ptr, value) 		()
#endif

#ifdef EMULATED_ATOMIC_OPERATIONS
#define HAVE_ALL_16_BIT_OPS

#define nativeExchange16(ptr, value)			()
#define nativeCompareThenSet16(ptr, oldvalue, newvalue) ()
#define nativeTestThenSet16(ptr)    	     		()
#define nativeClear16(ptr)				()
#define nativeFetchThenIncrement16(ptr)   		()
#define nativeFetchThenDecrement16(ptr) 		()
#define nativeFetchThenAdd16(ptr, value) 		()
#define nativeFetchThenSubtract16(ptr, value) 		()
#define nativeIncrementThenFetch16(ptr) 		()
#define nativeDecrementThenFetch16(ptr) 		()
#define nativeAddThenFetch16(ptr, value) 		()
#define nativeSubtractThenFetch16(ptr, value) 		()
#endif

#ifdef EMULATED_ATOMIC_OPERATIONS
#define HAVE_ALL_64_BIT_OPS

#define nativeExchange64(ptr, value)			()
#define nativeCompareThenSet64(ptr, oldvalue, newvalue) ()
#define nativeTestThenSet64(ptr)    	     		()
#define nativeClear64(ptr)				()
#define nativeFetchThenIncrement64(ptr)   		()
#define nativeFetchThenDecrement64(ptr) 		()
#define nativeFetchThenAdd64(ptr, value) 		()
#define nativeFetchThenSubtract64(ptr, value) 		()
#define nativeIncrementThenFetch64(ptr) 		()
#define nativeDecrementThenFetch64(ptr) 		()
#define nativeAddThenFetch64(ptr, value) 		()
#define nativeSubtractThenFetch64(ptr, value) 		()
#endif

/*
If any of the operations are not provided then we must emulate some of
them.
 */

#if !defined(HAVE_ALL_8_BIT_OPS) || !defined(HAVE_ALL_16_BIT_OPS) || !defined(HAVE_ALL_64_BIT_OPS)

#include "SDL_mutex.h"
#include "SDL_error.h"

static SDL_mutex * lock = NULL;

static __inline__ void
privateWaitLock()
{
   if(NULL == lock)
   {
      lock = SDL_CreateMutex();
      if (NULL == lock)
      {
	 SDL_SetError("SDL_atomic.c: can't create a mutex");
	 return;
      }
   }

   if (-1 == SDL_LockMutex(lock))
   {
      SDL_SetError("SDL_atomic.c: can't lock mutex");
   }
}

static __inline__ void
privateUnlock()
{
   if (-1 == SDL_UnlockMutex(lock))
   {
      SDL_SetError("SDL_atomic.c: can't unlock mutex");
   }
}

#endif

/* 8 bit atomic operations */

Uint8
SDL_AtomicExchange8(volatile Uint8 * ptr, Uint8 value)
{
#ifdef nativeExchange8
   return nativeExchange8(ptr, value);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   *ptr = value;
   privateUnlock();

   return tmp;
#endif
}

SDL_bool
SDL_AtomicCompareThenSet8(volatile Uint8 * ptr, Uint8 oldvalue, Uint8 newvalue)
{
#ifdef nativeCompareThenSet8
   return (SDL_bool)nativeCompareThenSet8(ptr, oldvalue, newvalue);
#else
   SDL_bool result = SDL_FALSE;

   privateWaitLock();
   result = (*ptr == oldvalue);
   if (result)
   {
      *ptr = newvalue;
   }
   privateUnlock();

   return result;
#endif
}

SDL_bool
SDL_AtomicTestThenSet8(volatile Uint8 * ptr)
{
#ifdef nativeTestThenSet8
   return (SDL_bool)nativeTestThenSet8(ptr);
#else
   SDL_bool result = SDL_FALSE;

   privateWaitLock();
   result = (*ptr == 0);
   if (result)
   {
      *ptr = 1;
   }
   privateUnlock();

   return result;
#endif
}

void
SDL_AtomicClear8(volatile Uint8 * ptr)
{
#ifdef nativeClear8
   nativeClear8(ptr);
#else
   privateWaitLock();
   *ptr = 0;
   privateUnlock();

   return;
#endif
}

Uint8
SDL_AtomicFetchThenIncrement8(volatile Uint8 * ptr)
{
#ifdef nativeFetchThenIncrement8
   return nativeFetchThenIncrement8(ptr);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)+= 1;
   privateUnlock();

   return tmp;
#endif
}

Uint8
SDL_AtomicFetchThenDecrement8(volatile Uint8 * ptr)
{
#ifdef nativeFetchThenDecrement8
   return nativeFetchThenDecrement8(ptr);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr) -= 1;
   privateUnlock();

   return tmp;
#endif
}

Uint8
SDL_AtomicFetchThenAdd8(volatile Uint8 * ptr, Uint8 value)
{
#ifdef nativeFetchThenAdd8
   return nativeFetchThenAdd8(ptr, value);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)+= value;
   privateUnlock();

   return tmp;
#endif
}

Uint8
SDL_AtomicFetchThenSubtract8(volatile Uint8 * ptr, Uint8 value)
{
#ifdef nativeFetchThenSubtract8
   return nativeFetchThenSubtract8(ptr, value);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)-= value;
   privateUnlock();

   return tmp;
#endif
}

Uint8
SDL_AtomicIncrementThenFetch8(volatile Uint8 * ptr)
{
#ifdef nativeIncrementThenFetch8
   return nativeIncrementThenFetch8(ptr);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   (*ptr)+= 1;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint8
SDL_AtomicDecrementThenFetch8(volatile Uint8 * ptr)
{
#ifdef nativeDecrementThenFetch8
   return nativeDecrementThenFetch8(ptr);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   (*ptr)-= 1;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint8
SDL_AtomicAddThenFetch8(volatile Uint8 * ptr, Uint8 value)
{
#ifdef nativeAddThenFetch8
   return nativeAddThenFetch8(ptr, value);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   (*ptr)+= value;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint8
SDL_AtomicSubtractThenFetch8(volatile Uint8 * ptr, Uint8 value)
{
#ifdef nativeSubtractThenFetch8
   return nativeSubtractThenFetch8(ptr, value);
#else
   Uint8 tmp = 0;

   privateWaitLock();
   (*ptr)-= value;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

/* 16 bit atomic operations */

Uint16
SDL_AtomicExchange16(volatile Uint16 * ptr, Uint16 value)
{
#ifdef nativeExchange16
   return nativeExchange16(ptr, value);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   *ptr = value;
   privateUnlock();

   return tmp;
#endif
}

SDL_bool
SDL_AtomicCompareThenSet16(volatile Uint16 * ptr, Uint16 oldvalue, Uint16 newvalue)
{
#ifdef nativeCompareThenSet16
   return (SDL_bool)nativeCompareThenSet16(ptr, oldvalue, newvalue);
#else
   SDL_bool result = SDL_FALSE;

   privateWaitLock();
   result = (*ptr == oldvalue);
   if (result)
   {
      *ptr = newvalue;
   }
   privateUnlock();

   return result;
#endif
}

SDL_bool
SDL_AtomicTestThenSet16(volatile Uint16 * ptr)
{
#ifdef nativeTestThenSet16
   return (SDL_bool)nativeTestThenSet16(ptr);
#else
   SDL_bool result = SDL_FALSE;

   privateWaitLock();
   result = (*ptr == 0);
   if (result)
   {
      *ptr = 1;
   }
   privateUnlock();

   return result;
#endif
}

void
SDL_AtomicClear16(volatile Uint16 * ptr)
{
#ifdef nativeClear16
   nativeClear16(ptr);
#else
   privateWaitLock();
   *ptr = 0;
   privateUnlock();

   return;
#endif
}

Uint16
SDL_AtomicFetchThenIncrement16(volatile Uint16 * ptr)
{
#ifdef nativeFetchThenIncrement16
   return nativeFetchThenIncrement16(ptr);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)+= 1;
   privateUnlock();

   return tmp;
#endif
}

Uint16
SDL_AtomicFetchThenDecrement16(volatile Uint16 * ptr)
{
#ifdef nativeFetchThenDecrement16
   return nativeFetchThenDecrement16(ptr);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr) -= 1;
   privateUnlock();

   return tmp;
#endif
}

Uint16
SDL_AtomicFetchThenAdd16(volatile Uint16 * ptr, Uint16 value)
{
#ifdef nativeFetchThenAdd16
   return nativeFetchThenAdd16(ptr, value);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)+= value;
   privateUnlock();

   return tmp;
#endif
}

Uint16
SDL_AtomicFetchThenSubtract16(volatile Uint16 * ptr, Uint16 value)
{
#ifdef nativeFetchThenSubtract16
   return nativeFetchThenSubtract16(ptr, value);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)-= value;
   privateUnlock();

   return tmp;
#endif
}

Uint16
SDL_AtomicIncrementThenFetch16(volatile Uint16 * ptr)
{
#ifdef nativeIncrementThenFetch16
   return nativeIncrementThenFetch16(ptr);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   (*ptr)+= 1;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint16
SDL_AtomicDecrementThenFetch16(volatile Uint16 * ptr)
{
#ifdef nativeDecrementThenFetch16
   return nativeDecrementThenFetch16(ptr);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   (*ptr)-= 1;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint16
SDL_AtomicAddThenFetch16(volatile Uint16 * ptr, Uint16 value)
{
#ifdef nativeAddThenFetch16
   return nativeAddThenFetch16(ptr, value);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   (*ptr)+= value;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint16
SDL_AtomicSubtractThenFetch16(volatile Uint16 * ptr, Uint16 value)
{
#ifdef nativeSubtractThenFetch16
   return nativeSubtractThenFetch16(ptr, value);
#else
   Uint16 tmp = 0;

   privateWaitLock();
   (*ptr)-= value;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

/* 64 bit atomic operations */
#ifdef SDL_HAS_64BIT_TYPE

Uint64
SDL_AtomicExchange64(volatile Uint64 * ptr, Uint64 value)
{
#ifdef nativeExchange64
   return nativeExchange64(ptr, value);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   *ptr = value;
   privateUnlock();

   return tmp;
#endif
}

SDL_bool
SDL_AtomicCompareThenSet64(volatile Uint64 * ptr, Uint64 oldvalue, Uint64 newvalue)
{
#ifdef nativeCompareThenSet64
   return (SDL_bool)nativeCompareThenSet64(ptr, oldvalue, newvalue);
#else
   SDL_bool result = SDL_FALSE;

   privateWaitLock();
   result = (*ptr == oldvalue);
   if (result)
   {
      *ptr = newvalue;
   }
   privateUnlock();

   return result;
#endif
}

SDL_bool
SDL_AtomicTestThenSet64(volatile Uint64 * ptr)
{
#ifdef nativeTestThenSet64
   return (SDL_bool)nativeTestThenSet64(ptr);
#else
   SDL_bool result = SDL_FALSE;

   privateWaitLock();
   result = (*ptr == 0);
   if (result)
   {
      *ptr = 1;
   }
   privateUnlock();

   return result;
#endif
}

void
SDL_AtomicClear64(volatile Uint64 * ptr)
{
#ifdef nativeClear64
   nativeClear64(ptr);
#else
   privateWaitLock();
   *ptr = 0;
   privateUnlock();

   return;
#endif
}

Uint64
SDL_AtomicFetchThenIncrement64(volatile Uint64 * ptr)
{
#ifdef nativeFetchThenIncrement64
   return nativeFetchThenIncrement64(ptr);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)+= 1;
   privateUnlock();

   return tmp;
#endif
}

Uint64
SDL_AtomicFetchThenDecrement64(volatile Uint64 * ptr)
{
#ifdef nativeFetchThenDecrement64
   return nativeFetchThenDecrement64(ptr);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr) -= 1;
   privateUnlock();

   return tmp;
#endif
}

Uint64
SDL_AtomicFetchThenAdd64(volatile Uint64 * ptr, Uint64 value)
{
#ifdef nativeFetchThenAdd64
   return nativeFetchThenAdd64(ptr, value);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)+= value;
   privateUnlock();

   return tmp;
#endif
}

Uint64
SDL_AtomicFetchThenSubtract64(volatile Uint64 * ptr, Uint64 value)
{
#ifdef nativeFetchThenSubtract64
   return nativeFetchThenSubtract64(ptr, value);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)-= value;
   privateUnlock();

   return tmp;
#endif
}

Uint64
SDL_AtomicIncrementThenFetch64(volatile Uint64 * ptr)
{
#ifdef nativeIncrementThenFetch64
   return nativeIncrementThenFetch64(ptr);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   (*ptr)+= 1;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint64
SDL_AtomicDecrementThenFetch64(volatile Uint64 * ptr)
{
#ifdef nativeDecrementThenFetch64
   return nativeDecrementThenFetch64(ptr);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   (*ptr)-= 1;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint64
SDL_AtomicAddThenFetch64(volatile Uint64 * ptr, Uint64 value)
{
#ifdef nativeAddThenFetch64
   return nativeAddThenFetch64(ptr, value);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   (*ptr)+= value;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint64
SDL_AtomicSubtractThenFetch64(volatile Uint64 * ptr, Uint64 value)
{
#ifdef nativeSubtractThenFetch64
   return nativeSubtractThenFetch64(ptr, value);
#else
   Uint64 tmp = 0;

   privateWaitLock();
   (*ptr)-= value;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}
#endif

/* QNX native 32 bit atomic operations */

Uint32
SDL_AtomicExchange32(volatile Uint32 * ptr, Uint32 value)
{
   Uint32 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   *ptr = value;
   privateUnlock();

   return tmp;
}

SDL_bool
SDL_AtomicCompareThenSet32(volatile Uint32 * ptr, Uint32 oldvalue, Uint32 newvalue)
{
   SDL_bool result = SDL_FALSE;

   privateWaitLock();
   result = (*ptr == oldvalue);
   if (result)
   {
      *ptr = newvalue;
   }
   privateUnlock();

   return result;
}

SDL_bool
SDL_AtomicTestThenSet32(volatile Uint32 * ptr)
{
   SDL_bool result = SDL_FALSE;

   privateWaitLock();
   result = (*ptr == 0);
   if (result)
   {
      *ptr = 1;
   }
   privateUnlock();

   return result;
}

void
SDL_AtomicClear32(volatile Uint32 * ptr)
{
   atomic_clr(ptr, 0xFFFFFFFF);
}

Uint32
SDL_AtomicFetchThenIncrement32(volatile Uint32 * ptr)
{
   return atomic_add_value(ptr, 0x00000001);
}

Uint32
SDL_AtomicFetchThenDecrement32(volatile Uint32 * ptr)
{
   return atomic_sub_value(ptr, 0x00000001);
}

Uint32
SDL_AtomicFetchThenAdd32(volatile Uint32 * ptr, Uint32 value)
{
   return atomic_add_value(ptr, value);
}

Uint32
SDL_AtomicFetchThenSubtract32(volatile Uint32 * ptr, Uint32 value)
{
   return atomic_sub_value(ptr, value);
}

Uint32
SDL_AtomicIncrementThenFetch32(volatile Uint32 * ptr)
{
   atomic_add(ptr, 0x00000001);
   return atomic_add_value(ptr, 0x00000000);
}

Uint32
SDL_AtomicDecrementThenFetch32(volatile Uint32 * ptr)
{
   atomic_sub(ptr, 0x00000001);
   return atomic_sub_value(ptr, 0x00000000);
}

Uint32
SDL_AtomicAddThenFetch32(volatile Uint32 * ptr, Uint32 value)
{
   atomic_add(ptr, value);
   return atomic_add_value(ptr, 0x00000000);
}

Uint32
SDL_AtomicSubtractThenFetch32(volatile Uint32 * ptr, Uint32 value)
{
   atomic_sub(ptr, value);
   return atomic_sub_value(ptr, 0x00000000);
}
