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

#ifdef HAVE_MATH_H
#define _USE_MATH_DEFINES
#include <math.h>
#else

/* Math routines from uClibc: http://www.uclibc.org */

#define M_PI    3.14159265358979323846264338327950288   /* pi */

extern double __ieee754_log(double x);
extern double __ieee754_pow(double x, double y);
extern double __ieee754_sqrt(double x);

#define log(x)      __ieee754_log(x)
#define pow(x, y)   __ieee754_pow(x, y)
#define sqrt(x)     __ieee754_sqrt(x)

extern double copysign(double x, double y);
extern double cos(double x);
extern double fabs(double x);
extern double floor(double x);
extern double scalbn(double x, int n);
extern double sin(double x);

#define sinf(x) (float)sin((double)x)
#define cosf(x) (float)cos((double)x)

#endif /* HAVE_MATH_H */

/* vi: set ts=4 sw=4 expandtab: */
