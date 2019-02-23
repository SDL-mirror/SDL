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

#include "SDL_os4video.h"
#include "SDL_os4window.h"
#include "SDL_os4library.h"
#include "SDL_os4opengl.h"

#include <proto/minigl.h>

#include <GL/gl.h>
//#include <mgl/gl.h>

#include <unistd.h> // usleep

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

struct Library *MiniGLBase;
struct MiniGLIFace *IMiniGL;

/* The client program needs access to this context pointer
 * to be able to make GL calls. This presents no problems when
 * it is statically linked against libSDL, but when linked
 * against a shared library version this will require some
 * trickery.
 */
DECLSPEC struct GLContextIFace *mini_CurrentContext = 0;

void *AmiGetGLProc(const char *proc);

static void
OS4_GL_LogLibraryError()
{
    dprintf("No MiniGL library available\n");
    SDL_SetError("No MiniGL library available");
}

int
OS4_GL_LoadLibrary(_THIS, const char * path)
{
    dprintf("Called %d\n", _this->gl_config.driver_loaded);

    if (!MiniGLBase) {
        MiniGLBase = OS4_OpenLibrary("minigl.library", 2);

        if (!MiniGLBase) {
            dprintf("Failed to open minigl.library\n");
            SDL_SetError("Failed to open minigl.library");
            return -1;
        }
    }

    if (!IMiniGL) {
        IMiniGL = (struct MiniGLIFace *) OS4_GetInterface(MiniGLBase);

        if (!IMiniGL) {
            dprintf("Failed to open MiniGL interace\n");
            SDL_SetError("Failed to open MiniGL interface");
            return -1;
        }

        dprintf("MiniGL library opened\n");
    }

    return 0;
}

void *
OS4_GL_GetProcAddress(_THIS, const char * proc)
{
    void *func = NULL;

    dprintf("Called for '%s'\n", proc);

    if (IMiniGL) {
        func = AmiGetGLProc(proc);
    }

    if (func == NULL) {
        dprintf("Failed to load '%s'\n", proc);
    }

    return func;
}

void
OS4_GL_UnloadLibrary(_THIS)
{
    dprintf("Called %d\n", _this->gl_config.driver_loaded);

    OS4_DropInterface((void *) &IMiniGL);
    OS4_CloseLibrary(&MiniGLBase);
}

SDL_bool
OS4_GL_AllocateBuffers(_THIS, int width, int height, int depth, SDL_WindowData * data)
{
    dprintf("Allocate double buffer bitmaps %d*%d*%d\n", width, height, depth);

    if (data->glFrontBuffer || data->glBackBuffer) {
        dprintf("Old front buffer pointer %p, back buffer pointer %p\n",
            data->glFrontBuffer, data->glBackBuffer);

        OS4_GL_FreeBuffers(_this, data);
    }

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

    uint32 srcFmt = IGraphics->GetBitMapAttr(data->glBackBuffer, BMA_PIXELFORMAT);
    uint32 src2Fmt = IGraphics->GetBitMapAttr(data->glFrontBuffer, BMA_PIXELFORMAT);
    uint32 dstFmt = IGraphics->GetBitMapAttr(data->syswin->RPort->BitMap, BMA_PIXELFORMAT);

    dprintf("SRC FMT %u, SRC2 FMT %u, DST FMT %u\n", srcFmt, src2Fmt, dstFmt);

    return SDL_TRUE;
}

void
OS4_GL_FreeBuffers(_THIS, SDL_WindowData * data)
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

