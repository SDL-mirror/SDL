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
   Linux version.

   Test for gnu C builtin support for atomic operations. The only way
   I know of is to check to see if the
   __GCC_HAVE_SYNC_COMPARE_AND_SWAP_* macros are defined.
 */

#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1
#define HAVE_ALL_8_BIT_OPS

#define nativeExchange8(ptr, value)			(__sync_lock_test_and_set(ptr, value))
#define nativeCompareThenSet8(ptr, oldvalue, newvalue) 	(oldvalue == __sync_val_compare_and_swap(ptr, oldvalue, newvalue))
#define nativeTestThenSet8(ptr)    	     		(0 == __sync_lock_test_and_set(ptr, 1))
#define nativeClear8(ptr)				(__sync_lock_release(ptr))
#define nativeFetchThenIncrement8(ptr)   		(__sync_fetch_and_add(ptr, 1))
#define nativeFetchThenDecrement8(ptr) 			(__sync_fetch_and_sub(ptr, 1))
#define nativeFetchThenAdd8(ptr, value) 		(__sync_fetch_and_add(ptr, value))
#define nativeFetchThenSubtract8(ptr, value) 		(__sync_fetch_and_sub(ptr, value))
#define nativeIncrementThenFetch8(ptr) 			(__sync_add_and_fetch(ptr, 1))
#define nativeDecrementThenFetch8(ptr) 			(__sync_sub_and_fetch(ptr, 1))
#define nativeAddThenFetch8(ptr, value) 		(__sync_add_and_fetch(ptr, value))
#define nativeSubtractThenFetch8(ptr, value) 		(__sync_sub_and_fetch(ptr, value))
#endif

#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2
#define HAVE_ALL_16_BIT_OPS

#define nativeExchange16(ptr, value)			(__sync_lock_test_and_set(ptr, value))
#define nativeCompareThenSet16(ptr, oldvalue, newvalue) (oldvalue == __sync_val_compare_and_swap(ptr, oldvalue, newvalue))
#define nativeTestThenSet16(ptr)    	     		(0 == __sync_lock_test_and_set(ptr, 1))
#define nativeClear16(ptr)				(__sync_lock_release(ptr))
#define nativeFetchThenIncrement16(ptr)   		(__sync_fetch_and_add(ptr, 1))
#define nativeFetchThenDecrement16(ptr) 		(__sync_fetch_and_sub(ptr, 1))
#define nativeFetchThenAdd16(ptr, value) 		(__sync_fetch_and_add(ptr, value))
#define nativeFetchThenSubtract16(ptr, value) 		(__sync_fetch_and_sub(ptr, value))
#define nativeIncrementThenFetch16(ptr) 		(__sync_add_and_fetch(ptr, 1))
#define nativeDecrementThenFetch16(ptr) 		(__sync_sub_and_fetch(ptr, 1))
#define nativeAddThenFetch16(ptr, value) 		(__sync_add_and_fetch(ptr, value))
#define nativeSubtractThenFetch16(ptr, value) 		(__sync_sub_and_fetch(ptr, value))
#endif

#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
#define HAVE_ALL_32_BIT_OPS

#define nativeExchange32(ptr, value)			(__sync_lock_test_and_set(ptr, value))
#define nativeCompareThenSet32(ptr, oldvalue, newvalue) (oldvalue == __sync_val_compare_and_swap(ptr, oldvalue, newvalue))
#define nativeTestThenSet32(ptr)    	     		(0 == __sync_lock_test_and_set(ptr, 1))
#define nativeClear32(ptr)				(__sync_lock_release(ptr))
#define nativeFetchThenIncrement32(ptr)   		(__sync_fetch_and_add(ptr, 1))
#define nativeFetchThenDecrement32(ptr) 		(__sync_fetch_and_sub(ptr, 1))
#define nativeFetchThenAdd32(ptr, value) 		(__sync_fetch_and_add(ptr, value))
#define nativeFetchThenSubtract32(ptr, value) 		(__sync_fetch_and_sub(ptr, value))
#define nativeIncrementThenFetch32(ptr) 		(__sync_add_and_fetch(ptr, 1))
#define nativeDecrementThenFetch32(ptr) 		(__sync_sub_and_fetch(ptr, 1))
#define nativeAddThenFetch32(ptr, value) 		(__sync_add_and_fetch(ptr, value))
#define nativeSubtractThenFetch32(ptr, value) 		(__sync_sub_and_fetch(ptr, value))
#endif

