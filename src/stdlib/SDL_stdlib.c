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


/* This file contains portable stdlib functions for SDL */

#include "SDL_stdlib.h"

#ifndef HAVE_LIBC
/* These are some C runtime intrinsics that need to be defined */

#if defined(_MSC_VER)

/* Float to long (FIXME!) */
long _ftol2_sse()
{
	return 0;
}

/* 64-bit math operators (FIXME!) */
void _allmul()
{
}
void _alldiv()
{
}
void _aulldiv()
{
}
void _allrem()
{
}
void _aullrem()
{
}
void _alldvrm()
{
}
void _aulldvrm()
{
}
void _allshl()
{
}
void _aullshr()
{
}

#endif /* MSC_VER */

#endif /* !HAVE_LIBC */