SDL_GLContext
OS4_GL_CreateContext(_THIS, SDL_Window * window)
{
    dprintf("Called\n");

    if (IMiniGL) {

        int width, height;
        uint32 depth;

        SDL_WindowData * data = window->driverdata;

        OS4_GetWindowActiveSize(window, &width, &height);

        if (data->glContext) {
            struct GLContextIFace *IGL = (struct GLContextIFace *)data->glContext;

            dprintf("Old context %p found, deleting\n", data->glContext);

            IGL->DeleteContext();

            data->glContext = NULL;
        }

        depth = IGraphics->GetBitMapAttr(data->syswin->RPort->BitMap, BMA_BITSPERPIXEL);

        if (!OS4_GL_AllocateBuffers(_this, width, height, depth, data)) {
            SDL_SetError("Failed to allocate MiniGL buffers");
            return NULL;
        }

        data->glContext = IMiniGL->CreateContextTags(
                        MGLCC_PrivateBuffers,   2,
                        MGLCC_FrontBuffer,      data->glFrontBuffer,
                        MGLCC_BackBuffer,       data->glBackBuffer,
                        MGLCC_Buffers,          2,
                        MGLCC_PixelDepth,       depth,
                        MGLCC_StencilBuffer,    TRUE,
                        MGLCC_VertexBufferSize, 1 << 17,
                        TAG_DONE);

        if (data->glContext) {

            dprintf("MiniGL context %p created for window '%s'\n",
                data->glContext, window->title);

            ((struct GLContextIFace *)data->glContext)->GLViewport(0, 0, width, height);
            mglMakeCurrent(data->glContext);
            mglLockMode(MGL_LOCK_SMART);

            return data->glContext;

        } else {
            dprintf("Failed to create MiniGL context for window '%s'\n", window->title);

            SDL_SetError("Failed to create MiniGL context");

            OS4_GL_FreeBuffers(_this, data);

            return NULL;
        }

    } else {
        OS4_GL_LogLibraryError();
        return NULL;
    }
}

int
OS4_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
    int result = -1;

    if (!window || !context) {
        dprintf("Called window=%p context=%p\n", window, context);
    }

    if (IMiniGL) {

        if (window) {
            SDL_WindowData * data = window->driverdata;

            if (context != data->glContext) {
                dprintf("Context pointer mismatch: %p<>%p\n", context, data->glContext);
                SDL_SetError("Context pointer mismatch");
            } else {
                mglMakeCurrent(context);
            }
        }

        result = 0;

    } else {
        OS4_GL_LogLibraryError();
    }
    
    return result;
}

void
OS4_GL_GetDrawableSize(_THIS, SDL_Window * window, int * w, int * h)
{
    SDL_WindowData * data = window->driverdata;

    int width = 0;
    int height = 0;
    int counter = 0;

    int activeWidth, activeHeight;

    OS4_GetWindowActiveSize(window, &activeWidth, &activeHeight);

    while (counter++ < 100) {
        OS4_GetWindowSize(_this, data->syswin, &width, &height);

        if (width != activeWidth || height != activeHeight) {
            dprintf("Waiting for Intuition %d\n", counter);
            dprintf("w %d (active width %d), h %d (active height %d)\n",
                width, activeWidth, height, activeHeight);
            usleep(1000); // TODO FIXME: this is slow (opens timer.device) currently
        } else {
            break;
        }
    }

    if (w) {
        *w = width;
        //dprintf("w=%d\n", *w);
    }

    if (h) {
        *h = height;
        //dprintf("h=%d\n", *h);
    }
}

int
OS4_GL_SetSwapInterval(_THIS, int interval)
{
    SDL_VideoData *data = _this->driverdata;

    switch (interval) {
        case 0:
        case 1:
            data->vsyncEnabled = interval ? TRUE : FALSE;
            dprintf("VSYNC %d\n", interval);
            return 0;
        default:
            dprintf("Unsupported interval %d\n", interval);
            return -1;
    }
}

int
OS4_GL_GetSwapInterval(_THIS)
{
    //dprintf("Called\n");

    SDL_VideoData *data = _this->driverdata;

    return data->vsyncEnabled ? 1 : 0;
}

