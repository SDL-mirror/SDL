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

#ifdef HAVE_OPENGL
#include <GL/glx.h>
#include <dlfcn.h>
#if defined(__OpenBSD__) && !defined(__ELF__)
#define dlsym(x,y) dlsym(x, "_" y)
#endif
#endif
#include "SDL_sysvideo.h"

struct SDL_PrivateGLData {
    int gl_active; /* to stop switching drivers while we have a valid context */

#ifdef HAVE_OPENGL
    GLXContext glx_context;	/* Current GL context */
    XVisualInfo* glx_visualinfo; /* XVisualInfo* returned by glXChooseVisual */

    void * (*glXGetProcAddress)(const GLubyte *procName);

    XVisualInfo* (*glXChooseVisual)
		( Display*		dpy,
		  int			screen,
		  int*		attribList );

    GLXContext (*glXCreateContext)
		( Display*		dpy,
		  XVisualInfo*	vis,
		  GLXContext		shareList,
		  Bool		direct );

    void (*glXDestroyContext)
		( Display* 		dpy,
		  GLXContext		ctx );

    Bool (*glXMakeCurrent)
		( Display*		dpy,
		  GLXDrawable		drawable,
		  GLXContext		ctx );

    void (*glXSwapBuffers)
		( Display*		dpy,
		  GLXDrawable		drawable );

    int (*glXGetConfig)
	 ( Display* dpy,
	   XVisualInfo* visual_info,
	   int attrib,
	   int* value );

    void (*glXReleaseBuffersMESA)
	 ( Display* dpy,
	   GLXDrawable drawable );

    const char *(*glXQueryExtensionsString)
	    ( Display* dpy,
	      int screen);

    
#endif /* HAVE_OPENGL */
};

/* Old variable names */
#define gl_active		(this->gl_data->gl_active)
#define glx_context		(this->gl_data->glx_context)
#define glx_visualinfo		(this->gl_data->glx_visualinfo)

/* OpenGL functions */
extern XVisualInfo *X11_GL_GetVisual(_THIS);
extern int X11_GL_CreateWindow(_THIS, int w, int h);
extern int X11_GL_CreateContext(_THIS);
extern void X11_GL_Shutdown(_THIS);
#ifdef HAVE_OPENGL
extern int X11_GL_MakeCurrent(_THIS);
extern int X11_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
extern void X11_GL_SwapBuffers(_THIS);
extern int X11_GL_LoadLibrary(_THIS, const char* path);
extern void *X11_GL_GetProcAddress(_THIS, const char* proc);
#endif
extern void X11_GL_UnloadLibrary(_THIS);

