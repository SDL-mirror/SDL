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

#include <stdlib.h>	/* For getenv() prototype */
#include <string.h>

#include "SDL_events_c.h"
#include "SDL_error.h"
#include "SDL_x11video.h"
#include "SDL_x11dga_c.h"
#include "SDL_x11gl_c.h"

#define DEFAULT_OPENGL	"libGL.so.1"

#ifndef GLX_ARB_multisample
#define GLX_ARB_multisample
#define GLX_SAMPLE_BUFFERS_ARB             100000
#define GLX_SAMPLES_ARB                    100001
#endif

/* return the preferred visual to use for openGL graphics */
XVisualInfo *X11_GL_GetVisual(_THIS)
{
#ifdef HAVE_OPENGL
	/* 64 seems nice. */
	int attribs[64];
	int i;

	/* load the gl driver from a default path */
	if ( ! this->gl_config.driver_loaded ) {
	        /* no driver has been loaded, use default (ourselves) */
	        if ( X11_GL_LoadLibrary(this, NULL) < 0 ) {
		        return NULL;
		}
	}

	/* See if we already have a window which we must use */
	if ( SDL_windowid ) {
		XWindowAttributes a;
		XVisualInfo vi_in;
		int out_count;

		XGetWindowAttributes(SDL_Display, SDL_Window, &a);
		vi_in.screen = SDL_Screen;
		vi_in.visualid = XVisualIDFromVisual(a.visual);
		glx_visualinfo = XGetVisualInfo(SDL_Display,
	                     VisualScreenMask|VisualIDMask, &vi_in, &out_count);
		return glx_visualinfo;
	}

        /* Setup our GLX attributes according to the gl_config. */
	i = 0;
	attribs[i++] = GLX_RGBA;
	attribs[i++] = GLX_RED_SIZE;
	attribs[i++] = this->gl_config.red_size;
	attribs[i++] = GLX_GREEN_SIZE;
	attribs[i++] = this->gl_config.green_size;
	attribs[i++] = GLX_BLUE_SIZE;
	attribs[i++] = this->gl_config.blue_size;

	if( this->gl_config.alpha_size ) {
		attribs[i++] = GLX_ALPHA_SIZE;
		attribs[i++] = this->gl_config.alpha_size;
	}

	if( this->gl_config.buffer_size ) {
		attribs[i++] = GLX_BUFFER_SIZE;
		attribs[i++] = this->gl_config.buffer_size;
	}

	if( this->gl_config.double_buffer ) {
		attribs[i++] = GLX_DOUBLEBUFFER;
	}

	attribs[i++] = GLX_DEPTH_SIZE;
	attribs[i++] = this->gl_config.depth_size;

	if( this->gl_config.stencil_size ) {
		attribs[i++] = GLX_STENCIL_SIZE;
		attribs[i++] = this->gl_config.stencil_size;
	}

	if( this->gl_config.accum_red_size ) {
		attribs[i++] = GLX_ACCUM_RED_SIZE;
		attribs[i++] = this->gl_config.accum_red_size;
	}

	if( this->gl_config.accum_green_size ) {
		attribs[i++] = GLX_ACCUM_GREEN_SIZE;
		attribs[i++] = this->gl_config.accum_green_size;
	}

	if( this->gl_config.accum_blue_size ) {
		attribs[i++] = GLX_ACCUM_BLUE_SIZE;
		attribs[i++] = this->gl_config.accum_blue_size;
	}

	if( this->gl_config.accum_alpha_size ) {
		attribs[i++] = GLX_ACCUM_ALPHA_SIZE;
		attribs[i++] = this->gl_config.accum_alpha_size;
	}

	if( this->gl_config.stereo ) {
		attribs[i++] = GLX_STEREO;
		attribs[i++] = this->gl_config.stereo;
	}
	
	if( this->gl_config.multisamplebuffers ) {
		attribs[i++] = GLX_SAMPLE_BUFFERS_ARB;
		attribs[i++] = this->gl_config.multisamplebuffers;
	}
	
	if( this->gl_config.multisamplesamples ) {
		attribs[i++] = GLX_SAMPLES_ARB;
		attribs[i++] = this->gl_config.multisamplesamples;
	}

#ifdef GLX_DIRECT_COLOR /* Try for a DirectColor visual for gamma support */
	attribs[i++] = GLX_X_VISUAL_TYPE;
	attribs[i++] = GLX_DIRECT_COLOR;
#endif
	attribs[i++] = None;

 	glx_visualinfo = this->gl_data->glXChooseVisual(GFX_Display, 
						  SDL_Screen, attribs);
#ifdef GLX_DIRECT_COLOR
	if( !glx_visualinfo ) { /* No DirectColor visual?  Try again.. */
		attribs[i-3] = None;
 		glx_visualinfo = this->gl_data->glXChooseVisual(GFX_Display, 
						  SDL_Screen, attribs);
	}
#endif
	if( !glx_visualinfo ) {
		SDL_SetError( "Couldn't find matching GLX visual");
		return NULL;
	}
	return glx_visualinfo;
#else
	SDL_SetError("X11 driver not configured with OpenGL");
	return NULL;
#endif
}