int
OS4_GL_SwapWindow(_THIS, SDL_Window * window)
{
    //dprintf("Called\n");

    if (IMiniGL) {

        SDL_WindowData *data = window->driverdata;

        if (data->glContext) {
            SDL_VideoData *videodata = _this->driverdata;

            struct BitMap *temp;
            int w, h;
            GLint buf;

            int32 blitRet;

            mglUnlockDisplay();

            ((struct GLContextIFace *)data->glContext)->MGLWaitGL(); // TODO: still needed?

            OS4_GetWindowSize(_this, data->syswin, &w, &h);

            if (videodata->vsyncEnabled) {
                IGraphics->WaitTOF();
            }

            glGetIntegerv(GL_DRAW_BUFFER, &buf);

            if (buf == GL_BACK || buf == GL_FRONT) {
                struct BitMap *from = (buf == GL_BACK) ? data->glBackBuffer : data->glFrontBuffer;

                BOOL ret = IGraphics->BltBitMapRastPort(from, 0, 0, data->syswin->RPort,
                    data->syswin->BorderLeft, data->syswin->BorderTop, w, h, 0xC0);

                if (!ret) {
                    dprintf("BltBitMapRastPort() failed\n");
                }
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

                ((struct GLContextIFace *)data->glContext)->MGLUpdateContextTags(
                                    MGLCC_FrontBuffer,data->glFrontBuffer,
                                    MGLCC_BackBuffer, data->glBackBuffer,
                                    TAG_DONE);
                return 0;
            } else {
                dprintf("BltBitMapTags() returned %d\n", blitRet);
                return -1;
            }
        } else {
            dprintf("No MiniGL context\n");
        }
    } else {
        OS4_GL_LogLibraryError();
    }

    return -1;
}

void
OS4_GL_DeleteContext(_THIS, SDL_GLContext context)
{
    dprintf("Called with context=%p\n", context);

    if (IMiniGL) {

        if (context) {
            SDL_Window *sdlwin;
            Uint32 deletions = 0;

            for (sdlwin = _this->windows; sdlwin; sdlwin = sdlwin->next) {

                SDL_WindowData *data = sdlwin->driverdata;

                if (data->glContext == context) {
                    struct GLContextIFace *IGL = context;

                    dprintf("Found MiniGL context, clearing window binding\n");

                    IGL->DeleteContext();

                    data->glContext = NULL;
                    deletions++;
                }
            }

            if (deletions == 0) {
                dprintf("MiniGL context doesn't seem to have window binding\n");
            }
        } else {
            dprintf("No context to delete\n");
        }
    } else {
        OS4_GL_LogLibraryError();
    }
}

SDL_bool
OS4_GL_ResizeContext(_THIS, SDL_Window * window)
{
    if (IMiniGL) {
        SDL_WindowData *data = window->driverdata;

        if (data) {
            int width, height;

            uint32 depth = IGraphics->GetBitMapAttr(data->syswin->RPort->BitMap, BMA_BITSPERPIXEL);

            OS4_GetWindowActiveSize(window, &width, &height);

            if (OS4_GL_AllocateBuffers(_this, width, height, depth, data)) {

                dprintf("Resizing MiniGL context to %d*%d\n", width, height);

                ((struct GLContextIFace *)data->glContext)->MGLUpdateContextTags(
                                MGLCC_FrontBuffer, data->glFrontBuffer,
                                MGLCC_BackBuffer, data->glBackBuffer,
                                TAG_DONE);

                ((struct GLContextIFace *)data->glContext)->GLViewport(0, 0, width, height);

                return SDL_TRUE;

            } else {
                dprintf("Failed to re-allocate MiniGL buffers\n");
                //SDL_Quit();
            }
        } else {
            dprintf("Window data NULL\n");
        }
    } else {
        OS4_GL_LogLibraryError();
    }

    return SDL_FALSE;
}

void
OS4_GL_UpdateWindowPointer(_THIS, SDL_Window * window)
{
    // Nothing to do for MiniGL
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
