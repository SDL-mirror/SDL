/*
 *	Audio interrupt routines
 *
 *	Patrice Mandin
 */

	.text

	.globl	_SDL_MintAudio_Callback

	.globl	_SDL_MintAudio_IntDma
	.globl	_SDL_MintAudio_IntXbios
	.globl	_SDL_MintAudio_IntGsxb

	.globl	_SDL_MintAudio_mutex
	.globl	_SDL_MintAudio_audiobuf
	.globl	_SDL_MintAudio_numbuf
	.globl	_SDL_MintAudio_audiosize

/*
	How it works:
	- Audio is playing buffer #0 (resp. #1)
	- We must calculate a sample in buffer #1 (resp. #0)
	  so we first call the callback to do it
	- Then we swap the buffers
*/

/*--- DMA 8 bits interrupt vector ---*/

	.text
_SDL_MintAudio_IntDma:

	/* Reenable interrupts, so other interrupts can work */
	movew	#0x2300,sr

	/* Clear service bit, so other MFP interrupts can work */
	bclr	#5,0xfffffa0f:w

	/* Check if we are not already running */
	tstw	_SDL_MintAudio_mutex
	bnes	intdma_end
	notw	_SDL_MintAudio_mutex
	
	/* Swap buffers */
	moveml	d0-d1/a0-a1,sp@-

	movel	_SDL_MintAudio_numbuf,d0
	movel	_SDL_MintAudio_audiobuf,d1
	eorl	#1,d0
	beqs	intdma_buffer0
	movel	_SDL_MintAudio_audiobuf+4,d1
intdma_buffer0:
	movel	d0,_SDL_MintAudio_numbuf

	/* Callback */
	movel	d1,sp@-
	jsr	_SDL_MintAudio_Callback
	movel	sp@+,d1

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
intdma_end:
	rte

/*--- Xbios interrupt vector ---*/

	.text
_SDL_MintAudio_IntXbios:

	/* Reenable interrupts, so other interrupts can work */
	movew	#0x2300,sr

	/* Clear service bit, so other MFP interrupts can work */
	bclr	#5,0xfffffa0f:w

	/* Check if we are not already running */
	tstw	_SDL_MintAudio_mutex
	bnes	intxbios_end
	notw	_SDL_MintAudio_mutex
	
	/* Swap buffers */
	moveml	d0-d2/a0-a2,sp@-

	movel	_SDL_MintAudio_numbuf,d0
	movel	_SDL_MintAudio_audiobuf,a0
	eorl	#1,d0
	beqs	intxbios_buffer0
	movel	_SDL_MintAudio_audiobuf+4,a0
intxbios_buffer0:
	movel	d0,_SDL_MintAudio_numbuf

	/* Callback */
	moveml	a0,sp@-
	jsr	_SDL_MintAudio_Callback
	moveml	sp@+,a0

	/* Setbuffer xbios function */
	movel	_SDL_MintAudio_audiosize,d1
	lea	a0@(0,d1:l),a1

	movel	a1,sp@-
	movel	a0,sp@-
	clrw	sp@-
	movew	#131,sp@-
	trap	#14
	lea	sp@(12),sp

	moveml	sp@+,d0-d2/a0-a2

	clrw	_SDL_MintAudio_mutex
intxbios_end:
	rte

/*--- GSXB interrupt vector ---*/

	.text
_SDL_MintAudio_IntGsxb:

	/* Check if we are not already running */
	tstw	_SDL_MintAudio_mutex
	bnes	intgsxb_end
	notw	_SDL_MintAudio_mutex

	/* Swap buffers */
	moveml	d0-d2/a0-a2,sp@-

	movel	_SDL_MintAudio_numbuf,d0
	movel	_SDL_MintAudio_audiobuf,a0
	eorl	#1,d0
	beqs	intgsxb_buffer0
	movel	_SDL_MintAudio_audiobuf+4,a0
intgsxb_buffer0:
	movel	d0,_SDL_MintAudio_numbuf

	/* Callback */
	moveml	a0,sp@-
	jsr	_SDL_MintAudio_Callback
	moveml	sp@+,a0

	/* Setbuffer xbios function */
	movel	_SDL_MintAudio_audiosize,d1
	lea	a0@(0,d1:l),a1

	movel	a1,sp@-
	movel	a0,sp@-
	clrw	sp@-
	movew	#131,sp@-
	trap	#14
	lea	sp@(12),sp

	moveml	sp@+,d0-d2/a0-a2

	clrw	_SDL_MintAudio_mutex
intgsxb_end:
	rts

/*--- Common variables ---*/

	.data
	.even
	.comm	_SDL_MintAudio_mutex,4
	.even
	.comm	_SDL_MintAudio_audiobuf,8
	.even
	.comm	_SDL_MintAudio_numbuf,4
	.even
	.comm	_SDL_MintAudio_audiosize,4