int X11_GL_CreateWindow(_THIS, int w, int h)
{
	int retval;
#ifdef HAVE_OPENGL
	XSetWindowAttributes attributes;
	unsigned long mask;
	unsigned long black;

	black = (glx_visualinfo->visual == DefaultVisual(SDL_Display,
						 	SDL_Screen))
	       	? BlackPixel(SDL_Display, SDL_Screen) : 0;
	attributes.background_pixel = black;
	attributes.border_pixel = black;
	attributes.colormap = SDL_XColorMap;
	mask = CWBackPixel | CWBorderPixel | CWColormap;

	SDL_Window = XCreateWindow(SDL_Display, WMwindow,
			0, 0, w, h, 0, glx_visualinfo->depth,
			InputOutput, glx_visualinfo->visual,
			mask, &attributes);
	if ( !SDL_Window ) {
		SDL_SetError("Could not create window");
		return -1;
	}
	retval = 0;
#else
	SDL_SetError("X11 driver not configured with OpenGL");
	retval = -1;
#endif
	return(retval);
}

int X11_GL_CreateContext(_THIS)
{
	int retval;
#ifdef HAVE_OPENGL
	/* We do this to create a clean separation between X and GLX errors. */
	XSync( SDL_Display, False );
	glx_context = this->gl_data->glXCreateContext(GFX_Display, 
				     glx_visualinfo, NULL, True);
	XSync( GFX_Display, False );

	if (glx_context == NULL) {
		SDL_SetError("Could not create GL context");
		return -1;
	}

	gl_active = 1;
#else
	SDL_SetError("X11 driver not configured with OpenGL");
#endif
	if ( gl_active ) {
		retval = 0;
	} else {
		retval = -1;
	}
	return(retval);
}

void X11_GL_Shutdown(_THIS)
{
#ifdef HAVE_OPENGL
	/* Clean up OpenGL */
	if( glx_context ) {
		this->gl_data->glXMakeCurrent(GFX_Display, None, NULL);

		if (glx_context != NULL)
			this->gl_data->glXDestroyContext(GFX_Display, glx_context);

		if( this->gl_data->glXReleaseBuffersMESA ) {
		    this->gl_data->glXReleaseBuffersMESA(GFX_Display,SDL_Window);
		}
		glx_context = NULL;
	}
	gl_active = 0;
#endif /* HAVE_OPENGL */
}

#ifdef HAVE_OPENGL

static int ExtensionSupported(const char *extension)
{
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	/* Extension names should not have spaces. */
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
	      return 0;
	
	extensions = current_video->glGetString(GL_EXTENSIONS);
	/* It takes a bit of care to be fool-proof about parsing the
	 *      OpenGL extensions string. Don't be fooled by sub-strings,
	 *           etc. */
	
	start = extensions;
	
	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where) break;
		
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
	        if (*terminator == ' ' || *terminator == '\0') return 1;
						  
		start = terminator;
	}
	
	return 0;
}