#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
#define HAVE_ALL_64_BIT_OPS

#define nativeExchange64(ptr, value)			(__sync_lock_test_and_set(ptr, value))
#define nativeCompareThenSet64(ptr, oldvalue, newvalue) (oldvalue == __sync_val_compare_and_swap(ptr, oldvalue, newvalue))
#define nativeTestThenSet64(ptr)    	     		(0 == __sync_lock_test_and_set(ptr, 1))
#define nativeClear64(ptr)				(__sync_lock_release(ptr))
#define nativeFetchThenIncrement64(ptr)   		(__sync_fetch_and_add(ptr, 1))
#define nativeFetchThenDecrement64(ptr) 		(__sync_fetch_and_sub(ptr, 1))
#define nativeFetchThenAdd64(ptr, value) 		(__sync_fetch_and_add(ptr, value))
#define nativeFetchThenSubtract64(ptr, value) 		(__sync_fetch_and_sub(ptr, value))
#define nativeIncrementThenFetch64(ptr) 		(__sync_add_and_fetch(ptr, 1))
#define nativeDecrementThenFetch64(ptr) 		(__sync_sub_and_fetch(ptr, 1))
#define nativeAddThenFetch64(ptr, value) 		(__sync_add_and_fetch(ptr, value))
#define nativeSubtractThenFetch64(ptr, value) 		(__sync_sub_and_fetch(ptr, value))
#endif

/* 
If any of the operations are not provided then we must emulate some of
them.
 */

#if !defined(HAVE_ALL_8_BIT_OPS) || !defined(HAVE_ALL_16_BIT_OPS) || !defined(HAVE_ALL_32_BIT_OPS) || !defined(HAVE_ALL_64_BIT_OPS)

static Uint32 lock = 0;

#define privateWaitLock()	       \
   while (nativeTestThenSet32(&lock))  \
   {				       \
   };

#define privateUnlock() (nativeClear32(&lock))
#endif

/* 8 bit atomic operations */

Uint8
SDL_AtomicExchange8(Uint8 * ptr, Uint8 value)
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
SDL_AtomicCompareThenSet8(Uint8 * ptr, Uint8 oldvalue, Uint8 newvalue)
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
SDL_AtomicTestThenSet8(Uint8 * ptr)
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
SDL_AtomicClear8(Uint8 * ptr)
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
SDL_AtomicFetchThenIncrement8(Uint8 * ptr)
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
SDL_AtomicFetchThenDecrement8(Uint8 * ptr)
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
SDL_AtomicFetchThenAdd8(Uint8 * ptr, Uint8 value)
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
SDL_AtomicFetchThenSubtract8(Uint8 * ptr, Uint8 value)
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
SDL_AtomicIncrementThenFetch8(Uint8 * ptr)
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
SDL_AtomicDecrementThenFetch8(Uint8 * ptr)
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
SDL_AtomicAddThenFetch8(Uint8 * ptr, Uint8 value)
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
SDL_AtomicSubtractThenFetch8(Uint8 * ptr, Uint8 value)
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
SDL_AtomicExchange16(Uint16 * ptr, Uint16 value)
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
SDL_AtomicCompareThenSet16(Uint16 * ptr, Uint16 oldvalue, Uint16 newvalue)
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
SDL_AtomicTestThenSet16(Uint16 * ptr)
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
SDL_AtomicClear16(Uint16 * ptr)
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
SDL_AtomicFetchThenIncrement16(Uint16 * ptr)
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
SDL_AtomicFetchThenDecrement16(Uint16 * ptr)
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
SDL_AtomicFetchThenAdd16(Uint16 * ptr, Uint16 value)
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
SDL_AtomicFetchThenSubtract16(Uint16 * ptr, Uint16 value)
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
SDL_AtomicIncrementThenFetch16(Uint16 * ptr)
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
SDL_AtomicDecrementThenFetch16(Uint16 * ptr)
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
SDL_AtomicAddThenFetch16(Uint16 * ptr, Uint16 value)
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
SDL_AtomicSubtractThenFetch16(Uint16 * ptr, Uint16 value)
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

/* 32 bit atomic operations */

Uint32
SDL_AtomicExchange32(Uint32 * ptr, Uint32 value)
{
#ifdef nativeExchange32
   return nativeExchange32(ptr, value);
#else
   Uint32 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   *ptr = value;
   privateUnlock();

   return tmp;
#endif
}

