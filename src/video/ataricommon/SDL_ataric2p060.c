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

#include <string.h>

#include <mint/cookie.h>

#include "SDL_ataric2p_s.h"

/*--- Variables ---*/

/* CPU is 060 ? */
int atari_cpu060_avail;

/*--- Functions ---*/

void atari_test_cpu060_present(void)
{
	unsigned long cookie_cpu;

	atari_cpu060_avail=0;

	/* Cookie _CPU present ? */
	if (Getcookie(C__CPU, &cookie_cpu) == C_FOUND) {
		atari_cpu060_avail = (cookie_cpu == 60);
	}
}

void Atari_C2pConvert8_060(
	Uint8 *src,			/* Source screen (one byte=one pixel) */
	Uint8 *dest,		/* Destination (8 bits planes) */
	Uint32 width,		/* Dimensions of screen to convert */
	Uint32 height,
	Uint32 dblligne,	/* Double the lines when converting ? */
	Uint32 srcpitch,	/* Length of one source line in bytes */
	Uint32 dstpitch		/* Length of one destination line in bytes */
)
{
	int x,y,z;
	Uint8 *src_line, *dst_line;

	for (y=0; y<height; y++) {
		src_line = src;
		dst_line = dest;

		for (x=0; x<(width>>4); x++) {
			Uint32 somme1, somme2;
			Uint32 *convtable;

			/* bytes 0-7 */
			somme1 = somme2 = 0;
			for (z=0; z<8 ;z++) {
				convtable = (Uint32 *) &Atari_table_c2p[(*src_line++)<<3];
				somme1 <<= 1;
				somme2 <<= 1;
				somme1 |= *convtable++;
				somme2 |= *convtable;
			}

			*(dst_line+14) = somme2;	/* 000000FF */
			*(dst_line+6) = somme1;		/* 000000FF */
			somme2 >>= 8;
			somme1 >>= 8;
			*(dst_line+12) = somme2;	/* 0000FF00 */
			*(dst_line+4) = somme1;		/* 0000FF00 */
			somme2 >>= 8;
			somme1 >>= 8;
			*(dst_line+10) = somme2;	/* 00FF0000 */
			*(dst_line+2) = somme1;		/* 00FF0000 */
			somme2 >>= 8;
			somme1 >>= 8;
			*(dst_line+8) = somme2;		/* FF000000 */
			*dst_line++ = somme1;		/* FF000000 */

			/* bytes 8-15 */
			somme1 = somme2 = 0;
			for (z=0; z<8 ;z++) {
				convtable = (Uint32 *) &Atari_table_c2p[(*src_line++)<<3];
				somme1 <<= 1;
				somme2 <<= 1;
				somme1 |= *convtable++;
				somme2 |= *convtable;
			}

			*(dst_line+14) = somme2;	/* 000000FF */
			*(dst_line+6) = somme1;		/* 000000FF */
			somme2 >>= 8;
			somme1 >>= 8;
			*(dst_line+12) = somme2;	/* 0000FF00 */
			*(dst_line+4) = somme1;		/* 0000FF00 */
			somme2 >>= 8;
			somme1 >>= 8;
			*(dst_line+10) = somme2;	/* 00FF0000 */
			*(dst_line+2) = somme1;		/* 00FF0000 */
			somme2 >>= 8;
			somme1 >>= 8;
			*(dst_line+8) = somme2;		/* FF000000 */
			*dst_line = somme1;			/* FF000000 */

			dst_line += 15;
		}

		if (dblligne) {
			memcpy(dest+dstpitch, dest, width);
			dest += dstpitch;
		}

		src += srcpitch;
		dest += dstpitch;
	}
}

void Atari_C2pConvert4_060(
	Uint8 *src,			/* Source screen (one byte=one pixel) */
	Uint8 *dest,		/* Destination (4 bits planes) */
	Uint32 width,		/* Dimensions of screen to convert */
	Uint32 height,
	Uint32 dblligne,	/* Double the lines when converting ? */
	Uint32 srcpitch,	/* Length of one source line in bytes */
	Uint32 dstpitch		/* Length of one destination line in bytes */
)
{
	int x,y,z;
	Uint8 *src_line, *dst_line;

	for (y=0;y<height;y++) {
		src_line = src;
		dst_line = dest;

		for (x=0; x<(width>>4);x++) {
			Uint32 somme;
			Uint32 *convtable;

			/* bytes 0-7 */
			somme=0;
			for (z=0; z<8 ; z++) {
				convtable = (Uint32 *) &Atari_table_c2p[(*src_line++)<<2];
				somme <<= 1;
				somme |= *convtable;
			}

			*(dst_line+6) = somme; somme >>= 8;	/* 000000FF */
			*(dst_line+4) = somme; somme >>= 8;	/* 0000FF00 */
			*(dst_line+2) = somme; somme >>= 8;	/* 00FF0000 */
			*dst_line++ = somme;				/* FF000000 */

			/* bytes 8-15 */
			somme = 0;
			for (z=0; z<8 ;z++) {
				convtable = (Uint32 *) &Atari_table_c2p[(*src_line++)<<2];
				somme <<= 1;
				somme |= *convtable;
			}

			*(dst_line+6) = somme; somme >>= 8;	/* 000000FF */
			*(dst_line+4) = somme; somme >>= 8;	/* 0000FF00 */
			*(dst_line+2) = somme; somme >>= 8;	/* 00FF0000 */
			*dst_line = somme;					/* FF000000 */

			dst_line += 7;
		}

		if (dblligne) {
			memcpy(dest+dstpitch, dest, width>>1);
			dest += dstpitch;
		}

		src += srcpitch;
		dest += dstpitch;
	}
}
