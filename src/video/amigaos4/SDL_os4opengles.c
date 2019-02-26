/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

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

#if SDL_VIDEO_OPENGL_ES2

#include "SDL_os4video.h"
#include "SDL_os4window.h"
#include "SDL_os4opengl.h"
#include "SDL_os4opengles.h"
#include "SDL_os4library.h"

#include <proto/ogles2.h>
#include <GLES2/gl2.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static struct Library *OGLES2base;
struct OGLES2IFace *IOGLES2;

void *AmiGetGLESProc(const char *proc);

static void
OS4_GLES_LogLibraryError()
{
    dprintf("No OpenGL ES 2 library available\n");
    SDL_SetError("No OpenGL ES 2 library available");
}

int
OS4_GLES_LoadLibrary(_THIS, const char * path)
{
    dprintf("Called %d\n", _this->gl_config.driver_loaded);

    if (!OGLES2base) {
        OGLES2base = OS4_OpenLibrary("ogles2.library", 0);

        if (!OGLES2base) {
            dprintf("Failed to open ogles2.library\n");
            SDL_SetError("Failed to open ogles2.library");
            return -1;
        }
    }

    if (!IOGLES2) {
        IOGLES2 = (struct OGLES2IFace *) OS4_GetInterface(OGLES2base);

        if (!IOGLES2) {
            dprintf("Failed to open OpenGL ES 2 interface\n");
            SDL_SetError("Failed to open OpenGL ES 2 interface");
            return -1;
        }

        dprintf("OpenGL ES 2 library opened\n");
    }

    return 0;
}

void *
OS4_GLES_GetProcAddress(_THIS, const char * proc)
{
    void *func = NULL;

    dprintf("Called for '%s'\n", proc);

    if (IOGLES2) {
        func = AmiGetGLESProc(proc);
    }

    if (func == NULL) {
        dprintf("Failed to load '%s'\n", proc);
        SDL_SetError("Failed to load function");
    }

    return func;
}

void
OS4_GLES_UnloadLibrary(_THIS)
{
    dprintf("Called %d\n", _this->gl_config.driver_loaded);

    OS4_DropInterface((void *) &IOGLES2);
    OS4_CloseLibrary(&OGLES2base);
}

SDL_GLContext
OS4_GLES_CreateContext(_THIS, SDL_Window * window)
{
    dprintf("Called\n");

    if (IOGLES2) {

        int width, height;

        OS4_GetWindowActiveSize(window, &width, &height);

#if MANAGE_BITMAP
        uint32 depth;
#endif

        ULONG errCode = 0;

        SDL_WindowData *data = window->driverdata;

        if (data->glContext) {
            dprintf("Old context %p found, deleting\n", data->glContext);

            aglDestroyContext(data->glContext);

            data->glContext = NULL;
        }

#if MANAGE_BITMAP
        depth = IGraphics->GetBitMapAttr(data->syswin->RPort->BitMap, BMA_BITSPERPIXEL);

        if (!OS4_GL_AllocateBuffers(_this, width, height, depth, data)) {
            SDL_SetError("Failed to allocate OpenGL ES 2 buffers");
            return NULL;
        }
#endif
        dprintf("Depth buffer size %d, stencil buffer size %d\n",
            _this->gl_config.depth_size, _this->gl_config.stencil_size);

        data->glContext = aglCreateContextTags2(
            &errCode,
            OGLES2_CCT_WINDOW, (ULONG)data->syswin,
            OGLES2_CCT_VSYNC, 0,
#if MANAGE_BITMAP
            OGLES2_CCT_BITMAP, (ULONG)data->glBackBuffer,
#endif
            OGLES2_CCT_DEPTH, _this->gl_config.depth_size,
            OGLES2_CCT_STENCIL, _this->gl_config.stencil_size,
            TAG_DONE);

        if (data->glContext) {

            dprintf("OpenGL ES 2 context %p created for window '%s'\n",
                data->glContext, window->title);

            aglMakeCurrent(data->glContext);
            glViewport(0, 0, width, height);
            return data->glContext;

        } else {
            dprintf("Failed to create OpenGL ES 2 context for window '%s' (error code %d)\n",
                window->title, errCode);

            SDL_SetError("Failed to create OpenGL ES 2 context");
#if MANAGE_BITMAP
            OS4_GL_FreeBuffers(_this, data);
#endif
            return NULL;
        }

    } else {
        OS4_GLES_LogLibraryError();
        return NULL;
    }

    return NULL;
}

int
OS4_GLES_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
    int result = -1;

    if (!window || !context) {
        dprintf("Called window=%p context=%p\n", window, context);
    }

    if (IOGLES2) {

        if (window) {
            SDL_WindowData *data = window->driverdata;

            if (context != data->glContext) {
                dprintf("Context pointer mismatch: %p<>%p\n", context, data->glContext);
                SDL_SetError("Context pointer mismatch");
            } else {
                aglMakeCurrent(context);
            }
        }

        result = 0;

    } else {
        OS4_GLES_LogLibraryError();
    }

    return result;
}

