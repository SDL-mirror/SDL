/*
    SDL - Simple DihintsMedia Layer
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
 *  \file SDL_hints.h
 *  
 *  Official documentation for SDL configuration variables
 *
 *  This file contains functions to set and get configuration hints,
 *  as well as listing each of them alphabetically.
 *
 *  The convention for naming hints is SDL_HINT_X, where "SDL_X" is
 *  the environment variable that can be used to override the default.
 *
 *  In general these hints are just that - they may or may not be
 *  supported or applicable on any given platform, but they provide
 *  a way for an application or user to give the library a hint as
 *  to how they would like the library to work.
 */

#ifndef _SDL_hints_h
#define _SDL_hints_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif



/**
 *  \brief  An enumeration of hint priorities
 */
typedef enum
{
    SDL_HINT_DEFAULT,
    SDL_HINT_NORMAL,
    SDL_HINT_OVERRIDE
} SDL_HintPriority;


/**
 *  \brief Set a hint
 *
 *  The priority controls the behavior when setting a hint that already
 *  has a value.  Hints will replace existing hints of their priority and
 *  lower.  Environment variables are considered to have override priority.
 * 
 *  \return SDL_TRUE if the hint was set, SDL_FALSE otherwise
 */
extern DECLSPEC SDL_bool SDLCALL SDL_SetHint(const char *name,
                                             const char *value,
                                             SDL_HintPriority priority);

/**
 *  \brief Get a hint
 *  
 *  \return The string value of a hint variable.
 */
extern DECLSPEC const char * SDLCALL SDL_GetHint(const char *name);

/**
 *  \brief  Clear all hints
 *
 *  This function is called during SDL_Quit() to free stored hints.
 */
extern DECLSPEC void SDLCALL SDL_ClearHints();


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_hints_h */

/* vi: set ts=4 sw=4 expandtab: */