SDL_bool
SDL_AtomicCompareThenSet32(Uint32 * ptr, Uint32 oldvalue, Uint32 newvalue)
{
#ifdef nativeCompareThenSet32
   return (SDL_bool)nativeCompareThenSet32(ptr, oldvalue, newvalue);
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
SDL_AtomicTestThenSet32(Uint32 * ptr)
{
#ifdef nativeTestThenSet32
   return (SDL_bool)nativeTestThenSet32(ptr);
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
SDL_AtomicClear32(Uint32 * ptr)
{
#ifdef nativeClear32
   nativeClear32(ptr);
#else
   privateWaitLock();
   *ptr = 0;
   privateUnlock();

   return;
#endif
}

Uint32
SDL_AtomicFetchThenIncrement32(Uint32 * ptr)
{
#ifdef nativeFetchThenIncrement32
   return nativeFetchThenIncrement32(ptr);
#else
   Uint32 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)+= 1;
   privateUnlock();

   return tmp;
#endif
}

Uint32
SDL_AtomicFetchThenDecrement32(Uint32 * ptr)
{
#ifdef nativeFetchThenDecrement32
   return nativeFetchThenDecrement32(ptr);
#else
   Uint32 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr) -= 1;
   privateUnlock();

   return tmp;
#endif
}

Uint32
SDL_AtomicFetchThenAdd32(Uint32 * ptr, Uint32 value)
{
#ifdef nativeFetchThenAdd32
   return nativeFetchThenAdd32(ptr, value);
#else
   Uint32 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)+= value;
   privateUnlock();

   return tmp;
#endif
}

Uint32
SDL_AtomicFetchThenSubtract32(Uint32 * ptr, Uint32 value)
{
#ifdef nativeFetchThenSubtract32
   return nativeFetchThenSubtract32(ptr, value);
#else
   Uint32 tmp = 0;

   privateWaitLock();
   tmp = *ptr;
   (*ptr)-= value;
   privateUnlock();

   return tmp;
#endif
}

Uint32
SDL_AtomicIncrementThenFetch32(Uint32 * ptr)
{
#ifdef nativeIncrementThenFetch32
   return nativeIncrementThenFetch32(ptr);
#else
   Uint32 tmp = 0;

   privateWaitLock();
   (*ptr)+= 1;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint32
SDL_AtomicDecrementThenFetch32(Uint32 * ptr)
{
#ifdef nativeDecrementThenFetch32
   return nativeDecrementThenFetch32(ptr);
#else
   Uint32 tmp = 0;

   privateWaitLock();
   (*ptr)-= 1;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint32
SDL_AtomicAddThenFetch32(Uint32 * ptr, Uint32 value)
{
#ifdef nativeAddThenFetch32
   return nativeAddThenFetch32(ptr, value);
#else
   Uint32 tmp = 0;

   privateWaitLock();
   (*ptr)+= value;
   tmp = *ptr;
   privateUnlock();

   return tmp;
#endif
}

Uint32
SDL_AtomicSubtractThenFetch32(Uint32 * ptr, Uint32 value)
{
#ifdef nativeSubtractThenFetch32
   return nativeSubtractThenFetch32(ptr, value);
#else
   Uint32 tmp = 0;

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
SDL_AtomicExchange64(Uint64 * ptr, Uint64 value)
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
SDL_AtomicCompareThenSet64(Uint64 * ptr, Uint64 oldvalue, Uint64 newvalue)
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
SDL_AtomicTestThenSet64(Uint64 * ptr)
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
SDL_AtomicClear64(Uint64 * ptr)
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
SDL_AtomicFetchThenIncrement64(Uint64 * ptr)
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
SDL_AtomicFetchThenDecrement64(Uint64 * ptr)
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
SDL_AtomicFetchThenAdd64(Uint64 * ptr, Uint64 value)
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
SDL_AtomicFetchThenSubtract64(Uint64 * ptr, Uint64 value)
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
SDL_AtomicIncrementThenFetch64(Uint64 * ptr)
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
SDL_AtomicDecrementThenFetch64(Uint64 * ptr)
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
SDL_AtomicAddThenFetch64(Uint64 * ptr, Uint64 value)
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
SDL_AtomicSubtractThenFetch64(Uint64 * ptr, Uint64 value)
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

