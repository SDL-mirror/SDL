/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

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

#include <proto/ogles2.h>
#include <GLES2/gl2.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static struct Library *OGLES2base;
struct OGLES2IFace *IOGLES2;

void *AmiGetGLProc(const char *proc);

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
        OGLES2base = IExec->OpenLibrary("opengles2.library", 0);

        if (!OGLES2base) {
            dprintf("Failed to open opengles2.library\n");
            SDL_SetError("Failed to open opengles2.library");
            return -1;
        }
    }

    if (!IOGLES2) {
        IOGLES2 = (struct OGLES2IFace *) IExec->GetInterface(OGLES2base, "main", 1, NULL);

        if (!IOGLES2) {
            dprintf("Failed to open OpenGL ES 2 interace\n");
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
#if 0 // TODO: need own function table
    if (IOGLES2) {
        func = (void *)AmiGetGLProc(proc);
    }

    if (func == NULL) {
        dprintf("Failed to load '%s'\n", proc);
    }
#endif
    return func;
}

void
OS4_GLES_UnloadLibrary(_THIS)
{
    dprintf("Called %d\n", _this->gl_config.driver_loaded);

    if (IOGLES2) {
        IExec->DropInterface((struct Interface *) IOGLES2);
        IOGLES2 = NULL;
    }

    if (OGLES2base) {
        IExec->CloseLibrary(OGLES2base);
        OGLES2base = NULL;
    }
}

SDL_GLContext
OS4_GLES_CreateContext(_THIS, SDL_Window * window)
{
    dprintf("Called\n");

    if (IOGLES2) {

        int width, height;
        uint32 depth;
        ULONG errCode = 0;

        SDL_WindowData *data = window->driverdata;

        if (data->glContext) {
            dprintf("Old context %p found\n", data->glContext);

            aglDestroyContext(data->glContext);
            data->glContext = NULL;

#if MANAGE_BITMAP
            OS4_GL_FreeBuffers(_this, data);
#endif
        }

        depth = IGraphics->GetBitMapAttr(data->syswin->RPort->BitMap, BMA_BITSPERPIXEL);

        IIntuition->GetWindowAttrs(
                        data->syswin,
                        WA_InnerWidth, &width,
                        WA_InnerHeight, &height,
                        TAG_DONE);
#if MANAGE_BITMAP
        if (!OS4_GL_AllocateBuffers(_this, width, height, depth, data)) {
            SDL_SetError("Failed to allocate OpenGL ES 2 buffers");
            return NULL;
        }
#endif
        dprintf("Depth buffer size %d, stencil buffer size %d\n",
            _this->gl_config.depth_size, _this->gl_config.stencil_size);

        data->glContext = aglCreateContextTags(
            &errCode,
            OGLES2_CCT_WINDOW, data->syswin,
#if MANAGE_BITMAP
            OGLES2_CCT_BITMAP, data->glBackBuffer,
#endif
            OGLES2_CCT_DEPTH, _this->gl_config.depth_size,
            OGLES2_CCT_STENCIL, _this->gl_config.stencil_size,
            TAG_DONE);

        if (data->glContext) {

            dprintf("OpenGL ES 2 context created for window '%s'\n", window->title);

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
    dprintf("Called w=%p c=%p\n", window, context);

    if (IOGLES2) {

        if (window) {
            SDL_WindowData *data = window->driverdata;

            if (context != data->glContext) {
                dprintf("Context pointer mismatch: %p<>%p\n", context, data->glContext);
                SDL_SetError("Context pointer mismatch");
                return -1;
            }

            aglMakeCurrent(context);
        }
    } else {
        OS4_GLES_LogLibraryError();
    }

    return 0;
}

void
OS4_GLES_SwapWindow(_THIS, SDL_Window * window)
{
    //dprintf("Called\n");

    if (IOGLES2) {

        SDL_WindowData *data = window->driverdata;

        if (data->glContext) {
            SDL_VideoData *videodata = _this->driverdata;
#if MANAGE_BITMAP
            struct BitMap *temp;
#endif
            int w, h;

            LONG ret = IIntuition->GetWindowAttrs(data->syswin,
                WA_InnerWidth, &w,
                WA_InnerHeight, &h,
                TAG_DONE);

            if (ret) {
                dprintf("GetWindowAttrs() returned %d\n", ret);
            }

            if (videodata->vsyncEnabled) {
                IGraphics->WaitTOF();
            }

            aglSwapBuffers();

#if MANAGE_BITMAP
            IGraphics->BltBitMapRastPort(data->glBackBuffer, 0, 0, data->syswin->RPort,
                data->syswin->BorderLeft, data->syswin->BorderTop, w, h, 0xC0);

            /* copy back into front */
            IGraphics->BltBitMapTags(BLITA_Source,  data->glBackBuffer,
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

            temp = data->glFrontBuffer;
            data->glFrontBuffer = data->glBackBuffer;
            data->glBackBuffer = temp;

            aglSetBitmap(data->glBackBuffer);
#endif
        } else {
            dprintf("No OpenGL ES 2 context\n");
        }
    } else {
        OS4_GLES_LogLibraryError();
    }
}

void
OS4_GLES_DeleteContext(_THIS, SDL_GLContext context)
{
    dprintf("Called\n");

    if (IOGLES2) {

        if (context) {
            aglDestroyContext(context);
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
            uint32 depth;

            OS4_GL_FreeBuffers(_this, data);

            depth = IGraphics->GetBitMapAttr(data->syswin->RPort->BitMap, BMA_BITSPERPIXEL);

            if (OS4_GL_AllocateBuffers(_this, window->w, window->h, depth, data)) {

                dprintf("Resizing context to %d*%d\n", window->w, window->h);

                aglSetBitmap(data->glBackBuffer);

                glViewport(0, 0, window->w, window->h);
                return SDL_TRUE;

            } else {
                dprintf("Failed to re-allocate OpenGL buffers\n");
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

#endif /* SDL_VIDEO_OPENGL_ES2 */

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
