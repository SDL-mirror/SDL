/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
 *	IKBD 6301 mouse vector
 *
 *	Patrice Mandin
 */

	.text

	.globl	_SDL_AtariXbiosMouseInstall
	.globl	_SDL_AtariXbiosMouseVector

	.globl	_SDL_AtariXbios_mouseb
	.globl	_SDL_AtariXbios_mousex
	.globl	_SDL_AtariXbios_mousey

/*--- Vector installer ---*/

_SDL_AtariXbiosMouseInstall:
	movel	sp@(4),a0
	movel	sp@(8),a1

	/* Stop interrupts */

	movew	#0x2700,sr

	/* Save old vector */

	movel	a0@(16),oldvector

	/* Set our routine */

	movel	a1,a0@(16)

	/* Restart interrupts */

	movew	#0x2300,sr

	rts

/*--- Our mouse vector ---*/

	.text
	.even
	.ascii "XBRA"
	.ascii "_SDL"
_SDL_AtariXbiosMouseVector:
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
	movel	oldvector,a1
	jsr	(a1)

	moveml	sp@+,d0/a1
	rts

	.data
	
	.even
	.comm	oldvector,4*1

	.even
	.comm	_SDL_AtariXbios_mousex,2*1
	.even
	.comm	_SDL_AtariXbios_mousey,2*1
	.even
	.comm	_SDL_AtariXbios_mouseb,2*1

