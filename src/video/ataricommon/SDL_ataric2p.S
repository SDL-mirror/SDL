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
 *	Chunky to planar conversion routine
 *	1 byte/pixel -> 4 or 8 bit planes
 *
 *	Patrice Mandin
 *	Xavier Joubert
 */

/* ------------	Function pointers ------------ */

	.globl	_Atari_C2pInit
	.globl	_Atari_C2pConvert

	.data

	.even
	.comm	_Atari_C2pInit,4
	.comm	_Atari_C2pConvert,4

/* ------------	Conversion Table C2P ------------ */

	.globl	_Atari_table_c2p

	.data

	.comm	_Atari_table_c2p,2048

/* ------------	Init table C2P, 8 bits ------------ */

	.globl	_Atari_C2pInit8

	.text
_Atari_C2pInit8:
	movel	d2,sp@-

	lea	_Atari_table_c2p,a0
	movew	#255,d0
c2p_initbcl:
#if defined (__M68020__)
	lea	a0@(0,d0:w:8),a1
#else
	movew	d0,d2
	lslw	#3,d2
	lea	a0@(0,d2:w),a1
#endif
	moveq	#7,d1
c2p_initbyte:
	btst	d1,d0
	sne	d2
	negw	d2
	moveb	d2,a1@(0,d1:w)

	dbra	d1,c2p_initbyte

	dbra	d0,c2p_initbcl

	movel	sp@+,d2
	rts

/* ------------	Conversion C2P, 8 bits ------------ */

	.globl	_Atari_C2pConvert8

	.text
_Atari_C2pConvert8:
	movel	sp@(4),c2p_source
	movel	sp@(8),c2p_dest
	movel	sp@(12),c2p_width
	movel	sp@(16),c2p_height
	movel	sp@(20),c2p_dblligne
	movel	sp@(24),c2p_srcpitch
	movel	sp@(28),c2p_dstpitch

	moveml	d2-d7/a2-a6,sp@-

	movel	c2p_source,a0
	movel	c2p_dest,a1
	lea	_Atari_table_c2p,a2
	movel	#0x000f0001,d3
#if defined(__M68020__)
	moveq	#0,d0
#endif
	
	movel	c2p_height,d7
	subql	#1,d7
c2p8_bcly:
	movel	a0,a4	| Save start address of source
	movel	a1,a5	| Save start address of dest

	| Conversion
                 
	movel	c2p_width,d6
	lsrw	#4,d6
	subql	#1,d6
c2p8_bclx:
	| Octets 0-7
	
	moveq	#0,d1
	moveq	#0,d2
	moveq	#7,d5
c2p8_bcl07:
#if defined(__M68020__)
	moveb	a0@+,d0
	lea	a2@(0,d0:w:8),a3
#else
	moveq	#0,d0
	moveb	a0@+,d0
	lslw	#3,d0
	lea	a2@(0,d0:w),a3
#endif
	lsll	#1,d1
	lsll	#1,d2
	orl	a3@+,d1
	orl	a3@,d2
	dbra	d5,c2p8_bcl07

	movepl	d1,a1@(0)
	movepl	d2,a1@(8)
	addw	d3,a1
	swap	d3
	
	| Octets 8-15

	moveq	#0,d1
	moveq	#0,d2
	moveq	#7,d5
c2p8_bcl815:
#if defined(__M68020__)
	moveb	a0@+,d0
	lea	a2@(0,d0:w:8),a3
#else
	moveq	#0,d0
	moveb	a0@+,d0
	lslw	#3,d0
	lea	a2@(0,d0:w),a3
#endif
	lsll	#1,d1
	lsll	#1,d2
	orl	a3@+,d1
	orl	a3@,d2
	dbra	d5,c2p8_bcl815

	movepl	d1,a1@(0)
	movepl	d2,a1@(8)
	addw	d3,a1
	swap	d3

	dbra	d6,c2p8_bclx

	| Double line ?

	tstl	c2p_dblligne
	beq	c2p8_nodblligne

	movel	a5,a6			| src line
	movel	a5,a1			| dest line
	addl	c2p_dstpitch,a1

	movel	c2p_width,d6
	lsrw	#2,d6
	subql	#1,d6
c2p8_copydbl:
	movel	a6@+,a1@+
	dbra	d6,c2p8_copydbl

	addl	c2p_dstpitch,a5
