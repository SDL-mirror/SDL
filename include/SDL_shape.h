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

struct SDL_Shaped_Window;
typedef struct SDL_Shaped_Window SDL_Shaped_Window;

/**
 *  \brief Create a shaped window with the specified position, dimensions, and flags.
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

extern DECLSPE SDL_bool SDLCALL SDL_WindowIsShaped(const SDL_Window *window);
extern DECLSPEC int SDLCALL SDL_SelectShapeRenderer(const SDL_Window *window);

extern DECLSPEC void SDLCALL SDL_DestroyShapedWindow(const SDL_Shaped_Window *window);
