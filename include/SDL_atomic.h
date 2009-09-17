/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

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

    Contributed by Bob Pendleton, bob@pendleton.com
 */

/**
 * \file SDL_atomic.h
 *
 * Atomic operations.
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
 * These operations may, or may not, actually be implemented using
 * processor specific atomic operations. When possible they are
 * implemented as true processor specific atomic operations. When that
 * is not possible the are implemented using locks that *do* use the
 * available atomic operations.
 *
 * At the very minimum spin locks must be implemented. Without spin
 * locks it is not possible (AFAICT) to emulate the rest of the atomic
 * operations.
 */

/* Function prototypes */

/**
 * SDL AtomicLock.
 * 
 * The spin lock functions and type are required and can not be
 * emulated because they are used in the emulation code.
 */

typedef volatile Uint32 SDL_SpinLock;

/**
 * \fn  void SDL_AtomicLock(SDL_SpinLock *lock);
 *
 * \brief Lock a spin lock by setting it to a none zero value.
 *
 * \param lock points to the lock.
 *
 */
extern DECLSPEC void SDLCALL SDL_AtomicLock(SDL_SpinLock *lock);

/**
 * \fn  void SDL_AtomicUnlock(SDL_SpinLock *lock);
 *
 * \brief Unlock a spin lock by setting it to 0. Always returns immediately
 *
 * \param lock points to the lock.
 *
 */
extern DECLSPEC void SDLCALL SDL_AtomicUnlock(SDL_SpinLock *lock);

/* 32 bit atomic operations */

/**
 * \fn  SDL_bool SDL_AtomicTestThenSet32(volatile Uint32 * ptr);
 *
 * \brief Check to see if *ptr == 0 and set it to 1.
 *
 * \return SDL_True if the value pointed to by ptr was zero and
 * SDL_False if it was not zero
 *
 * \param ptr points to the value to be tested and set.
 *
 */
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicTestThenSet32(volatile Uint32 * ptr);

/**
 * \fn  void SDL_AtomicClear32(volatile Uint32 * ptr);
 *
 * \brief set the value pointed to by ptr to be zero.
 *
 * \param ptr address of the value to be set to zero
 *
 */
extern DECLSPEC void SDLCALL SDL_AtomicClear32(volatile Uint32 * ptr);

/**
 * \fn  Uint32 SDL_AtomicFetchThenIncrement32(volatile Uint32 * ptr);
 *
 * \brief fetch the current value of *ptr and then increment that
 * value in place.
 *
 * \return the value before it was incremented.
 *
 * \param ptr address of the value to fetch and increment
 *
 */
extern DECLSPEC Uint32 SDLCALL SDL_AtomicFetchThenIncrement32(volatile Uint32 * ptr);

/**
 * \fn  Uint32 SDL_AtomicFetchThenDecrement32(volatile Uint32 * ptr);
 *
 * \brief fetch *ptr and then decrement the value in place.
 *
 * \return the value before it was decremented.
 *
 * \param ptr address of the value to fetch and drement
 *
 */
extern DECLSPEC Uint32 SDLCALL SDL_AtomicFetchThenDecrement32(volatile Uint32 * ptr);

/**
 * \fn  Uint32 SDL_AtomicFetchThenAdd32(volatile Uint32 * ptr, Uint32 value);
 *
 * \brief fetch the current value at ptr and then add value to *ptr.
 *
 * \return *ptr before the addition took place.
 *
 * \param ptr the address of data we are changing.
 * \param value the value to add to *ptr. 
 *
 */
extern DECLSPEC Uint32 SDLCALL SDL_AtomicFetchThenAdd32(volatile Uint32 * ptr, Uint32 value);

/**
 * \fn  Uint32 SDL_AtomicFetchThenSubtract32(volatile Uint32 * ptr, Uint32 value);
 *
 * \brief Fetch *ptr and then subtract value from it.
 *
 * \return *ptr before the subtraction took place.
 *
 * \param ptr the address of the data being changed.
 * \param value the value to subtract from *ptr.
 *
 */
extern DECLSPEC Uint32 SDLCALL SDL_AtomicFetchThenSubtract32(volatile Uint32 * ptr, Uint32 value);

/**
 * \fn  Uint32 SDL_AtomicIncrementThenFetch32(volatile Uint32 * ptr);
 *
 * \brief Add one to the data pointed to by ptr and return that value.
 *
 * \return the incremented value.
 *
 * \param ptr address of the data to increment.
 *
 */
extern DECLSPEC Uint32 SDLCALL SDL_AtomicIncrementThenFetch32(volatile Uint32 * ptr);

/**
 * \fn  Uint32 SDL_AtomicDecrementThenFetch32(volatile Uint32 * ptr);
 *
 * \brief Subtract one from data pointed to by ptr and return the new value.
 *
 * \return The decremented value.
 *
 * \param ptr The address of the data to decrement.
 *
 */
extern DECLSPEC Uint32 SDLCALL SDL_AtomicDecrementThenFetch32(volatile Uint32 * ptr);

/**
 * \fn  Uint32 SDL_AtomicAddThenFetch32(volatile Uint32 * ptr, Uint32 value);
 *
 * \brief Add value to the data pointed to by ptr and return result.
 *
 * \return The sum of *ptr and value.
 *
 * \param ptr The address of the data to be modified.
 * \param value The value to be added.
 *
 */
extern DECLSPEC Uint32 SDLCALL SDL_AtomicAddThenFetch32(volatile Uint32 * ptr, Uint32 value);

/**
 * \fn  Uint32 SDL_AtomicSubtractThenFetch32(volatile Uint32 * ptr, Uint32 value);
 *
 * \brief Subtract value from the data pointed to by ptr and return the result.
 *
 * \return the difference between *ptr and value.
 *
 * \param ptr The address of the data to be modified.
 * \param value The value to be subtracted.
 *
 */
extern DECLSPEC Uint32 SDLCALL SDL_AtomicSubtractThenFetch32(volatile Uint32 * ptr, Uint32 value);

/* 64 bit atomic operations */
#ifdef SDL_HAS_64BIT_TYPE

extern DECLSPEC SDL_bool SDLCALL SDL_AtomicTestThenSet64(volatile Uint64 * ptr);
extern DECLSPEC void SDLCALL SDL_AtomicClear64(volatile Uint64 * ptr);
extern DECLSPEC Uint64 SDLCALL SDL_AtomicFetchThenIncrement64(volatile Uint64 * ptr);
extern DECLSPEC Uint64 SDLCALL SDL_AtomicFetchThenDecrement64(volatile Uint64 * ptr);
extern DECLSPEC Uint64 SDLCALL SDL_AtomicFetchThenAdd64(volatile Uint64 * ptr, Uint64 value);
extern DECLSPEC Uint64 SDLCALL SDL_AtomicFetchThenSubtract64(volatile Uint64 * ptr, Uint64 value);
extern DECLSPEC Uint64 SDLCALL SDL_AtomicIncrementThenFetch64(volatile Uint64 * ptr);
extern DECLSPEC Uint64 SDLCALL SDL_AtomicDecrementThenFetch64(volatile Uint64 * ptr);
extern DECLSPEC Uint64 SDLCALL SDL_AtomicAddThenFetch64(volatile Uint64 * ptr, Uint64 value);
extern DECLSPEC Uint64 SDLCALL SDL_AtomicSubtractThenFetch64(volatile Uint64 * ptr, Uint64 value);
#endif /*  SDL_HAS_64BIT_TYPE */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#include "close_code.h"

#endif /* _SDL_atomic_h_ */

/* vi: set ts=4 sw=4 expandtab: */