/* Make the current context active */
int X11_GL_MakeCurrent(_THIS)
{
	int retval;
	
	retval = 0;
	if ( ! this->gl_data->glXMakeCurrent(GFX_Display,
	                                     SDL_Window, glx_context) ) {
		SDL_SetError("Unable to make GL context current");
		retval = -1;
	}
	XSync( GFX_Display, False );

	/* 
	 * The context is now current, check for glXReleaseBuffersMESA() 
	 * extension. If extension is _not_ supported, destroy the pointer 
	 * (to make sure it will not be called in X11_GL_Shutdown() ).
	 * 
	 * DRI/Mesa drivers include glXReleaseBuffersMESA() in the libGL.so, 
	 * but there's no need to call it (is is only needed for some old 
	 * non-DRI drivers).
	 * 
	 * When using for example glew (http://glew.sf.net), dlsym() for
	 * glXReleaseBuffersMESA() returns the pointer from the glew library
	 * (namespace conflict).
	 *
	 * The glXReleaseBuffersMESA() pointer in the glew is NULL, if the 
	 * driver doesn't support this extension. So blindly calling it will
	 * cause segfault with DRI/Mesa drivers!
	 * 
	 */
	
	if ( ! ExtensionSupported("glXReleaseBuffersMESA") ) {
		this->gl_data->glXReleaseBuffersMESA = NULL;
	}

	/* More Voodoo X server workarounds... Grr... */
	SDL_Lock_EventThread();
	X11_CheckDGAMouse(this);
	SDL_Unlock_EventThread();

	return(retval);
}

/* Get attribute data from glX. */
int X11_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value)
{
	int retval;
	int glx_attrib = None;

	switch( attrib ) {
	    case SDL_GL_RED_SIZE:
		glx_attrib = GLX_RED_SIZE;
		break;
	    case SDL_GL_GREEN_SIZE:
		glx_attrib = GLX_GREEN_SIZE;
		break;
	    case SDL_GL_BLUE_SIZE:
		glx_attrib = GLX_BLUE_SIZE;
		break;
	    case SDL_GL_ALPHA_SIZE:
		glx_attrib = GLX_ALPHA_SIZE;
		break;
	    case SDL_GL_DOUBLEBUFFER:
		glx_attrib = GLX_DOUBLEBUFFER;
		break;
	    case SDL_GL_BUFFER_SIZE:
		glx_attrib = GLX_BUFFER_SIZE;
		break;
	    case SDL_GL_DEPTH_SIZE:
		glx_attrib = GLX_DEPTH_SIZE;
		break;
	    case SDL_GL_STENCIL_SIZE:
		glx_attrib = GLX_STENCIL_SIZE;
		break;
	    case SDL_GL_ACCUM_RED_SIZE:
		glx_attrib = GLX_ACCUM_RED_SIZE;
		break;
	    case SDL_GL_ACCUM_GREEN_SIZE:
		glx_attrib = GLX_ACCUM_GREEN_SIZE;
		break;
	    case SDL_GL_ACCUM_BLUE_SIZE:
		glx_attrib = GLX_ACCUM_BLUE_SIZE;
		break;
	    case SDL_GL_ACCUM_ALPHA_SIZE:
		glx_attrib = GLX_ACCUM_ALPHA_SIZE;
		break;
	    case SDL_GL_STEREO:
		glx_attrib = GLX_STEREO;
		break;
 	    case SDL_GL_MULTISAMPLEBUFFERS:
 		glx_attrib = GLX_SAMPLE_BUFFERS_ARB;
 		break;
 	    case SDL_GL_MULTISAMPLESAMPLES:
 		glx_attrib = GLX_SAMPLES_ARB;
 		break;
	    default:
		return(-1);
	}

	retval = this->gl_data->glXGetConfig(GFX_Display, glx_visualinfo, glx_attrib, value);

	return retval;
}

void X11_GL_SwapBuffers(_THIS)
{
	this->gl_data->glXSwapBuffers(GFX_Display, SDL_Window);
}

#endif /* HAVE_OPENGL */

void X11_GL_UnloadLibrary(_THIS)
{
#ifdef HAVE_OPENGL
	if ( this->gl_config.driver_loaded ) {
		dlclose(this->gl_config.dll_handle);

		this->gl_data->glXGetProcAddress = NULL;
		this->gl_data->glXChooseVisual = NULL;
		this->gl_data->glXCreateContext = NULL;
		this->gl_data->glXDestroyContext = NULL;
		this->gl_data->glXMakeCurrent = NULL;
		this->gl_data->glXSwapBuffers = NULL;

		this->gl_config.dll_handle = NULL;
		this->gl_config.driver_loaded = 0;
	}
#endif
}

