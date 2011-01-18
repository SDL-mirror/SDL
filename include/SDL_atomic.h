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

/**
 * \file SDL_atomic.h
 * 
 * Atomic operations.
 * 
 * IMPORTANT:
 * If you are not an expert in concurrent lockless programming, you should
 * only be using the atomic lock and reference counting functions in this
 * file.  In all other cases you should be protecting your data structures
 * with full mutexes.
 * 
 * The list of "safe" functions to use are:
 *  SDL_AtomicLock()
 *  SDL_AtomicUnlock()
 *  SDL_AtomicIncRef()
 *  SDL_AtomicDecRef()
 * 
 * Seriously, here be dragons!
 *
 * These operations may, or may not, actually be implemented using
 * processor specific atomic operations. When possible they are
 * implemented as true processor specific atomic operations. When that
 * is not possible the are implemented using locks that *do* use the
 * available atomic operations.
 *
 * All of the atomic operations that modify memory are full memory barriers.
 */

#ifndef _SDL_atomic_h_
#define _SDL_atomic_h_

#include "SDL_stdinc.h"
#include "SDL_platform.h"

#include "begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 * \name SDL AtomicLock
 * 
 * The atomic locks are efficient spinlocks using CPU instructions,
 * but are vulnerable to starvation and can spin forever if a thread
 * holding a lock has been terminated.  For this reason you should
 * minimize the code executed inside an atomic lock and never do
 * expensive things like API or system calls while holding them.
 *
 * The atomic locks are not safe to lock recursively.
 *
 * Porting Note:
 * The spin lock functions and type are required and can not be
 * emulated because they are used in the atomic emulation code.
 */
/*@{*/

typedef int SDL_SpinLock;

/**
 * \brief Try to lock a spin lock by setting it to a non-zero value.
 * 
 * \param lock Points to the lock.
 *
 * \return SDL_TRUE if the lock succeeded, SDL_FALSE if the lock is already held.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicTryLock(SDL_SpinLock *lock);

/**
 * \brief Lock a spin lock by setting it to a non-zero value.
 * 
 * \param lock Points to the lock.
 */
extern DECLSPEC void SDLCALL SDL_AtomicLock(SDL_SpinLock *lock);

/**
 * \brief Unlock a spin lock by setting it to 0. Always returns immediately
 *
 * \param lock Points to the lock.
 */
extern DECLSPEC void SDLCALL SDL_AtomicUnlock(SDL_SpinLock *lock);

/*@}*//*SDL AtomicLock*/

/* Platform specific optimized versions of the atomic functions,
 * you can disable these by defining SDL_DISABLE_ATOMIC_INLINE
 */
#ifndef SDL_DISABLE_ATOMIC_INLINE

#if defined(_MSC_VER)
#include <intrin.h>

#define SDL_AtomicSet(a, v)     _InterlockedExchange((long*)&(a)->value, (v))
#define SDL_AtomicGet(a)        ((a)->value)
#define SDL_AtomicAdd(a, v)     _InterlockedExchangeAdd((long*)&(a)->value, (v))
#define SDL_AtomicCAS(a, oldval, newval) (_InterlockedCompareExchange((long*)&(a)->value, (newval), (oldval)) == (oldval))
#define SDL_AtomicSetPtr(a, v)  (void)_InterlockedExchangePointer((a), (v))
#define SDL_AtomicGetPtr(a)     (*(a))
#if _M_IX86
#define SDL_AtomicCASPtr(a, oldval, newval) (_InterlockedCompareExchange((long*)(a), (long)(newval), (long)(oldval)) == (long)(oldval))
#else
#define SDL_AtomicCASPtr(a, oldval, newval) (_InterlockedCompareExchangePointer((a), (newval), (oldval)) == (oldval))
#endif

#elif defined(__MACOSX__)
#include <libkern/OSAtomic.h>

