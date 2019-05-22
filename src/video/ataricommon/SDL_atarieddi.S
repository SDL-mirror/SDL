/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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

/*
 *	Read EdDI version
 *
 *	Patrice Mandin
 */

	.text

	.globl	_Atari_get_EdDI_version

/*--- Vector installer ---*/

_Atari_get_EdDI_version:
#if defined(__mcoldfire__)
	movel	sp@(4),a0
	movel	a0,eddi_cookie
	lea	sp@(-8),sp
	moveml	d2/a2,sp@
#else
	movel	sp@(4),eddi_cookie
	moveml	d2/a2,sp@-
#endif

	/* Supexec() to avoid crash on FreeMiNT with MP */
	pea	get_EdDI_version(pc)
	movew	#38,sp@-
	trap	#14
	addql	#6,sp

#if defined(__mcoldfire__)
	moveml	sp@,d2/a2
	lea	sp@(8),sp
#else
	moveml	sp@+,d2/a2
#endif
	rts

get_EdDI_version:
	movel	eddi_cookie,a0

	/* Call EdDI function #0 */
	clrw	d0
	jmp	(a0)

	.bss

	.even
	.comm	eddi_cookie, 4
