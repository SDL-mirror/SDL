/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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
#include "SDL_config.h"

/* Android SDL video driver implementation
*/

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_androidvideo.h"
#include "SDL_androidevents.h"
#include "SDL_androidrender.h"

/* Android header */
#include "egl.h"


//EGL globals
static EGLDisplay iEglDisplay; 	
static EGLConfig  iEglConfig;   	
static EGLContext iEglContext; 	
static EGLSurface iEglSurface;	

EGLint attribList [] = 
{ 
	EGL_BUFFER_SIZE, 16, //16 bit color
	EGL_DEPTH_SIZE, 15,
	EGL_NONE 
};	




/* GL functions */
int Android_GL_LoadLibrary(_THIS, const char *path){
	printf("[STUB] GL_LoadLibrary\n");
	return 0;
}

void *Android_GL_GetProcAddress(_THIS, const char *proc){
	printf("[STUB] GL_GetProcAddress\n");
	return 0;
}

void Android_GL_UnloadLibrary(_THIS){
	printf("[STUB] GL_UnloadLibrary\n");
}

/*
int *Android_GL_GetVisual(_THIS, Display * display, int screen){
	printf("[STUB] GL_GetVisual\n");
	return 0;
}
*/

SDL_GLContext Android_GL_CreateContext(_THIS, SDL_Window * window){
	printf("[STUB] GL_CreateContext\n");

	//Start up the display
	iEglDisplay = eglGetDisplay (EGL_DEFAULT_DISPLAY);
	if(iEglDisplay == EGL_NO_DISPLAY){
		printf("Unable to find a  suitable EGLDisplay\n");
		return NULL;
	}

	printf("1\n");
		
	if(!eglInitialize(iEglDisplay, 0, 0)){
		printf("Couldn't init display\n");
		return NULL;
	}

	printf("2\n");
	
	EGLint numConfigs;
	
	if(!eglChooseConfig(iEglDisplay, attribList, &iEglConfig, 1, &numConfigs)){
		printf("Couldn't choose config\n");
		return NULL;
	}

	printf("3\n");
	 
	iEglContext = eglCreateContext(iEglDisplay, iEglConfig, EGL_NO_CONTEXT, 0);
		
	if(iEglContext == 0){
		printf("Couldn't create context\n");
		return NULL;
	}

	printf("4\n");
		
	NativeWindowType iWindow = 1; //android_createDisplaySurface();
	
	iEglSurface = eglCreateWindowSurface(iEglDisplay, iEglConfig, iWindow, 0);	

	printf("5\n");
	
	if(iEglSurface == NULL){
		printf("Couldn't create surface\n");
		return NULL;
	}

	printf("6\n");
	
	eglMakeCurrent(iEglDisplay, iEglSurface, iEglSurface, iEglContext);

	printf("fininshed making context\n");
	
	return iEglSurface;
}

int Android_GL_MakeCurrent(_THIS, SDL_Window * window,
                              SDL_GLContext context){
	printf("[STUB] GL_MakeCurrent\n");
	return 0;
}

int Android_GL_SetSwapInterval(_THIS, int interval){
	printf("[STUB] GL_SetSwapInterval\n");
	return 0;
}

int Android_GL_GetSwapInterval(_THIS){
	printf("[STUB] GL_GetSwapInterval\n");
	return 0;
}

void Android_GL_SwapWindow(_THIS, SDL_Window * window){
	printf("[STUB] GL_SwapWindow\n");
}

void Android_GL_DeleteContext(_THIS, SDL_GLContext context){
	printf("[STUB] GL_DeleteContext\n");
}
