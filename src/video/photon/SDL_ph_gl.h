/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
    static char rcsid = "@(#) $Id$";
#endif

#ifndef __SDL_PH_GL_H__
#define __SDL_PH_GL_H__

#include "SDL_ph_video.h"

#define DEFAULT_OPENGL "/usr/lib/libGL.so"

#ifdef HAVE_OPENGL
    void  ph_GL_SwapBuffers(_THIS);
    int   ph_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
    int   ph_GL_LoadLibrary(_THIS, const char* path);
    void* ph_GL_GetProcAddress(_THIS, const char* proc);
    int   ph_GL_MakeCurrent(_THIS);

    int   ph_SetupOpenGLContext(_THIS, int width, int height, int bpp, Uint32 flags);
#endif /* HAVE_OPENGL */

#endif /* __SDL_PH_GL_H__ */
