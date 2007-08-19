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
    SDLKey sym;                 /**< SDL virtual key code - see ::SDLKey for details */
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
 * \fn Uint8 *SDL_GetKeyState(int *numkeys)
 *
 * \brief Get a snapshot of the current state of the selected keyboard.
 *
 * \param numkeys if non-NULL, receives the length of the returned array.
 *
 * \return An array of key states. Indexes into this array are obtained by using the SDLK_INDEX() macro on the \link ::SDLPhysicalKey SDLK_* \endlink syms.
 *
 * Example:
 * 	Uint8 *keystate = SDL_GetKeyState(NULL);
 *	if ( keystate[SDLK_INDEX(SDLK_RETURN)] ) ... <RETURN> is pressed.
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
 * \fn SDLKey SDL_GetLayoutKey(SDLKey physicalKey)
 * 
 * \brief Get the layout key code corresponding to the given physical key code according to the current keyboard layout.
 *
 * See ::SDLKey for details.
 *
 * If \a physicalKey is not a physical key code, it is returned unchanged.
 *
 * \sa SDL_GetKeyName()
 */
extern DECLSPEC SDLKey SDLCALL SDL_GetLayoutKey(SDLKey physicalKey);

/**
 * \fn const char *SDL_GetKeyName(SDLKey layoutKey)
 * 
 * \brief Get a human-readable name for a key.
 *
 * \param layoutKey An SDL layout key code.
 *
 * If what you have is a physical key code, e.g. from the \link SDL_keysym::sym key.keysym.sym \endlink field of the SDL_Event structure, convert it to a layout key code using SDL_GetLayoutKey() first. Doing this ensures that the returned name matches what users see on their keyboards. Calling this function directly on a physical key code (that is not also a layout key code) is possible, but is not recommended except for debugging purposes. The name returned in that case is the name of the \link ::SDLPhysicalKey SDLK_* \endlink constant and is not suitable for display to users.
 *
 * \return A pointer to a UTF-8 string that stays valid at least until the next call to this function. If you need it around any longer, you must copy it. Always non-NULL.
 *
 * \sa SDLKey
 */
extern DECLSPEC const char *SDLCALL SDL_GetKeyName(SDLKey layoutKey);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_keyboard_h */

/* vi: set ts=4 sw=4 expandtab: */