#define SDL_AtomicSet(a, v) \
({                          \
    int oldvalue;           \
                            \
    do {                    \
        oldvalue = (a)->value; \
    } while (!OSAtomicCompareAndSwap32Barrier(oldvalue, v, &(a)->value)); \
                            \
    oldvalue;               \
})
#define SDL_AtomicGet(a)        ((a)->value)
#define SDL_AtomicAdd(a, v) \
({                          \
    int oldvalue;           \
                            \
    do {                    \
        oldvalue = (a)->value; \
    } while (!OSAtomicCompareAndSwap32Barrier(oldvalue, oldvalue+v, &(a)->value)); \
                            \
    oldvalue;               \
})
#define SDL_AtomicCAS(a, oldval, newval) OSAtomicCompareAndSwap32Barrier(oldval, newval, &(a)->value)
#define SDL_AtomicSetPtr(a, v)  (*(a) = v, OSMemoryBarrier())
#define SDL_AtomicGetPtr(a)     (*(a))
#if SIZEOF_VOIDP == 4
#define SDL_AtomicCASPtr(a, oldval, newval) OSAtomicCompareAndSwap32Barrier((int32_t)(oldval), (int32_t)(newval), (int32_t*)(a))
#elif SIZEOF_VOIDP == 8
#define SDL_AtomicCASPtr(a, oldval, newval) OSAtomicCompareAndSwap64Barrier((int64_t)(oldval), (int64_t)(newval), (int64_t*)(a))
#endif

#elif defined(HAVE_GCC_ATOMICS)

#define SDL_AtomicSet(a, v)     __sync_lock_test_and_set(&(a)->value, v)
#define SDL_AtomicGet(a)        ((a)->value)
#define SDL_AtomicAdd(a, v)     __sync_fetch_and_add(&(a)->value, v)
#define SDL_AtomicCAS(a, oldval, newval) __sync_bool_compare_and_swap(&(a)->value, oldval, newval)
#define SDL_AtomicSetPtr(a, v)  (*(a) = v, __sync_synchronize())
#define SDL_AtomicGetPtr(a)     (*(a))
#define SDL_AtomicCASPtr(a, oldval, newval) __sync_bool_compare_and_swap(a, oldval, newval)

#endif

#endif /* !SDL_DISABLE_ATOMIC_INLINE */


/**
 * \brief A type representing an atomic integer value.  It is a struct
 *        so people don't accidentally use numeric operations on it.
 */
#ifndef SDL_atomic_t_defined
typedef struct { int value; } SDL_atomic_t;
#endif

/**
 * \brief Set an atomic variable to a value.
 *
 * \return The previous value of the atomic variable.
 */
#ifndef SDL_AtomicSet
extern DECLSPEC int SDLCALL SDL_AtomicSet(SDL_atomic_t *a, int value);
#endif

/**
 * \brief Get the value of an atomic variable
 */
#ifndef SDL_AtomicGet
extern DECLSPEC int SDLCALL SDL_AtomicGet(SDL_atomic_t *a);
#endif

/**
 * \brief  Add to an atomic variable.
 *
 * \return The previous value of the atomic variable.
 */
#ifndef SDL_AtomicAdd
extern DECLSPEC int SDLCALL SDL_AtomicAdd(SDL_atomic_t *a, int value);
#endif

/**
 * \brief Increment an atomic variable used as a reference count.
 */
#ifndef SDL_AtomicIncRef
extern DECLSPEC void SDLCALL SDL_AtomicIncRef(SDL_atomic_t *a);
#endif

/**
 * \brief Decrement an atomic variable used as a reference count.
 *
 * \return SDL_TRUE if the variable has reached zero after decrementing,
 *         SDL_FALSE otherwise
 */
#ifndef SDL_AtomicDecRef
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicDecRef(SDL_atomic_t *a);
#endif

/**
 * \brief Set an atomic variable to a new value if it is currently an old value.
 *
 * \return SDL_TRUE if the atomic variable was set, SDL_FALSE otherwise.
 *
 * \note If you don't know what this function is for, you shouldn't use it!
*/
#ifndef SDL_AtomicCAS
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicCAS(SDL_atomic_t *a, int oldval, int newval);
#endif

/**
 * \brief Set a pointer to a value atomically.
 */
#ifndef SDL_AtomicSetPtr
extern DECLSPEC void SDLCALL SDL_AtomicSetPtr(void** a, void* value);
#endif

/**
 * \brief Get the value of a pointer atomically.
 */
#ifndef SDL_AtomicGetPtr
extern DECLSPEC void* SDLCALL SDL_AtomicGetPtr(void** a);
#endif

/**
 * \brief Set a pointer to a new value if it is currently an old value.
 *
 * \return SDL_TRUE if the pointer was set, SDL_FALSE otherwise.
 *
 * \note If you don't know what this function is for, you shouldn't use it!
*/
#ifndef SDL_AtomicCASPtr
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicCASPtr(void **a, void *oldval, void *newval);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#include "close_code.h"

#endif /* _SDL_atomic_h_ */

/* vi: set ts=4 sw=4 expandtab: */
