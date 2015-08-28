/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

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
#include "SDL_config.h"

#include "../SDL_sysvideo.h"

#include "SDL_os4video.h"
#include "SDL_os4utils.h"
#include "SDL_os4blit.h"

//#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

/*
 * Copy numItem pixels from an 8-bit paletted surface to a 16-bit surface
 */
void cpy_8_16(void *dst, const void *src, uint32 numItems, const uint32 palette[])
{
	if (numItems > 1)
	{
		__asm volatile(
		   "addi	%2, %2, -1			\n\
			mtctr	%2					\n\
										\n\
			lbz		0, 0(%1)			\n\
			slwi	0, 0, 2				\n\
			lwzx	0, %3, 0			\n\
			sth		0, 0(%0)			\n\
		1:	lbzu	0, 1(%1)			\n\
			slwi	0, 0, 2				\n\
			lwzx	0, %3, 0			\n\
			sthu	0, 2(%0)			\n\
			bdnz	1b"
		:
		: "r" (dst), "r" (src), "r" (numItems), "r" (palette)
		: "r0", "memory");
	}
	else
	{
		__asm volatile(
		   "lbz		0, 0(%1)			\n\
		    slwi	0, 0, 2				\n\
			lwzx	0, %2, 0			\n\
			sth		0, 0(%0)"
		:
		: "r" (dst), "r" (src), "r" (palette)
		: "r0", "memory");
	}
}

/*
 * Copy numItem pixels from an 8-bit paletted surface to a packed 24-bit surface
 */
void cpy_8_24(void *dst, const void *src, uint32 numItems, const uint32 palette[])
{
    const uint8 *srcPixel   = (uint8 *) src;
	uint8       *dstByte    = (uint8 *) dst;
	int          pixelCount = numItems;

	for (; pixelCount > 0; pixelCount--)
	{
		*dstByte++ = palette[*srcPixel] >> 16;
		*dstByte++ = palette[*srcPixel] >> 8;
		*dstByte++ = palette[*srcPixel];

		srcPixel++;
	}
}

/*
 * Copy numItem pixels from an 8-bit paletted surface to a 32-bit surface
 */
void cpy_8_32(void *dst, const void *src, uint32 numItems, const uint32 palette[])
{
	if (numItems > 1)
	{
		__asm volatile(
			"addi	%2, %2, -1			\n\
			mtctr	%2					\n\
										\n\
			lbz	0, 0(%1)				\n\
			slwi	0, 0, 2				\n\
			lwzx	0, %3, 0			\n\
			stw	0, 0(%0)				\n\
		1:	lbzu	0, 1(%1)			\n\
			slwi	0, 0, 2				\n\
			lwzx	0, %3, 0			\n\
			stwu	0, 4(%0)			\n\
			bdnz	1b"
		:
		: "r" (dst), "r" (src), "r" (numItems), "r" (palette)
		: "r0", "memory");
	}
	else
	{
		__asm volatile(
			"lbz	0, 0(%1)			\n\
			slwi	0, 0, 2				\n\
			lwzx	0, %2, 0			\n\
			stw	0, 0(%0)"
		:
		: "r" (dst), "r" (src), "r" (palette)
		: "r0", "memory");
	}
}
