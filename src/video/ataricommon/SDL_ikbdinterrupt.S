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

/*
 *	IKBD 6301 interrupt routine
 *
 *	Patrice Mandin
 */

	.text

	.globl	_SDL_AtariIkbdInstall
	.globl	_SDL_AtariIkbdUninstall

	.globl	_SDL_AtariIkbd_keyboard
	.globl	_SDL_AtariIkbd_mouseb
	.globl	_SDL_AtariIkbd_mousex
	.globl	_SDL_AtariIkbd_mousey
	.globl	_SDL_AtariIkbd_joystick

	.globl	_SDL_AtariIkbd_enabled

/*--- Install our IKBD vector ---*/

_SDL_AtariIkbdInstall:
	moveml	d0-d1/a0-a1,sp@-

	| Init interrupts

	movew	#0x2700,sr

	| Save MFP registers used for keyboard

	lea	0xfffffa00:w,a0
	btst	#6,a0@(0x09)
	sne	ikbd_ierb
	btst	#6,a0@(0x15)
	sne	ikbd_imrb

	| Set our routine

	movel	0x118:w,old_ikbd
	movel	#ikbd,0x118:w
	bset	#6,0xfffffa09:w	| IERB
	bset	#6,0xfffffa15:w	| IMRB

	moveb	#8,0xfffffc02:w

	movew	#0x2300,sr

	| Interrupts done

	movew	#0xffff,_SDL_AtariIkbd_enabled

	moveml	sp@+,d0-d1/a0-a1
	rts

/*--- Uninstall our IKBD vector ---*/

_SDL_AtariIkbdUninstall:
	movel	a0,sp@-

	| Stop interrupt

	movew	#0x2700,sr

	| Restore previous MFP registers

	lea	0xfffffa00:w,a0

	bclr	#6,a0@(0x09)
	tstb	ikbd_ierb
	beq	ikbd_restoreierb
	bset	#6,a0@(0x09)
ikbd_restoreierb:

	bclr	#6,a0@(0x15)
	tstb	ikbd_imrb
	beq	ikbd_restoreimrb
	bset	#6,a0@(0x15)
ikbd_restoreimrb:

	movel	old_ikbd,0x118:w

	| Clear keyboard buffer

	lea	0xfffffc00:w,a0
ikbd_videbuffer:
	btst	#0,a0@
	beq	ikbd_finbuffer
	tstb	a0@(0x02)
	bra	ikbd_videbuffer
ikbd_finbuffer:

	movew	#0x2300,sr

	movel	sp@+,a0
	rts

	.data
	.even
	.comm	ikbd_ierb,4*1
	.even
	.comm	ikbd_imrb,4*1

/*--- Our custom IKBD vector ---*/

	.text
	.even
	.ascii	"XBRA"
	.ascii	"LSDL"
	.comm	old_ikbd,4*1
ikbd:
	moveml	d0-d1/a0,sp@-
	moveb	0xfffffc02:w,d0

	| paquet joystick ?
	
	cmpb	#0xff,d0
	beq	ikbd_yes_joystick

	| paquet souris ?

	cmpb	#0xf8,d0
	bmi	ikbd_no_mouse
	cmpb	#0xfc,d0
	bpl	ikbd_no_mouse

ikbd_yes_mouse:
	andw	#3,d0
	movew	d0,_SDL_AtariIkbd_mouseb

	movel	#ikbd_mousex,0x118:w
	bra	ikbd_endit

ikbd_yes_joystick:
	movel	#ikbd_joystick,0x118:w
	bra	ikbd_endit

ikbd_no_mouse:
	moveb	d0,d1
	lea	_SDL_AtariIkbd_keyboard,a0
	andl	#0x7f,d1
	tas	d0
	spl	a0@(0,d1:w)
ikbd_fin:

	| Fin interruption

ikbd_endit:
	moveml	sp@+,d0-d1/a0
	bclr	#6,0xfffffa11:w
	rte

ikbd_mousex:
	movel	d0,sp@-

	moveb	0xfffffc02:w,d0
	extw	d0
	addw	d0,_SDL_AtariIkbd_mousex

	movel	sp@+,d0

	movel	#ikbd_mousey,0x118:w
	bclr	#6,0xfffffa11:w
	rte

ikbd_mousey:
	movel	d0,sp@-

	moveb	0xfffffc02:w,d0
	extw	d0
	addw	d0,_SDL_AtariIkbd_mousey

	movel	sp@+,d0

	movel	#ikbd,0x118:w
	bclr	#6,0xfffffa11:w
	rte

ikbd_joystick:
	moveb	0xfffffc02:w,_SDL_AtariIkbd_joystick+1

	movel	#ikbd,0x118:w
	bclr	#6,0xfffffa11:w
	rte

	.data
	
	.even
_SDL_AtariIkbd_enabled:
	.word	0
	.even
	.comm	_SDL_AtariIkbd_keyboard,128
	.even
	.comm	_SDL_AtariIkbd_mousex,2*1
	.even
	.comm	_SDL_AtariIkbd_mousey,2*1
	.even
	.comm	_SDL_AtariIkbd_mouseb,2*1
	.even
	.comm	_SDL_AtariIkbd_joystick,2*1