c2p8_nodblligne:

	| Next line

	movel	a4,a0		
	addl	c2p_srcpitch,a0
	movel	a5,a1
	addl	c2p_dstpitch,a1

	dbra	d7,c2p8_bcly

	moveml	sp@+,d2-d7/a2-a6
	rts

/* ------------	Init table C2P, 4 bits ------------ */

	.globl	_Atari_C2pInit4

	.text
_Atari_C2pInit4:
	/* Nothing to do */
	/* work is done in convert_c2p_pal */
	rts

/* ------------	Conversion C2P, 4 bits ------------ */

	.globl	_Atari_C2pConvert4

	.text
_Atari_C2pConvert4:
	movel	sp@(4),c2p_source
	movel	sp@(8),c2p_dest
	movel	sp@(12),c2p_width
	movel	sp@(16),c2p_height
	movel	sp@(20),c2p_dblligne
	movel	sp@(24),c2p_srcpitch
	movel	sp@(28),c2p_dstpitch

	moveml	d2-d7/a2-a6,sp@-

	movel	c2p_source,a0
	movel	c2p_dest,a1
	lea	_Atari_table_c2p,a2
	movel	#0x00070001,d3
#if defined(__M68020__)
	moveq	#0,d0
#endif
	
	movel	c2p_height,d7
	subql	#1,d7
c2p4_bcly:
	movel	a0,a4	| Save start address of source
	movel	a1,a5	| Save start address of dest

	| Conversion
                 			
	movel	c2p_width,d6
	lsrw	#4,d6
	subql	#1,d6
c2p4_bclx:
	| Octets 0-7
	
	moveq	#0,d1
	moveq	#7,d5
c2p4_bcl07:
#if defined(__M68020__)
	moveb	a0@+,d0
	lea	a2@(0,d0:w:4),a3
#else
	moveq	#0,d0
	moveb	a0@+,d0
	lslw	#2,d0
	lea	a2@(0,d0:w),a3
#endif
	lsll	#1,d1
	orl	a3@,d1
	dbra	d5,c2p4_bcl07

	movepl	d1,a1@(0)
	addw	d3,a1
	swap	d3
	
	| Octets 8-15

	moveq	#0,d1
	moveq	#7,d5
c2p4_bcl815:
#if defined(__M68020__)
	moveb	a0@+,d0
	lea	a2@(0,d0:w:4),a3
#else
	moveq	#0,d0
	moveb	a0@+,d0
	lslw	#2,d0
	lea	a2@(0,d0:w),a3
#endif
	lsll	#1,d1
	orl	a3@,d1
	dbra	d5,c2p4_bcl815

	movepl	d1,a1@(0)
	addw	d3,a1
	swap	d3

	dbra	d6,c2p4_bclx

	| Double line ?

	tstl	c2p_dblligne
	beq	c2p4_nodblligne

	movel	a5,a6			| src line
	movel	a5,a1			| dest line
	addl	c2p_dstpitch,a1

	movel	c2p_width,d6
	lsrw	#3,d6
	subql	#1,d6
c2p4_copydbl:
	movel	a6@+,a1@+
	dbra	d6,c2p4_copydbl

	addl	c2p_dstpitch,a5
c2p4_nodblligne:

	| Next line

	movel	a4,a0		
	addl	c2p_srcpitch,a0
	movel	a5,a1
	addl	c2p_dstpitch,a1

	dbra	d7,c2p4_bcly

	moveml	sp@+,d2-d7/a2-a6
	rts

* ------------	Conversion of a light palette in 4 bits ------------ */

	.globl	_Atari_C2pConvert4_pal

	.text
_Atari_C2pConvert4_pal:
	/* a0 is a 256-word light palette */
	movel	sp@(4),a0

	moveml	d2-d3,sp@-

	lea	_Atari_table_c2p,a1
	movew	#255,d3
c2p_pal_initbcl:
	movew	a0@+,d0
	lsrw	#4,d0
	and	#15,d0

	moveq	#3,d1
c2p_pal_initbyte:
	btst	d1,d0
	sne	d2
	negw	d2
	moveb	d2,a1@(0,d1:w)

	dbra	d1,c2p_pal_initbyte

	addql	#4,a1
	dbra	d3,c2p_pal_initbcl

	moveml	sp@+,d2-d3

	rts

/* ------------	Buffers ------------ */

	.data

	.even
	.comm	c2p_source,4
	.comm	c2p_dest,4
	.comm	c2p_width,4
	.comm	c2p_height,4
	.comm	c2p_dblligne,4
	.comm	c2p_srcpitch,4
	.comm	c2p_dstpitch,4

