/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* CPU feature detection for SDL */

#include "SDL.h"
#include "SDL_cpuinfo.h"

#define CPU_HAS_RDTSC	0x00000001
#define CPU_HAS_MMX	0x00000002
#define CPU_HAS_3DNOW	0x00000004
#define CPU_HAS_SSE	0x00000008

static __inline__ int CPU_haveCPUID()
{
	int has_CPUID = 0;
#if defined(__GNUC__) && defined(i386)
	__asm__ (
"push %%ecx\n"
"        pushfl                      # Get original EFLAGS             \n"
"        popl    %%eax                                                 \n"
"        movl    %%eax,%%ecx                                           \n"
"        xorl    $0x200000,%%eax     # Flip ID bit in EFLAGS           \n"
"        pushl   %%eax               # Save new EFLAGS value on stack  \n"
"        popfl                       # Replace current EFLAGS value    \n"
"        pushfl                      # Get new EFLAGS                  \n"
"        popl    %%eax               # Store new EFLAGS in EAX         \n"
"        xorl    %%ecx,%%eax         # Can not toggle ID bit,          \n"
"        jz      1f                  # Processor=80486                 \n"
"        movl    $1,%0               # We have CPUID support           \n"
"1:                                                                    \n"
"pop %%ecx\n"
	: "=r" (has_CPUID)
	:
	: "%eax", "%ecx"
	);
#elif defined(_MSC_VER)
	__asm__ {
        pushfd                      ; Get original EFLAGS
        pop     eax
        mov     ecx, eax
        xor     eax, 200000h        ; Flip ID bit in EFLAGS
        push    eax                 ; Save new EFLAGS value on stack
        popfd                       ; Replace current EFLAGS value
        pushfd                      ; Get new EFLAGS
        pop     eax                 ; Store new EFLAGS in EAX
        xor     eax, ecx            ; Can not toggle ID bit,
        jz      done                ; Processor=80486
        mov     has_CPUID,1         ; We have CPUID support
done:
	}
#endif
	return has_CPUID;
}

static __inline__ int CPU_getCPUIDFeatures()
{
	int features = 0;
#if defined(__GNUC__) && defined(i386)
	__asm__ (
"push %%ebx\n"
"push %%ecx\n"
"push %%edx\n"
"        xorl    %%eax,%%eax         # Set up for CPUID instruction    \n"
"        cpuid                       # Get and save vendor ID          \n"
"        cmpl    $1,%%eax            # Make sure 1 is valid input for CPUID\n"
"        jl      1f                  # We dont have the CPUID instruction\n"
"        xorl    %%eax,%%eax                                           \n"
"        incl    %%eax                                                 \n"
"        cpuid                       # Get family/model/stepping/features\n"
"        movl    %%edx,%0                                              \n"
"1:                                                                    \n"
"pop %%edx\n"
"pop %%ecx\n"
"pop %%ebx\n"
	: "=r" (features)
	:
	: "%eax", "%ebx", "%ecx", "%edx"
	);
#elif defined(_MSC_VER)
	__asm__ {
        xor     eax, eax            ; Set up for CPUID instruction
        cpuid                       ; Get and save vendor ID
        cmp     eax, 1              ; Make sure 1 is valid input for CPUID
        jl      done                ; We dont have the CPUID instruction
        xor     eax, eax
        inc     eax
        cpuid                       ; Get family/model/stepping/features
        mov     features, edx
done:
	}
#endif
	return features;
}

static __inline__ int CPU_haveRDTSC()
{
	if ( CPU_haveCPUID() ) {
		return (CPU_getCPUIDFeatures() & 0x00000010);
	}
	return 0;
}

static __inline__ int CPU_haveMMX()
{
	if ( CPU_haveCPUID() ) {
		return (CPU_getCPUIDFeatures() & 0x00800000);
	}
	return 0;
}

static __inline__ int CPU_have3DNow()
{
	int has_3DNow = 0;
#if defined(__GNUC__) && defined(i386)
	__asm__ (
"push %%ebx\n"
"push %%ecx\n"
"push %%edx\n"
"        movl    $0x80000000,%%eax   # Query for extended functions    \n"
"        cpuid                       # Get extended function limit     \n"
"        cmpl    $0x80000001,%%eax                                     \n"
"        jbe     1f                  # Nope, we dont have function 800000001h\n"
"        movl    $0x80000001,%%eax   # Setup extended function 800000001h\n"
"        cpuid                       # and get the information         \n"
"        testl   $0x80000000,%%edx   # Bit 31 is set if 3DNow! present \n"
"        jz      1f                  # Nope, we dont have 3DNow support\n"
"        movl    $1,%0               # Yep, we have 3DNow! support!    \n"
"1:                                                                    \n"
"pop %%edx\n"
"pop %%ecx\n"
"pop %%ebx\n"
	: "=r" (has_3DNow)
	:
	: "%eax", "%ebx", "%ecx", "%edx"
	);
#elif defined(_MSC_VER)
	__asm__ {
        mov     eax,80000000h       ; Query for extended functions
        cpuid                       ; Get extended function limit
        cmp     eax,80000001h
        jbe     done                ; Nope, we dont have function 800000001h
        mov     eax,80000001h       ; Setup extended function 800000001h
        cpuid                       ; and get the information
        test    edx,80000000h       ; Bit 31 is set if 3DNow! present
        jz      done                ; Nope, we dont have 3DNow support
        mov     has_3DNow,1         ; Yep, we have 3DNow! support!
done:
	}
#endif
	return has_3DNow;
}

static __inline__ int CPU_haveSSE()
{
	if ( CPU_haveCPUID() ) {
		return (CPU_getCPUIDFeatures() & 0x02000000);
	}
	return 0;
}

static Uint32 SDL_CPUFeatures = 0xFFFFFFFF;

static Uint32 SDL_GetCPUFeatures()
{
	if ( SDL_CPUFeatures == 0xFFFFFFFF ) {
		SDL_CPUFeatures = 0;
		if ( CPU_haveRDTSC() ) {
			SDL_CPUFeatures |= CPU_HAS_RDTSC;
		}
		if ( CPU_haveMMX() ) {
			SDL_CPUFeatures |= CPU_HAS_MMX;
		}
		if ( CPU_have3DNow() ) {
			SDL_CPUFeatures |= CPU_HAS_3DNOW;
		}
		if ( CPU_haveSSE() ) {
			SDL_CPUFeatures |= CPU_HAS_SSE;
		}
	}
	return SDL_CPUFeatures;
}

SDL_bool SDL_HasRDTSC()
{
	if ( SDL_GetCPUFeatures() & CPU_HAS_RDTSC ) {
		return SDL_TRUE;
	}
	return SDL_FALSE;
}

SDL_bool SDL_HasMMX()
{
	if ( SDL_GetCPUFeatures() & CPU_HAS_MMX ) {
		return SDL_TRUE;
	}
	return SDL_FALSE;
}

SDL_bool SDL_Has3DNow()
{
	if ( SDL_GetCPUFeatures() & CPU_HAS_3DNOW ) {
		return SDL_TRUE;
	}
	return SDL_FALSE;
}

SDL_bool SDL_HasSSE()
{
	if ( SDL_GetCPUFeatures() & CPU_HAS_SSE ) {
		return SDL_TRUE;
	}
	return SDL_FALSE;
}

#ifdef TEST_MAIN

#include <stdio.h>

int main()
{
	printf("MMX: %d\n", SDL_HasMMX());
	printf("3DNow: %d\n", SDL_Has3DNow());
	printf("SSE: %d\n", SDL_HasSSE());
	return 0;
}

#endif /* TEST_MAIN */
