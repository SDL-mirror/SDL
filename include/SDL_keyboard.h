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

/**
 * \file SDL_keyboard.h
 *
 * Include file for SDL keyboard event handling
 */

#ifndef _SDL_keyboard_h
#define _SDL_keyboard_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_keysym.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 * \struct SDL_keysym
 *
 * \brief The SDL keysym structure, used in key events.
 */
typedef struct SDL_keysym
{
    Uint8 scancode;             /**< keyboard specific scancode */
    Uint8 padding[3];           /**< alignment padding */
    Uint16 sym;                 /**< SDL virtual keysym */
    Uint16 mod;                 /**< current key modifiers */
    Uint32 unicode;             /**< OBSOLETE, use SDL_TextInputEvent instead */
} SDL_keysym;

/* Function prototypes */

/**
 * \fn int SDL_GetNumKeyboards(void)
 *
 * \brief Get the number of keyboard input devices available.
 *
 * \sa SDL_SelectKeyboard()
 */
extern DECLSPEC int SDLCALL SDL_GetNumKeyboards(void);

/**
 * \fn int SDL_SelectKeyboard(int index)
 *
 * \brief Set the index of the currently selected keyboard.
 *
 * \return The index of the previously selected keyboard.
 *
 * \note You can query the currently selected keyboard by passing an index of -1.
 *
 * \sa SDL_GetNumKeyboards()
 */
extern DECLSPEC int SDLCALL SDL_SelectKeyboard(int index);

/**
 * \fn int SDL_EnableUNICODE(int enable)
 *
 * \brief Enable/Disable UNICODE translation of keyboard input.
 *
 * \param enable 1 to enable translation, 0 to disable translation, -1 to query translation
 *
 * \return The previous state of keyboard translation
 *
 * \note This translation has some overhead, so translation defaults off.
 */
extern DECLSPEC int SDLCALL SDL_EnableUNICODE(int enable);

/**
 * \fn int SDL_EnableKeyRepeat(int delay, int interval)
 * 
 * \brief Enable keyboard repeat for the selected keyboard.
 *
 * \param delay The initial delay in milliseconds between the time when a
 *              key is pressed and keyboard repeat begins.  Setting a delay
 *              of 0 will disable keyboard repeat.
 * \param interval The time in milliseconds between keyboard repeat events.
 *
 * \return 0 on success, or -1 if there was an error.
 *
 * \note Keyboard repeat defaults to off.
 */
#define SDL_DEFAULT_REPEAT_DELAY	500
#define SDL_DEFAULT_REPEAT_INTERVAL	30
 /**/
    extern DECLSPEC int SDLCALL SDL_EnableKeyRepeat(int delay, int interval);

/**
 * \fn void SDL_GetKeyRepeat(int *delay, int *interval)
 *
 * \brief Get the current keyboard repeat setting for the selected keyboard.
 */
extern DECLSPEC void SDLCALL SDL_GetKeyRepeat(int *delay, int *interval);

/**
 * \fn Uint8 *SDL_GetKeyState(int *numkeys)
 *
 * \brief Get a snapshot of the current state of the selected keyboard.
 *
 * \return An array of keystates, indexed by the SDLK_* syms.
 *
 * Example:
 * 	Uint8 *keystate = SDL_GetKeyState(NULL);
 *	if ( keystate[SDLK_RETURN] ) ... <RETURN> is pressed.
 */
extern DECLSPEC Uint8 *SDLCALL SDL_GetKeyState(int *numkeys);

/**
 * \fn SDLMod SDL_GetModState(void)
 *
 * \brief Get the current key modifier state for the selected keyboard.
 */
extern DECLSPEC SDLMod SDLCALL SDL_GetModState(void);

/**
 * \fn void SDL_SetModState(SDLMod modstate)
 *
 * \brief Set the current key modifier state for the selected keyboard.
 *
 * \note This does not change the keyboard state, only the key modifier flags.
 */
extern DECLSPEC void SDLCALL SDL_SetModState(SDLMod modstate);

/**
 * \fn const char *SDL_GetKeyName(SDLKey key)
 * 
 * \brief Get the name of an SDL virtual keysym
 */
extern DECLSPEC const char *SDLCALL SDL_GetKeyName(SDLKey key);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_keyboard_h */

/* vi: set ts=4 sw=4 expandtab: */
