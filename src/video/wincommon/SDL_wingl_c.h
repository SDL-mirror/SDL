/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* WGL implementation of SDL OpenGL support */

#include "SDL_sysvideo.h"


struct SDL_PrivateGLData {
    int gl_active; /* to stop switching drivers while we have a valid context */

#ifdef HAVE_OPENGL
    PIXELFORMATDESCRIPTOR GL_pfd;
    HDC GL_hdc;
    HGLRC GL_hrc;

    void * (WINAPI *wglGetProcAddress)(const char *proc);

    HGLRC (WINAPI *wglCreateContext)(HDC hdc);

    BOOL (WINAPI *wglDeleteContext)(HGLRC hglrc);

    BOOL (WINAPI *wglMakeCurrent)(HDC hdc, HGLRC hglrc);

#endif /* HAVE_OPENGL */
};

/* Old variable names */
#define gl_active	(this->gl_data->gl_active)
#define GL_pfd		(this->gl_data->GL_pfd)
#define GL_hdc		(this->gl_data->GL_hdc)
#define GL_hrc		(this->gl_data->GL_hrc)

/* OpenGL functions */
extern int WIN_GL_SetupWindow(_THIS);
extern void WIN_GL_ShutDown(_THIS);
#ifdef HAVE_OPENGL
extern int WIN_GL_MakeCurrent(_THIS);
extern int WIN_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
extern void WIN_GL_SwapBuffers(_THIS);
extern void WIN_GL_UnloadLibrary(_THIS);
extern int WIN_GL_LoadLibrary(_THIS, const char* path);
extern void *WIN_GL_GetProcAddress(_THIS, const char* proc);
#endif

