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
 *  for 68060 CPU, without movep instruction
 *	1 byte/pixel -> 4 or 8 bit planes
 *
 *	Patrice Mandin
 */

#ifndef _SDL_ATARI_C2P060_H_
#define _SDL_ATARI_C2P060_H_

/*--- Variables ---*/

extern int atari_cpu060_avail;

/*--- Functions ---*/

extern void atari_test_cpu060_present(void);

extern void Atari_C2pConvert8_060(
	Uint8 *src,			/* Source screen (one byte=one pixel) */
	Uint8 *dest,		/* Destination (8 bits planes) */
	Uint32 width,		/* Dimensions of screen to convert */
	Uint32 height,
	Uint32 dblligne,	/* Double the lines when converting ? */
	Uint32 srcpitch,	/* Length of one source line in bytes */
	Uint32 dstpitch		/* Length of one destination line in bytes */
);

extern void Atari_C2pConvert4_060(
	Uint8 *src,			/* Source screen (one byte=one pixel) */
	Uint8 *dest,		/* Destination (4 bits planes) */
	Uint32 width,		/* Dimensions of screen to convert */
	Uint32 height,
	Uint32 dblligne,	/* Double the lines when converting ? */
	Uint32 srcpitch,	/* Length of one source line in bytes */
	Uint32 dstpitch		/* Length of one destination line in bytes */
);

#endif /* _SDL_ATARI_C2P060_H_ */
