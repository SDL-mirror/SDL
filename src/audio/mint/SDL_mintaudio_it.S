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

/*
	Audio interrupts

	Patrice Mandin
 */

	.text

	.globl	_SDL_MintAudio_Callback

	.globl	_SDL_MintAudio_GsxbInterrupt
	.globl	_SDL_MintAudio_EmptyGsxbInterrupt
	.globl	_SDL_MintAudio_XbiosInterrupt
	.globl	_SDL_MintAudio_Dma8Interrupt
	.globl	_SDL_MintAudio_StfaInterrupt

	.globl	_SDL_MintAudio_mutex
	.globl	_SDL_MintAudio_audiobuf
	.globl	_SDL_MintAudio_numbuf
	.globl	_SDL_MintAudio_audiosize

	.globl	_SDL_MintAudio_stfa

/*
	How it works:
	- Audio is playing buffer #0 (resp. #1)
	- We must calculate a sample in buffer #1 (resp. #0)
	  so we first call the callback to do it
	- Then we swap the buffers
*/

/*--- GSXB interrupt vector ---*/

	.text
_SDL_MintAudio_GsxbInterrupt:

	/* Check if we are not already running */
	tstw	_SDL_MintAudio_mutex
	bnes	_SDL_MintAudio_EmptyGsxbInterrupt
	notw	_SDL_MintAudio_mutex
	
	/* Swap buffers */
	eorw	#1,_SDL_MintAudio_numbuf

	moveml	d0-d2/a0-a2,sp@-

	/* Callback */
	jsr	_SDL_MintAudio_Callback

	/* Set new buffer */

	moveq	#0,d0
	movel	_SDL_MintAudio_audiosize,d1

	movew	_SDL_MintAudio_numbuf,d0
	lsll	#2,d0
	lea	_SDL_MintAudio_audiobuf,a0
	movel	a0@(d0:l),a1

	lea	a1@(d1:l),a2

	movel	a2,sp@-
	movel	a1,sp@-
	clrw	sp@-
	movew	#131,sp@-
	trap	#14
	lea	sp@(12),sp

	moveml	sp@+,d0-d2/a0-a2

	clrw	_SDL_MintAudio_mutex
_SDL_MintAudio_EmptyGsxbInterrupt:
	rts

/*--- Xbios interrupt vector ---*/

_SDL_MintAudio_XbiosInterrupt:

	/* Reenable interrupts, so other interrupts can work */
	movew	#0x2300,sr

	/* Clear service bit, so other MFP interrupts can work */
	bclr	#5,0xfffffa0f:w

	/* Check if we are not already running */
	tstw	_SDL_MintAudio_mutex
	bnes	SDL_MintAudio_XbiosEnd
	notw	_SDL_MintAudio_mutex
	
	/* Swap buffers */
	eorw	#1,_SDL_MintAudio_numbuf

	moveml	d0-d2/a0-a2,sp@-

	/* Callback */
	jsr	_SDL_MintAudio_Callback

	/* Set new buffer */

	moveq	#0,d0
	movel	_SDL_MintAudio_audiosize,d1

	movew	_SDL_MintAudio_numbuf,d0
	lsll	#2,d0
	lea	_SDL_MintAudio_audiobuf,a0
	movel	a0@(d0:l),a1

	lea	a1@(d1:l),a2

	movel	a2,sp@-
	movel	a1,sp@-
	clrw	sp@-
	movew	#131,sp@-
	trap	#14
	lea	sp@(12),sp

	moveml	sp@+,d0-d2/a0-a2

	clrw	_SDL_MintAudio_mutex
SDL_MintAudio_XbiosEnd:
	rte

/*--- DMA 8 bits interrupt vector ---*/

_SDL_MintAudio_Dma8Interrupt:

	/* Reenable interrupts, so other interrupts can work */
	movew	#0x2300,sr

	/* Clear service bit, so other MFP interrupts can work */
	bclr	#5,0xfffffa0f:w

	/* Check if we are not already running */
	tstw	_SDL_MintAudio_mutex
	bnes	SDL_MintAudio_Dma8End
	notw	_SDL_MintAudio_mutex
	
	/* Swap buffers */
	eorw	#1,_SDL_MintAudio_numbuf

	moveml	d0-d1/a0-a1,sp@-

	/* Callback */
	jsr	_SDL_MintAudio_Callback

	/* Set new buffer */

	moveq	#0,d0

	movew	_SDL_MintAudio_numbuf,d0
	lslw	#2,d0
	lea	_SDL_MintAudio_audiobuf,a0
	movel	a0@(d0:w),d1

	/* Modify DMA addresses */
	lea	0xffff8900:w,a0

	moveb	d1,a0@(0x07)	/* Start address */
	rorl	#8,d1
	moveb	d1,a0@(0x05)
	rorl	#8,d1
	moveb	d1,a0@(0x03)
	swap	d1

	addl	_SDL_MintAudio_audiosize,d1

	moveb	d1,a0@(0x13)	/* End address */
	rorl	#8,d1
	moveb	d1,a0@(0x11)
	rorl	#8,d1
	moveb	d1,a0@(0x0f)

	moveml	sp@+,d0-d1/a0-a1

	clrw	_SDL_MintAudio_mutex
SDL_MintAudio_Dma8End:
	rte

/*--- STFA interrupt vector ---*/

STFA_SOUND_START	=	6
STFA_SOUND_END		=	STFA_SOUND_START+8

_SDL_MintAudio_StfaInterrupt:

	/* Reenable interrupts, so other interrupts can work */
	movew	#0x2300,sr

	/* Check if we are not already running */
	tstw	_SDL_MintAudio_mutex
	bnes	SDL_MintAudio_StfaEnd
	notw	_SDL_MintAudio_mutex
	
	/* Swap buffers */
	eorw	#1,_SDL_MintAudio_numbuf

	moveml	d0-d1/a0-a1,sp@-

	/* Callback */
	jsr	_SDL_MintAudio_Callback

	/* Set new buffer */

	moveq	#0,d0
	movel	_SDL_MintAudio_stfa,a1

	movew	_SDL_MintAudio_numbuf,d0
	lslw	#2,d0
	lea	_SDL_MintAudio_audiobuf,a0
	movel	a0@(d0:w),d1

	/* Modify STFA replay buffers */
	movel	d1,a1@(STFA_SOUND_START)
	addl	_SDL_MintAudio_audiosize,d1
	movel	d1,a1@(STFA_SOUND_END)

	moveml	sp@+,d0-d1/a0-a1

	clrw	_SDL_MintAudio_mutex
SDL_MintAudio_StfaEnd:
	rte
