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

#include <android/log.h>

#include <pthread.h>

/*
These things are in the JNI android support
*/
extern void Android_CreateContext();
extern void Android_Render();

/* GL functions */
int Android_GL_LoadLibrary(_THIS, const char *path){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_LoadLibrary\n");
	return 0;
}

void *Android_GL_GetProcAddress(_THIS, const char *proc){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_GetProcAddress\n");
	return 0;
}

void Android_GL_UnloadLibrary(_THIS){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_UnloadLibrary\n");
}

/*
int *Android_GL_GetVisual(_THIS, Display * display, int screen){
	__android_log_print(ANDROID_LOG_INFO, "SDL","[STUB] GL_GetVisual\n");
	return 0;
}
*/

SDL_GLContext Android_GL_CreateContext(_THIS, SDL_Window * window){
	Android_CreateContext();
	return 1;
}

int Android_GL_MakeCurrent(_THIS, SDL_Window * window,
                              SDL_GLContext context){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_MakeCurrent\n");
	return 0;
}

int Android_GL_SetSwapInterval(_THIS, int interval){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_SetSwapInterval\n");
	return 0;
}

int Android_GL_GetSwapInterval(_THIS){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_GetSwapInterval\n");
	return 0;
}

void Android_GL_SwapWindow(_THIS, SDL_Window * window){
	Android_Render();
}

void Android_GL_DeleteContext(_THIS, SDL_GLContext context){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_DeleteContext\n");
}
