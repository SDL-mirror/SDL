/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

#ifdef unix /* FIXME: Better setjmp detection? */
#define USE_SETJMP
#include <signal.h>
#include <setjmp.h>
#endif

#include "SDL.h"
#include "SDL_cpuinfo.h"

#ifdef MACOSX
#include <sys/sysctl.h> /* For AltiVec check */
#endif

#define CPU_HAS_RDTSC	0x00000001
#define CPU_HAS_MMX	0x00000002
#define CPU_HAS_MMXEXT	0x00000004
#define CPU_HAS_3DNOW	0x00000010
#define CPU_HAS_3DNOWEXT 0x00000020
#define CPU_HAS_SSE	0x00000040
#define CPU_HAS_SSE2	0x00000080
#define CPU_HAS_ALTIVEC	0x00000100

#ifdef USE_SETJMP
/* This is the brute force way of detecting instruction sets...
   the idea is borrowed from the libmpeg2 library - thanks!
 */
static jmp_buf jmpbuf;
static void illegal_instruction(int sig)
{
	longjmp(jmpbuf, 1);
}
#endif // USE_SETJMP

static __inline__ int CPU_haveCPUID()
{
	int has_CPUID = 0;
#if defined(__GNUC__) && defined(i386)
	__asm__ (
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
	: "=m" (has_CPUID)
	:
	: "%eax", "%ecx"
	);
#elif defined(_MSC_VER)
	__asm {
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
"        movl    %%ebx,%%edi\n"
"        xorl    %%eax,%%eax         # Set up for CPUID instruction    \n"
"        cpuid                       # Get and save vendor ID          \n"
"        cmpl    $1,%%eax            # Make sure 1 is valid input for CPUID\n"
"        jl      1f                  # We dont have the CPUID instruction\n"
"        xorl    %%eax,%%eax                                           \n"
"        incl    %%eax                                                 \n"
"        cpuid                       # Get family/model/stepping/features\n"
"        movl    %%edx,%0                                              \n"
"1:                                                                    \n"
"        movl    %%edi,%%ebx\n"
	: "=m" (features)
	:
	: "%eax", "%ebx", "%ecx", "%edx", "%edi"
	);
#elif defined(_MSC_VER)
	__asm {
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

static __inline__ int CPU_getCPUIDFeaturesExt()
{
	int features = 0;
#if defined(__GNUC__) && defined(i386)
	__asm__ (
"        movl    %%ebx,%%edi\n"
"        movl    $0x80000000,%%eax   # Query for extended functions    \n"
"        cpuid                       # Get extended function limit     \n"
"        cmpl    $0x80000001,%%eax                                     \n"
"        jl      1f                  # Nope, we dont have function 800000001h\n"
"        movl    $0x80000001,%%eax   # Setup extended function 800000001h\n"
"        cpuid                       # and get the information         \n"
"        movl    %%edx,%0                                              \n"
"1:                                                                    \n"
"        movl    %%edi,%%ebx\n"
	: "=m" (features)
	:
	: "%eax", "%ebx", "%ecx", "%edx", "%edi"
	);
#elif defined(_MSC_VER)
	__asm {
        mov     eax,80000000h       ; Query for extended functions
        cpuid                       ; Get extended function limit
        cmp     eax,80000001h
        jl      done                ; Nope, we dont have function 800000001h
        mov     eax,80000001h       ; Setup extended function 800000001h
        cpuid                       ; and get the information
        mov     features,edx
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

static __inline__ int CPU_haveMMXExt()
{
	if ( CPU_haveCPUID() ) {
		return (CPU_getCPUIDFeaturesExt() & 0x00400000);
	}
	return 0;
}

static __inline__ int CPU_have3DNow()
{
	if ( CPU_haveCPUID() ) {
		return (CPU_getCPUIDFeaturesExt() & 0x80000000);
	}
	return 0;
}

static __inline__ int CPU_have3DNowExt()
{
	if ( CPU_haveCPUID() ) {
		return (CPU_getCPUIDFeaturesExt() & 0x40000000);
	}
	return 0;
}

static __inline__ int CPU_haveSSE()
{
	if ( CPU_haveCPUID() ) {
		return (CPU_getCPUIDFeatures() & 0x02000000);
	}
	return 0;
}

static __inline__ int CPU_haveSSE2()
{
	if ( CPU_haveCPUID() ) {
		return (CPU_getCPUIDFeatures() & 0x04000000);
	}
	return 0;
}

static __inline__ int CPU_haveAltiVec()
{
	volatile int altivec = 0;
#ifdef MACOSX
	int selectors[2] = { CTL_HW, HW_VECTORUNIT }; 
	int hasVectorUnit = 0; 
	size_t length = sizeof(hasVectorUnit); 
	int error = sysctl(selectors, 2, &hasVectorUnit, &length, NULL, 0); 
	if( 0 == error )
		altivec = (hasVectorUnit != 0); 
#elif defined(USE_SETJMP) && defined(GCC_ALTIVEC)
	void (*handler)(int sig);
	handler = signal(SIGILL, illegal_instruction);
	if ( setjmp(jmpbuf) == 0 ) {
		asm volatile ("mtspr 256, %0\n\t"
			      "vand %%v0, %%v0, %%v0"
			      :
			      : "r" (-1));
		altivec = 1;
	}
	signal(SIGILL, handler);
#endif
	return altivec; 
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
		if ( CPU_haveMMXExt() ) {
			SDL_CPUFeatures |= CPU_HAS_MMXEXT;
		}
		if ( CPU_have3DNow() ) {
			SDL_CPUFeatures |= CPU_HAS_3DNOW;
		}
		if ( CPU_have3DNowExt() ) {
			SDL_CPUFeatures |= CPU_HAS_3DNOWEXT;
		}
		if ( CPU_haveSSE() ) {
			SDL_CPUFeatures |= CPU_HAS_SSE;
		}
		if ( CPU_haveSSE2() ) {
			SDL_CPUFeatures |= CPU_HAS_SSE2;
		}
		if ( CPU_haveAltiVec() ) {
			SDL_CPUFeatures |= CPU_HAS_ALTIVEC;
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

SDL_bool SDL_HasMMXExt()
{
	if ( SDL_GetCPUFeatures() & CPU_HAS_MMXEXT ) {
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

SDL_bool SDL_Has3DNowExt()
{
	if ( SDL_GetCPUFeatures() & CPU_HAS_3DNOWEXT ) {
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

SDL_bool SDL_HasSSE2()
{
	if ( SDL_GetCPUFeatures() & CPU_HAS_SSE2 ) {
		return SDL_TRUE;
	}
	return SDL_FALSE;
}

SDL_bool SDL_HasAltiVec()
{
	if ( SDL_GetCPUFeatures() & CPU_HAS_ALTIVEC ) {
		return SDL_TRUE;
	}
	return SDL_FALSE;
}

#ifdef TEST_MAIN

#include <stdio.h>

int main()
{
	printf("RDTSC: %d\n", SDL_HasRDTSC());
	printf("MMX: %d\n", SDL_HasMMX());
	printf("MMXExt: %d\n", SDL_HasMMXExt());
	printf("3DNow: %d\n", SDL_Has3DNow());
	printf("3DNowExt: %d\n", SDL_Has3DNowExt());
	printf("SSE: %d\n", SDL_HasSSE());
	printf("SSE2: %d\n", SDL_HasSSE2());
	printf("AltiVec: %d\n", SDL_HasAltiVec());
	return 0;
}

#endif /* TEST_MAIN */
