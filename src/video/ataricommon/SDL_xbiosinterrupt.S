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
 *	XBIOS mouse & joystick vectors
 *
 *	Patrice Mandin
 */

	.text

	.globl	_SDL_AtariXbios_Install
	.globl	_SDL_AtariXbios_Restore
	.globl	_SDL_AtariXbios_MouseVector
	.globl	_SDL_AtariXbios_JoystickVector

	.globl	_SDL_AtariXbios_mouseb
	.globl	_SDL_AtariXbios_mousex
	.globl	_SDL_AtariXbios_mousey
	.globl	_SDL_AtariXbios_joystick

/*--- Vector installer ---*/

_SDL_AtariXbios_Install:
	movel	sp@(4),a0

	/* Stop interrupts */

	movew	#0x2700,sr

	/* Save old mouse vector, set our routine */

	clrl	oldmousevector
	movel	sp@(8),d0
	beqs	no_new_mouse_vector
	movel	a0@(16),oldmousevector
	movel	d0,a0@(16)
no_new_mouse_vector:

	/* Save old joystick vector, set our routine */

	clrl	oldjoystickvector
	movel	sp@(12),d0
	beqs	no_new_joystick_vector
	movel	a0@(24),oldjoystickvector
	movel	d0,a0@(24)
no_new_joystick_vector:

	/* Restart interrupts */

	movew	#0x2300,sr

	rts

/*--- Vector restorer ---*/

_SDL_AtariXbios_Restore:
	movel	sp@(4),a0

	/* Stop interrupts */

	movew	#0x2700,sr

	/* Restore mouse vector */

	movel	oldmousevector,d0
	beqs	no_restore_mouse
	movel	d0,a0@(16)
no_restore_mouse:

	/* Restore joystick vector */

	movel	oldjoystickvector,d0
	beqs	no_restore_joystick
	movel	d0,a0@(24)
no_restore_joystick:

	/* Restart interrupts */

	movew	#0x2300,sr

	rts

/*--- Our mouse vector ---*/

	.text
	.even
	.ascii "XBRA"
	.ascii "LSDL"
	.comm	oldmousevector,4*1
_SDL_AtariXbios_MouseVector:
	moveml	d0/a1,sp@-

	/* Mouse buttons */
	moveb	(a0),d0
	andw	#3,d0
	movew	d0,_SDL_AtariXbios_mouseb

	/* X movement */
	moveb	a0@(1),d0
	extw	d0
	addw	d0,_SDL_AtariXbios_mousex

	/* Y movement */
	moveb	a0@(2),d0
	extw	d0
	addw	d0,_SDL_AtariXbios_mousey

	/* Jump through old vector */
	movel	oldmousevector,a1
	jsr	(a1)

	moveml	sp@+,d0/a1
	rts

	.data
	.even
	.comm	_SDL_AtariXbios_mousex,2*1
	.even
	.comm	_SDL_AtariXbios_mousey,2*1
	.even
	.comm	_SDL_AtariXbios_mouseb,2*1

/*--- Our joystick vector ---*/

	.text
	.even
	.ascii "XBRA"
	.ascii "LSDL"
	.comm	oldjoystickvector,4*1
_SDL_AtariXbios_JoystickVector:
	moveml	d0/a1,sp@-

	/* New joystick state */
	moveb	a0@(1),d0
	andw	#0x8f,d0
	movew	d0,_SDL_AtariXbios_joystick

	/* Jump through old vector */
	movel	oldjoystickvector,a1
	jsr	(a1)

	moveml	sp@+,d0/a1
	rts

	.data
	.even
	.comm	_SDL_AtariXbios_joystick,2*1
