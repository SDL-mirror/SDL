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

/* StormMesa implementation of SDL OpenGL support */

#include "SDL_sysvideo.h"
#define _THIS   SDL_VideoDevice *_this

#ifdef HAVE_OPENGL
#include <GL/Amigamesa.h>
extern void *AmiGetGLProc(const char *proc);
#endif /* HAVE_OPENGL */

struct SDL_PrivateGLData {
	int gl_active;
};

/* OpenGL functions */
extern int CGX_GL_Init(_THIS);
extern void CGX_GL_Quit(_THIS);
extern int CGX_GL_Update(_THIS);
#ifdef HAVE_OPENGL
extern int CGX_GL_MakeCurrent(_THIS);
extern int CGX_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
extern void CGX_GL_SwapBuffers(_THIS);
extern void *CGX_GL_GetProcAddress(_THIS, const char *proc);
extern int CGX_GL_LoadLibrary(_THIS, const char *path);
#endif

#undef _THIS
