/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "SDL_bopengl.h"
#include "../SDL_sysvideo.h"


/* Passing a NULL path means load pointers from the application */
int BE_GL_LoadLibrary(_THIS, const char *path)
{
#if 0
	if (path == NULL) {
		if (_this->gl_config.dll_handle == NULL) {
			image_info info;
			int32 cookie = 0;
			while (get_next_image_info(0, &cookie, &info) == B_OK) {
				void *location = NULL;
				if (get_image_symbol
				((image_id) cookie, "glBegin",
				B_SYMBOL_TYPE_ANY, &location) == B_OK) {
					_this->gl_config.dll_handle = (void *) cookie;
					_this->gl_config.driver_loaded = 1;
					SDL_strlcpy(_this->gl_config.driver_path,
					"libGL.so",
					SDL_arraysize(_this->
					gl_config.driver_path));
				}
			}
		}
	} else {
        /*
           FIXME None of BeOS libGL.so implementations have exported functions 
           to load BGLView, which should be reloaded from new lib.
           So for now just "load" linked libGL.so :(
        */
        if (_this->gl_config.dll_handle == NULL) {
        	return BE_GL_LoadLibrary(_this, NULL);
        }

            /* Unload old first */
            /*if (_this->gl_config.dll_handle != NULL) { */
            /* Do not try to unload application itself (if LoadLibrary was called before with NULL ;) */
            /*      image_info info;
               if (get_image_info((image_id)_this->gl_config.dll_handle, &info) == B_OK) {
               if (info.type != B_APP_IMAGE) {
               unload_add_on((image_id)_this->gl_config.dll_handle);
               }
               }

               }

               if ((_this->gl_config.dll_handle = (void*)load_add_on(path)) != (void*)B_ERROR) {
               _this->gl_config.driver_loaded = 1;
               SDL_strlcpy(_this->gl_config.driver_path, path, SDL_arraysize(_this->gl_config.driver_path));
               } */
	}
	
	if (_this->gl_config.dll_handle != NULL) {
		return 0;
	} else {
		_this->gl_config.dll_handle = NULL;
		_this->gl_config.driver_loaded = 0;
		*_this->gl_config.driver_path = '\0';
		return -1;
	}
#endif
}

void *BE_GL_GetProcAddress(_THIS, const char *proc)
{
#if 0
	if (_this->gl_config.dll_handle != NULL) {
		void *location = NULL;
		status_t err;
		if ((err =
			get_image_symbol((image_id) _this->gl_config.dll_handle,
                              proc, B_SYMBOL_TYPE_ANY,
                              &location)) == B_OK) {
            return location;
        } else {
                SDL_SetError("Couldn't find OpenGL symbol");
                return NULL;
        }
	} else {
		SDL_SetError("OpenGL library not loaded");
		return NULL;
	}
#endif
}




int BE_GL_MakeCurrent(_THIS)
{
	/* FIXME: should we glview->unlock and then glview->lock()? */
	return 0;
}












#if 0 /* Functions from 1.2 that do not appear to be used in 1.3 */

    int BE_GL_GetAttribute(_THIS, SDL_GLattr attrib, int *value)
    {
        /*
           FIXME? Right now BE_GL_GetAttribute shouldn't be called between glBegin() and glEnd() - it doesn't use "cached" values
         */
        switch (attrib) {
        case SDL_GL_RED_SIZE:
            glGetIntegerv(GL_RED_BITS, (GLint *) value);
            break;
        case SDL_GL_GREEN_SIZE:
            glGetIntegerv(GL_GREEN_BITS, (GLint *) value);
            break;
        case SDL_GL_BLUE_SIZE:
            glGetIntegerv(GL_BLUE_BITS, (GLint *) value);
            break;
        case SDL_GL_ALPHA_SIZE:
            glGetIntegerv(GL_ALPHA_BITS, (GLint *) value);
            break;
        case SDL_GL_DOUBLEBUFFER:
            glGetBooleanv(GL_DOUBLEBUFFER, (GLboolean *) value);
            break;
        case SDL_GL_BUFFER_SIZE:
            int v;
            glGetIntegerv(GL_RED_BITS, (GLint *) & v);
            *value = v;
            glGetIntegerv(GL_GREEN_BITS, (GLint *) & v);
            *value += v;
            glGetIntegerv(GL_BLUE_BITS, (GLint *) & v);
            *value += v;
            glGetIntegerv(GL_ALPHA_BITS, (GLint *) & v);
            *value += v;
            break;
        case SDL_GL_DEPTH_SIZE:
            glGetIntegerv(GL_DEPTH_BITS, (GLint *) value);      /* Mesa creates 16 only? r5 always 32 */
            break;
        case SDL_GL_STENCIL_SIZE:
            glGetIntegerv(GL_STENCIL_BITS, (GLint *) value);
            break;
        case SDL_GL_ACCUM_RED_SIZE:
            glGetIntegerv(GL_ACCUM_RED_BITS, (GLint *) value);
            break;
        case SDL_GL_ACCUM_GREEN_SIZE:
            glGetIntegerv(GL_ACCUM_GREEN_BITS, (GLint *) value);
            break;
        case SDL_GL_ACCUM_BLUE_SIZE:
            glGetIntegerv(GL_ACCUM_BLUE_BITS, (GLint *) value);
            break;
        case SDL_GL_ACCUM_ALPHA_SIZE:
            glGetIntegerv(GL_ACCUM_ALPHA_BITS, (GLint *) value);
            break;
        case SDL_GL_STEREO:
        case SDL_GL_MULTISAMPLEBUFFERS:
        case SDL_GL_MULTISAMPLESAMPLES:
        default:
            *value = 0;
            return (-1);
        }
        return 0;
    }

    void BE_GL_SwapBuffers(_THIS)
    {
        SDL_Win->SwapBuffers();
    }
#endif
