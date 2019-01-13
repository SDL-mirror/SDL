/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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

/*
	Audio interrupts

	Patrice Mandin, Didier Méquignon
 */

	.text

	.globl	_SDL_MintAudio_XbiosInterruptMeasureClock
	.globl	_SDL_MintAudio_clocktics

	.globl	_SDL_MintAudio_XbiosInterrupt
	.globl	_SDL_MintAudio_Dma8Interrupt
	.globl	_SDL_MintAudio_StfaInterrupt

	.globl	_SDL_MintAudio_num_its

/*--- Xbios interrupt vector to measure Falcon external clock ---*/

_SDL_MintAudio_XbiosInterruptMeasureClock:          /* 1 mS */

	/* state DMA sound */
#if defined(__mcoldfire__)
	movel	d0,sp@-
	
	moveql	#0,d0
	btst	d0,0xFFFF8901:w
#else
	btst	#0,0xFFFF8901:w
#endif
	beqs	SDL_MintAudio_EndIntMeasure
	addql	#1,_SDL_MintAudio_clocktics
SDL_MintAudio_EndIntMeasure:

	/* Clear service bit */
#if defined(__mcoldfire__)
	moveql	#5,d0
	bclr	d0,0xFFFFFA0F:w

	movel	sp@+,d0
#else
	bclr	#5,0xFFFFFA0F:w
#endif
	rte

/*--- Interrupt vectors ---*/
/*    Xbios, DMA 8 bits: need to set ISR on MFP */
/*    STFA: do not touch MFP */

_SDL_MintAudio_XbiosInterrupt:
_SDL_MintAudio_Dma8Interrupt:
#if defined(__mcoldfire__)
	lea	sp@(-12),sp
	moveml	d0-d1/a0,sp@
#else
	moveml	d0-d1/a0,sp@-
#endif

	/* Clear service bit, so other MFP interrupts can work */
#if defined(__mcoldfire__)
	moveql	#5,d0
	bclr	d0,0xfffffa0f:w
#else
	bclr	#5,0xfffffa0f:w
#endif

_SDL_MintAudio_StfaInterrupt:
	move.l	_SDL_MintAudio_num_its,d0
	addql	#1,d0
	move.l	d0,_SDL_MintAudio_num_its
 
	cmp.l	#5,d0
	bmi.s	_SDL_MintAudio_nosilence

	move.l	_SDL_MintAudio_itbuffer,d1
	beq.s	_SDL_MintAudio_nosilence
	move.l	d1,a0

	move.l	_SDL_MintAudio_itbuflen,d0
	move.l	_SDL_MintAudio_itsilence,d1
_SDL_MintAudio_fillsilence:
	move.l	d1,a0@+
	subq.l	#1,d0
	bpl.s	_SDL_MintAudio_fillsilence

_SDL_MintAudio_nosilence:
#if defined(__mcoldfire__)
	moveml	sp@,d0-d1/a0
	lea	sp@(12),sp
#else
	moveml	sp@+,d0-d1/a0
#endif
	rte

	.bss

	.even
	.comm	_SDL_MintAudio_clocktics,4
	.comm	_SDL_MintAudio_num_its,4

	.comm	_SDL_MintAudio_itbuffer,4	/* Audio DMA buffer */
	.comm	_SDL_MintAudio_itbuflen,4	/* Length in longword */
	.comm	_SDL_MintAudio_itsilence,4	/* Silence value */
