/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

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
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_AMIGAOS4

#include "SDL_os4video.h"
#include "SDL_os4window.h"

#include <GL/gl.h>
//#include <mgl/gl.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

/* The client program needs access to this context pointer
 * to be able to make GL calls. This presents no problems when
 * it is statically linked against libSDL, but when linked
 * against a shared library version this will require some
 * trickery.
 */
struct GLContextIFace *mini_CurrentContext = 0;


void *AmiGetGLProc(const char *proc);

// TODO: fix driver_loaded usage. It's a counter

int OS4_GL_LoadLibrary(_THIS, const char *path)
{
	dprintf("Called %d\n", _this->gl_config.driver_loaded);

	// MiniGL was loaded during CreateDevice(), so we don't have to anything here

	return (_this->gl_config.driver_loaded) ? 0 : -1;
}

void *OS4_GL_GetProcAddress(_THIS, const char *proc)
{
	void *func = NULL;

	dprintf("Called for '%s'\n", proc);

	if (_this->gl_config.driver_loaded) {
		func = (void *)AmiGetGLProc(proc);
	}
	
	if (func == NULL) {
		dprintf("Failed to load '%s'\n", proc);
	}

	return func;
}

void OS4_GL_UnloadLibrary(_THIS)
{
	dprintf("Called\n");

	// Do nothing
}

static SDL_bool OS4_GL_AllocateBuffers(_THIS, int width, int height, int depth, SDL_WindowData * data)
{
	dprintf("Allocate double buffer bitmaps %d*%d*%d\n", width, height, depth);

	if (!(data->glFrontBuffer = IGraphics->AllocBitMapTags(
									width,
									height,
									depth,
									BMATags_Displayable, TRUE,
									BMATags_Friend, data->syswin->RPort->BitMap,
									TAG_DONE))) {

		dprintf("Failed to allocate front buffer\n");
		return SDL_FALSE;
	}

	if (!(data->glBackBuffer = IGraphics->AllocBitMapTags(
									width,
									height,
									depth,
									BMATags_Displayable, TRUE,
									BMATags_Friend, data->syswin->RPort->BitMap,
									TAG_DONE))) {

		dprintf("Failed to allocate back buffer\n");

		IGraphics->FreeBitMap(data->glFrontBuffer);
		data->glFrontBuffer = NULL;

		return SDL_FALSE;
	}

	return SDL_TRUE;
}

void OS4_GL_FreeBuffers(_THIS, SDL_WindowData * data)
{
	dprintf("Called\n");

	if (data->glFrontBuffer) {
		IGraphics->FreeBitMap(data->glFrontBuffer);
		data->glFrontBuffer = NULL;
	}

	if (data->glBackBuffer) {
		IGraphics->FreeBitMap(data->glBackBuffer);
		data->glBackBuffer = NULL;
	}
}

// TODO: what about resizable windows? Does SDL ask us for a new context?

SDL_GLContext OS4_GL_CreateContext(_THIS, SDL_Window * window)
{
	dprintf("Called\n");

	if (!_this->gl_config.driver_loaded) {
		// We might have deleted context earlier...
		if (IMiniGL) {
			dprintf("Reactivate OpenGL\n");
			_this->gl_config.driver_loaded = 1;
		}
	}

	if (_this->gl_config.driver_loaded) {

		int width, height;
        int depth = 16; // TODO: should we read this from screen?

		SDL_WindowData * data = window->driverdata;

		if (data->IGL) {
			dprintf("Old context %p found\n", data->IGL);

			data->IGL->DeleteContext();
			data->IGL = NULL;

			OS4_GL_FreeBuffers(_this, data);
		}

		IIntuition->GetWindowAttrs(
						data->syswin,
						WA_InnerWidth, &width,
						WA_InnerHeight, &height,
						TAG_DONE);

		if (!OS4_GL_AllocateBuffers(_this, width, height, depth, data)) {
			_this->gl_config.driver_loaded = 0;
			SDL_SetError("Failed to allocate OpenGL buffers");
			return NULL;
		}

		data->IGL = IMiniGL->CreateContextTags(
						MGLCC_PrivateBuffers,   2,
						MGLCC_FrontBuffer,      data->glFrontBuffer,
						MGLCC_BackBuffer,       data->glBackBuffer,
						MGLCC_Buffers,          2,
						MGLCC_PixelDepth,       depth,
						MGLCC_StencilBuffer,    TRUE,
						MGLCC_VertexBufferSize, 1 << 17,
						TAG_DONE);

		if (data->IGL) {

			dprintf("GL context created for window '%s'\n", window->title);

			data->IGL->GLViewport(0, 0, width, height);
			mglMakeCurrent(data->IGL);
			mglLockMode(MGL_LOCK_SMART);

			return data->IGL;

		} else {
			dprintf("Failed to create OpenGL context for window '%s'\n", window->title);

			SDL_SetError("Failed to create OpenGL context");

			_this->gl_config.driver_loaded = 0;

			OS4_GL_FreeBuffers(_this, data);

			return NULL;
		}

	} else {
		dprintf("No OpenGL\n");
		SDL_SetError("OpenGL not available");
		return NULL;
	}
}

