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

#ifndef _SDL_shape_h
#define _SDL_shape_h

#include "SDL_stdinc.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_surface.h"
#include "SDL_video.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/** \file SDL_shape.h
 *
 * Header file for the shaped window API.
 */

/**
 *  \brief Create a window that can be shaped with the specified position, dimensions, and flags.
 *  
 *  \param title The title of the window, in UTF-8 encoding.
 *  \param x     The x position of the window, ::SDL_WINDOWPOS_CENTERED, or 
 *               ::SDL_WINDOWPOS_UNDEFINED.
 *  \param y     The y position of the window, ::SDL_WINDOWPOS_CENTERED, or 
 *               ::SDL_WINDOWPOS_UNDEFINED.
 *  \param w     The width of the window.
 *  \param h     The height of the window.
 *  \param flags The flags for the window, a mask of SDL_WINDOW_BORDERLESS with any of the following: 
 *               ::SDL_WINDOW_OPENGL,     ::SDL_WINDOW_INPUT_GRABBED,
 *               ::SDL_WINDOW_SHOWN,      ::SDL_WINDOW_RESIZABLE,
 *               ::SDL_WINDOW_MAXIMIZED,  ::SDL_WINDOW_MINIMIZED,
 *		 ::SDL_WINDOW_BORDERLESS is always set, and ::SDL_WINDOW_FULLSCREEN is always unset.
 *  
 *  \return The id of the window created, or zero if window creation failed.
 *  
 *  \sa SDL_DestroyWindow()
 */
extern DECLSPEC SDL_Window * SDLCALL SDL_CreateShapedWindow(const char *title,unsigned int x,unsigned int y,unsigned int w,unsigned int h,Uint32 flags);

/**
 * \brief Return whether the given window is a shaped window. 
 *
 * \param window The window to query for being shaped.
 *
 * \return SDL_TRUE if the window is a window that can be shaped, SDL_FALSE if the window is unshaped or NULL.
 * \sa SDL_CreateShapedWindow
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IsShapedWindow(const SDL_Window *window);

/** \brief An enum denoting the specific type of contents present in an SDL_WindowShapeParams union. */
typedef enum {
	/** \brief The default mode, a binarized alpha cutoff of 1. */
	ShapeModeDefault,
	/** \brief A binarized alpha cutoff with a given integer value. */
	ShapeModeBinarizeAlpha
} WindowShapeMode;
/** \brief A union containing parameters for shaped windows. */
typedef union {
	/** \brief a cutoff alpha value for binarization of the window shape's alpha channel. */
	Uint8 binarizationCutoff;
} SDL_WindowShapeParams;

/** \brief A struct that tags the SDL_WindowShapeParams union with an enum describing the type of its contents. */
typedef struct SDL_WindowShapeMode {
	/** \brief The mode of these window-shape parameters. */
	WindowShapeMode mode;
	/** \brief Window-shape parameters. */
	SDL_WindowShapeParams parameters;
} SDL_WindowShapeMode;

/**
 * \brief Set the shape and parameters of a shaped window.
 *
 * \param window The shaped window whose parameters should be set.
 * \param shape A surface encoding the desired shape for the window.
 * \param shapeMode The parameters to set for the shaped window.
 *
 * \return 0 on success, -1 on invalid an invalid shape argument, or -2 if the SDL_Window* given does not reference
 *         a valid shaped window.
 *
 * \sa SDL_WindowShapeMode
 * \sa SDL_GetShapedWindowMode.
 */
extern DECLSPEC int SDLCALL SDL_SetWindowShape(SDL_Window *window,SDL_Surface *shape,SDL_WindowShapeMode *shapeMode);

/**
 * \brief Get the shape parameters of a shaped window.
 *
 * \param window The shaped window whose parameters should be retrieved.
 * \param shapeMode An empty shape-parameters structure to fill.
 *
 * \return 0 on success, -1 on a null shapeMode, or -2 if the SDL_Window given is not a shaped window, or -3 if the
 *         SDL_Window given is a window that can be shaped but isn't.
 *
 * \sa SDL_WindowShapeMode
 * \sa SDL_SetWindowShape
 */
extern DECLSPEC int SDLCALL SDL_GetShapedWindowMode(SDL_Window *window,SDL_WindowShapeMode *shapeMode);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_shape_h */
