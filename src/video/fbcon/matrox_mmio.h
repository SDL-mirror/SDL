/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

/* MGA register definitions */

#include "matrox_regs.h"

/* MGA control macros */

#define mga_in8(reg)		*(volatile Uint8  *)(mapped_io + (reg))
#define mga_in32(reg)		*(volatile Uint32 *)(mapped_io + (reg))

#define mga_out8(reg,v)		*(volatile Uint8  *)(mapped_io + (reg)) = v;
#define mga_out32(reg,v)	*(volatile Uint32 *)(mapped_io + (reg)) = v;


/* Wait for fifo space */
#define mga_wait(space)							\
{									\
	while ( mga_in8(MGAREG_FIFOSTATUS) < space )			\
		;							\
}


/* Wait for idle accelerator */
#define mga_waitidle()							\
{									\
	while ( mga_in32(MGAREG_STATUS) & 0x10000 )			\
		;							\
}