int OS4_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
	dprintf("Called w=%p c=%p\n", window, context);

	if (_this->gl_config.driver_loaded) {

		if (window) {
			SDL_WindowData * data = window->driverdata;

			if (context != data->IGL) {
				dprintf("Context pointer mismatch: %p<>%p\n", context, data->IGL);
				SDL_SetError("Context pointer mismatch");
				return -1;
			}

			mglMakeCurrent(context);
		}
		// TODO: is there anything to clear in MiniGL in case of NULL context?

	} else {
		dprintf("No OpenGL\n");
		SDL_SetError("OpenGL not available");
	}

	return 0;
}

void OS4_GL_GetDrawableSize(_THIS, SDL_Window * window, int *w, int *h)
{
	if (_this->gl_config.driver_loaded) {
		SDL_WindowData * data = window->driverdata;
		int width, height;

		IIntuition->GetWindowAttrs(
						data->syswin,
						WA_InnerWidth, &width,
						WA_InnerHeight, &height,
						TAG_DONE);
		*w = width;
		*h = height;

		dprintf("w=%d, h=%d\n", *w, *h);
	} else {
		dprintf("No OpenGL\n");
		SDL_SetError("OpenGL not available");
	}

}

int OS4_GL_SetSwapInterval(_THIS, int interval)
{
	dprintf("Called\n");

	if (_this->gl_config.driver_loaded) {
		// implement me
	} else {
		dprintf("No OpenGL\n");
		SDL_SetError("OpenGL not available");
	}

	return 0;
}

int OS4_GL_GetSwapInterval(_THIS)
{
	dprintf("Called\n");

	if (_this->gl_config.driver_loaded) {
		// implement me
	} else {
		dprintf("No OpenGL\n");
		SDL_SetError("OpenGL not available");
	}

	return 0;
}

void OS4_GL_SwapWindow(_THIS, SDL_Window * window)
{
	//dprintf("Called\n");

	if (_this->gl_config.driver_loaded) {

		int w, h;
	    GLint buf;
		struct BitMap *temp;
		SDL_WindowData * data = window->driverdata;

		mglUnlockDisplay();

		IIntuition->GetWindowAttrs(data->syswin, WA_InnerWidth, &w, WA_InnerHeight, &h, TAG_DONE);

		/* besure all has finished before we start blitting (testing to find lockup cause) */
		data->IGL->MGLWaitGL(); // TODO: still needed?

		glGetIntegerv(GL_DRAW_BUFFER, &buf);

		if (buf == GL_BACK)
        {
			IGraphics->BltBitMapRastPort(data->glBackBuffer, 0, 0, data->syswin->RPort,
			    data->syswin->BorderLeft, data->syswin->BorderTop, w, h, 0xC0);
        }
		else if (buf == GL_FRONT)
        {
			IGraphics->BltBitMapRastPort(data->glFrontBuffer, 0, 0, data->syswin->RPort,
			    data->syswin->BorderLeft, data->syswin->BorderTop, w, h, 0xC0);
        }

        /* copy back into front */
		IGraphics->BltBitMapTags(BLITA_Source,	data->glBackBuffer,
								 	 BLITA_SrcType,	BLITT_BITMAP,
 								 	 BLITA_SrcX,	0,
 								 	 BLITA_SrcY,	0,
									 BLITA_Dest,	data->glFrontBuffer,
								 	 BLITA_DestType,BLITT_BITMAP,
								 	 BLITA_DestX,	0,
								 	 BLITA_DestY,	0,
								 	 BLITA_Width,	w,
								 	 BLITA_Height,	h,
								 	 BLITA_Minterm,	0xC0,
								 	 TAG_DONE);

		temp = data->glFrontBuffer;
		data->glFrontBuffer = data->glBackBuffer;
		data->glBackBuffer = temp;

		data->IGL->MGLUpdateContextTags(
							MGLCC_FrontBuffer,data->glFrontBuffer,
							MGLCC_BackBuffer, data->glBackBuffer,
							TAG_DONE);

	} else {
		dprintf("No OpenGL\n");
		SDL_SetError("OpenGL not available");
	}
}

void OS4_GL_DeleteContext(_THIS, SDL_GLContext context)
{
	dprintf("Called\n");

	if (_this->gl_config.driver_loaded) {
		
		if (context) {
			struct GLContextIFace * IGL = context;
			IGL->DeleteContext();
		}

		_this->gl_config.driver_loaded = 0;

	} else {
		dprintf("No OpenGL\n");
		SDL_SetError("OpenGL not available");
	}
}

SDL_bool OS4_GL_ResizeContext(_THIS, SDL_Window * window)
{
	if (_this->gl_config.driver_loaded) {
		SDL_WindowData *data = window->driverdata;

		if (data) {

			OS4_GL_FreeBuffers(_this, data);

			if (OS4_GL_AllocateBuffers(_this, window->w, window->h, 16 /* FIXME */, data)) {

				dprintf("Resizing context to %d*%d\n", window->w, window->h);

				data->IGL->MGLUpdateContextTags(
								MGLCC_FrontBuffer, data->glFrontBuffer,
								MGLCC_BackBuffer, data->glBackBuffer,
								TAG_DONE);

				data->IGL->GLViewport(0, 0, window->w, window->h);

				return SDL_TRUE;

			} else {
				dprintf("Failed to re-allocate OpenGL buffers\n");
				//SDL_Quit();
			}
		}
	} else {
		dprintf("No OpenGL\n");
	}

	return SDL_FALSE;
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