int
OS4_GLES_SwapWindow(_THIS, SDL_Window * window)
{
    //dprintf("Called\n");

    if (IOGLES2) {

        SDL_WindowData *data = window->driverdata;

        if (data->glContext) {
            SDL_VideoData *videodata = _this->driverdata;

#if MANAGE_BITMAP
            struct BitMap *temp;
            int w, h;
            BOOL blitRpRet;
            int32 blitRet;

            OS4_GetWindowSize(_this, data->syswin, &w, &h);
#endif

            glFinish();

            if (videodata->vsyncEnabled) {
                IGraphics->WaitTOF();
            }

            aglSwapBuffers();

#if MANAGE_BITMAP
            blitRpRet = IGraphics->BltBitMapRastPort(data->glBackBuffer, 0, 0, data->syswin->RPort,
                data->syswin->BorderLeft, data->syswin->BorderTop, w, h, 0xC0);

            if (!blitRpRet) {
                dprintf("BltBitMapRastPort() failed\n");
            }

            blitRet = IGraphics->BltBitMapTags(BLITA_Source,  data->glBackBuffer,
                                     BLITA_SrcType, BLITT_BITMAP,
                                     BLITA_SrcX,    0,
                                     BLITA_SrcY,    0,
                                     BLITA_Dest,    data->glFrontBuffer,
                                     BLITA_DestType,BLITT_BITMAP,
                                     BLITA_DestX,   0,
                                     BLITA_DestY,   0,
                                     BLITA_Width,   w,
                                     BLITA_Height,  h,
                                     BLITA_Minterm, 0xC0,
                                     TAG_DONE);

            if (blitRet == -1) {
                temp = data->glFrontBuffer;
                data->glFrontBuffer = data->glBackBuffer;
                data->glBackBuffer = temp;

                aglSetBitmap(data->glBackBuffer);

                return 0;
            } else {
                dprintf("BltBitMapTags() returned %d\n", blitRet);
            }
#endif
        } else {
            dprintf("No OpenGL ES 2 context\n");
        }
    } else {
        OS4_GLES_LogLibraryError();
    }

    return -1;
}

void
OS4_GLES_DeleteContext(_THIS, SDL_GLContext context)
{
    dprintf("Called with context=%p\n", context);

    if (IOGLES2) {

        if (context) {
            SDL_Window *sdlwin;
            Uint32 deletions = 0;

            for (sdlwin = _this->windows; sdlwin; sdlwin = sdlwin->next) {

                SDL_WindowData *data = sdlwin->driverdata;

                if (data->glContext == context) {
                    dprintf("Found OpenGL ES 2 context, clearing window binding\n");

                    aglDestroyContext(context);

                    data->glContext = NULL;
                    deletions++;
                }
            }

            if (deletions == 0) {
                dprintf("OpenGL ES 2 context doesn't seem to have window binding\n");
            }
        } else {
            dprintf("No context to delete\n");
        }

    } else {
        OS4_GLES_LogLibraryError();
    }
}

SDL_bool
OS4_GLES_ResizeContext(_THIS, SDL_Window * window)
{
    if (IOGLES2) {
#if MANAGE_BITMAP
        SDL_WindowData *data = window->driverdata;

        if (data) {
            int width, height;

            uint32 depth = IGraphics->GetBitMapAttr(data->syswin->RPort->BitMap, BMA_BITSPERPIXEL);

            OS4_GetWindowActiveSize(window, &width, &height);

            if (OS4_GL_AllocateBuffers(_this, width, height, depth, data)) {

                dprintf("Resizing context to %d*%d\n", width, height);

                aglSetBitmap(data->glBackBuffer);

                glViewport(0, 0, width, height);
                return SDL_TRUE;

            } else {
                dprintf("Failed to re-allocate OpenGL ES 2 buffers\n");
                //SDL_Quit();
            }
        }
#endif
        return SDL_TRUE;
    } else {
        OS4_GLES_LogLibraryError();
    }

    return SDL_FALSE;
}

void
OS4_GLES_UpdateWindowPointer(_THIS, SDL_Window * window)
{
    if (IOGLES2) {
        SDL_WindowData *data = window->driverdata;

        dprintf("Updating GLES2 window pointer %p\n", data->syswin);
        aglSetParamsTags2(OGLES2_CCT_WINDOW, (ULONG)data->syswin, TAG_DONE);
    } else {
        OS4_GLES_LogLibraryError();
    }
}

#endif /* SDL_VIDEO_OPENGL_ES2 */

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
