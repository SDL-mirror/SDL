/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2010 Eli Gottlieb

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

    Eli Gottlieb
    eligottlieb@gmail.com
*/

/**
 *  \file SDL_ellipse.h
 *  
 *  Header file for SDL_ellipse definition and management functions.
 */

#ifndef _SDL_ellipse_h
#define _SDL_ellipse_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_pixels.h"
#include "SDL_rwops.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \brief  The structure that defines an ellipse.
 *
 *  \sa SDL_EllipseEmpty
 *  \sa SDL_EllipseEquals
 *  \sa SDL_EllipsesIntersect
 *  \sa SDL_IntersectEllipseAndLine
 */
typedef struct SDL_Ellipse {
	int x,y;
	int a,b;
	int r;
} SDL_Ellipse;

/**
 *  \brief Returns true if the ellipse has no area.
 */
#define SDL_EllipseEmpty(X)    ((X)->r <= 0)

/**
 *  \brief Returns true if the two ellipses are equal.
 */
#define SDL_EllipseEquals(A, B)   (((A)->x == (B)->x) && ((A)->y == (B)->y) && \
                                ((A)->a == (B)->a) && ((A)->b == (B)->b) && ((A)->r == (B)->r))

/**
 *  \brief Determine whether two ellipses intersect.
 *  
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_EllipsesIntersect(const SDL_Ellipse * A,const SDL_Ellipse * B);

/**
 *  \brief Calculate the intersection of an ellipse and line segment.
 *  
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectEllipseAndLine(const SDL_Ellipse *ellipse,int *X1,int *Y1,int *X2,int *Y2);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_ellipse_h */
