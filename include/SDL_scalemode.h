/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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

/**
 *  \file SDL_scalemode.h
 *  
 *  Header file declaring the SDL_ScaleMode enumeration
 */

#ifndef _SDL_scalemode_h
#define _SDL_scalemode_h

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \brief The texture scale mode used in SDL_RenderCopy().
 */
typedef enum
{
    SDL_SCALEMODE_NONE = 0x00000000,     /**< No scaling, rectangles must
                                              match dimensions */
    
    SDL_SCALEMODE_FAST = 0x00000001,     /**< Point sampling or 
                                              equivalent algorithm */
    
    SDL_SCALEMODE_SLOW = 0x00000002,     /**< Linear filtering or 
                                              equivalent algorithm */
    
    SDL_SCALEMODE_BEST = 0x00000004      /**< Bicubic filtering or 
                                              equivalent algorithm */
} SDL_ScaleMode;


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_video_h */

/* vi: set ts=4 sw=4 expandtab: */
