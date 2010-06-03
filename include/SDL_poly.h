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
 *  \file SDL_poly.h
 *  
 *  Header file for SDL_poly definition and management functions.
 */

#ifndef _SDL_poly_h
#define _SDL_poly_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_pixels.h"
#include "SDL_rwops.h"
#include "SDL_rect.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \brief  The structure that defines an polygon.
 *
 *  \sa SDL_PolyEmpty
 *  \sa SDL_PolyEquals
 *  \sa SDL_PolysIntersect
 *  \sa SDL_IntersectPoly
 *  \sa SDL_WrapPoints
 *  \sa SDL_IntersectPolyAndLine
 */
typedef struct SDL_Poly {
	SDL_Point *vertices;
	int count;
} SDL_Poly;

/**
 *  \brief Returns true if the polygon has no area.
 */
#define SDL_PolyEmpty(X)    (((X)->vertices == NULL) || ((X)->count <= 2))

/**
 *  \brief Determine whether two polygons are equal.
 *  
 *  \return SDL_TRUE if the polygons are equal, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_PolyEquals(const SDL_Poly *A,const SDL_Poly *B);

/**
 *  \brief Determine whether two rectangles intersect.
 *  
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_PolysIntersect(const SDL_Poly * A,const SDL_Poly * B);

/**
 *  \brief Calculate the intersection of two rectangles.
 *  
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectPoly(const SDL_Poly * A,const SDL_Poly * B,SDL_Poly * result);

/**
 *  \brief Calculate a minimal polygon wrapping a set of points
 *
 *  \return 0 on success, -1 if the parameters were invalid, and -2 if an insufficient number of points were supplied
 *          in the output polygon.
 */
extern DECLSPEC int SDLCALL SDL_WrapPoints(const SDL_Point * points,int count,SDL_Poly *result);

/**
 *  \brief Calculate the intersection of a polygon and line segment.
 *  
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectPolyAndLine(const SDL_Poly *poly,int *X1,int *Y1,int *X2,int *Y2);
                                                   
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_poly_h */
