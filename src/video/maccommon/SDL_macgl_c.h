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
static char rcsid =
 "@(#) $Id$";
#endif

/* AGL implementation of SDL OpenGL support */

#ifdef HAVE_OPENGL
#ifdef MACOSX
#include <OpenGL/gl.h> /* OpenGL.framework */
#include <AGL/agl.h>   /* AGL.framework */
#else
#include <GL/gl.h>
#include <agl.h>
#endif /* MACOSX */
#endif /* HAVE_OPENGL */

/* OpenGL functions */
extern int Mac_GL_Init(_THIS);
extern void Mac_GL_Quit(_THIS);
#ifdef HAVE_OPENGL
extern int Mac_GL_MakeCurrent(_THIS);
extern int Mac_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
extern void Mac_GL_SwapBuffers(_THIS);
extern int Mac_GL_LoadLibrary(_THIS, const char *location);
extern void Mac_GL_UnloadLibrary(_THIS);
extern void* Mac_GL_GetProcAddress(_THIS, const char *proc);
#endif