#ifdef HAVE_OPENGL

/* Passing a NULL path means load pointers from the application */
int X11_GL_LoadLibrary(_THIS, const char* path) 
{
	void* handle;
	int dlopen_flags;

 	if ( gl_active ) {
 		SDL_SetError("OpenGL context already created");
 		return -1;
 	}

#ifdef RTLD_GLOBAL
	dlopen_flags = RTLD_LAZY | RTLD_GLOBAL;
#else
	dlopen_flags = RTLD_LAZY;
#endif
	handle = dlopen(path, dlopen_flags);
	/* Catch the case where the application isn't linked with GL */
	if ( (dlsym(handle, "glXChooseVisual") == NULL) && (path == NULL) ) {
		dlclose(handle);
		path = getenv("SDL_VIDEO_GL_DRIVER");
		if ( path == NULL ) {
			path = DEFAULT_OPENGL;
		}
		handle = dlopen(path, dlopen_flags);
	}
	if ( handle == NULL ) {
		SDL_SetError("Could not load OpenGL library");
		return -1;
	}

	/* Unload the old driver and reset the pointers */
	X11_GL_UnloadLibrary(this);

	/* Load new function pointers */
	this->gl_data->glXGetProcAddress =
		(void *(*)(const GLubyte *)) dlsym(handle, "glXGetProcAddressARB");
	this->gl_data->glXChooseVisual =
		(XVisualInfo *(*)(Display *, int, int *)) dlsym(handle, "glXChooseVisual");
	this->gl_data->glXCreateContext =
		(GLXContext (*)(Display *, XVisualInfo *, GLXContext, int)) dlsym(handle, "glXCreateContext");
	this->gl_data->glXDestroyContext =
		(void (*)(Display *, GLXContext)) dlsym(handle, "glXDestroyContext");
	this->gl_data->glXMakeCurrent =
		(int (*)(Display *, GLXDrawable, GLXContext)) dlsym(handle, "glXMakeCurrent");
	this->gl_data->glXSwapBuffers =
		(void (*)(Display *, GLXDrawable)) dlsym(handle, "glXSwapBuffers");
	this->gl_data->glXGetConfig =
		(int (*)(Display *, XVisualInfo *, int, int *)) dlsym(handle, "glXGetConfig");
	this->gl_data->glXQueryExtensionsString =
		(const char *(*)(Display *, int)) dlsym(handle, "glXQueryExtensionsString");
	
	/* We don't compare below for this in case we're not using Mesa. */
	this->gl_data->glXReleaseBuffersMESA =
		(void (*)(Display *, GLXDrawable)) dlsym( handle, "glXReleaseBuffersMESA" );
	
	
	if ( (this->gl_data->glXChooseVisual == NULL) || 
	     (this->gl_data->glXCreateContext == NULL) ||
	     (this->gl_data->glXDestroyContext == NULL) ||
	     (this->gl_data->glXMakeCurrent == NULL) ||
	     (this->gl_data->glXSwapBuffers == NULL) ||
	     (this->gl_data->glXGetConfig == NULL) ||
	     (this->gl_data->glXQueryExtensionsString == NULL)) {
		SDL_SetError("Could not retrieve OpenGL functions");
		return -1;
	}

	this->gl_config.dll_handle = handle;
	this->gl_config.driver_loaded = 1;
	if ( path ) {
		strncpy(this->gl_config.driver_path, path,
			sizeof(this->gl_config.driver_path)-1);
	} else {
		strcpy(this->gl_config.driver_path, "");
	}
	return 0;
}

void *X11_GL_GetProcAddress(_THIS, const char* proc)
{
	static char procname[1024];
	void* handle;
	void* retval;
	
	handle = this->gl_config.dll_handle;
	if ( this->gl_data->glXGetProcAddress ) {
		return this->gl_data->glXGetProcAddress(proc);
	}
#if defined(__OpenBSD__) && !defined(__ELF__)
#undef dlsym(x,y);
#endif
	retval = dlsym(handle, proc);
	if (!retval && strlen(proc) <= 1022) {
		procname[0] = '_';
		strcpy(procname + 1, proc);
		retval = dlsym(handle, procname);
	}
	return retval;
}

#endif /* HAVE_OPENGL */
