/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

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

/*
 * GL support for AmigaOS 4.0 using MiniGL
 *
 * (c) 2005      Thomas Frieden
 * (c) 2005-2006 Richard Drummond
 * (c) 2007-2015 Various developers
 */

#if SDL_VIDEO_OPENGL

#include "SDL_os4video.h"
#include "SDL_os4utils.h"
#include "SDL_os4blit.h"
#include "SDL_opengl.h"

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/Picasso96API.h>
#include <graphics/blitattr.h>

#include <GL/gl.h>
#include <mgl/gl.h>

//#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

extern struct P96IFace *SDL_IP96;

static struct MiniGLIFace *IMiniGL = 0;
static struct Library *MiniGLBase = 0;

extern struct IntuitionIFace *SDL_IIntuition;
extern struct GraphicsIFace  *SDL_IGraphics;
extern struct P96IFace       *SDL_IP96;

/* The client program needs access to this context pointer
 * to be able to make GL calls. This presents no problems when
 * it is statically linked against libSDL, but when linked
 * against a shared library version this will require some
 * trickery.
 */
struct GLContextIFace *mini_CurrentContext = 0;

/*
 * Open MiniGL and initialize GL context
 */
int os4video_GL_Init(_THIS)
{
	struct SDL_PrivateVideoData *hidden = (struct SDL_PrivateVideoData *)_this->driverdata;
    int w,h;

	if( hidden->dontdeletecontext ) return 0;

//	printf("Creating context for window %p\n", hidden->win);

	dprintf("Initializing OpenGL.. ");
	MiniGLBase = IExec->OpenLibrary("minigl.library", 0);
	if (!MiniGLBase)
	{
		SDL_SetError("Failed to open minigl.library");
		hidden->OpenGL = FALSE;
		return -1;
	}

	IMiniGL = (struct MiniGLIFace *)IExec->GetInterface(MiniGLBase, "main", 1, NULL);
	if (!IMiniGL)
	{
		SDL_SetError("Failed to obtain minigl.library interface");
		hidden->OpenGL = FALSE;
		IExec->CloseLibrary(MiniGLBase);

		return -1;
	}

	SDL_IIntuition->GetWindowAttrs(hidden->win,WA_InnerWidth,&w,WA_InnerHeight,&h,TAG_DONE);

	if(!(hidden->m_frontBuffer = SDL_IP96->p96AllocBitMap(w,h,16,BMF_MINPLANES | BMF_DISPLAYABLE,hidden->win->RPort->BitMap,0)))
	{
		SDL_SetError("Failed to allocate a Bitmap for the front buffer");
		return -1;
	}

	if(!(hidden->m_backBuffer = SDL_IP96->p96AllocBitMap(w,h,16,BMF_MINPLANES | BMF_DISPLAYABLE,hidden->win->RPort->BitMap,0)))
	{
		SDL_SetError("Failed to allocate a Bitmap for the back buffer");
		SDL_IP96->p96FreeBitMap(hidden->m_frontBuffer);
		return -1;
	}

	hidden->IGL = IMiniGL->CreateContextTags(
                                     MGLCC_PrivateBuffers, 	2,
                                     MGLCC_FrontBuffer,		hidden->m_frontBuffer,
                                     MGLCC_BackBuffer,		hidden->m_backBuffer,
                                     MGLCC_Buffers,  		2,             /* double buffered */
                                     MGLCC_PixelDepth,      16,  // fixed at 16 for now 32 causes issues on SAM440ep with onboard graphics , make user setable later
                                     MGLCC_StencilBuffer,   TRUE,
                                     MGLCC_VertexBufferSize,1 << 17,
									 TAG_DONE);

	if (hidden->IGL)
	{
		_this->gl_config.driver_loaded = 1;

        hidden->IGL->GLViewport(0,0,w,h);

		mglMakeCurrent(hidden->IGL);
		mglLockMode(MGL_LOCK_SMART);
		hidden->OpenGL = TRUE;

		return 0;
	}
	else
	{
		_this->gl_config.driver_loaded = 0;
		SDL_SetError("Failed to create MiniGL context");
	}

	return -1;
}

void os4video_GL_Term(_THIS)
{
	struct SDL_PrivateVideoData *hidden = (struct SDL_PrivateVideoData *)_this->driverdata;

	if( hidden->dontdeletecontext ) return;

	if (hidden->OpenGL)
	{
        if(hidden->m_frontBuffer)
        {
            SDL_IP96->p96FreeBitMap(hidden->m_frontBuffer);
            hidden->m_frontBuffer = NULL;
        }
        if(hidden->m_backBuffer)
        {
            SDL_IP96->p96FreeBitMap(hidden->m_backBuffer);
            hidden->m_backBuffer = NULL;
        }

		hidden->IGL->DeleteContext();
		IExec->DropInterface((struct Interface *)IMiniGL);
		IExec->CloseLibrary(MiniGLBase);

		_this->gl_config.driver_loaded = 0;

		hidden->OpenGL = FALSE;
	}
}

int	os4video_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value)
{
	struct SDL_PrivateVideoData *hidden = (struct SDL_PrivateVideoData *)_this->driverdata;
	struct BitMap *bm = hidden->screenHWData.bm;
	SDL_PixelFormat pf;
	uint32 rgbFormat;

	if (!bm)
		return -1;

	rgbFormat = SDL_IP96->p96GetBitMapAttr(bm, P96BMA_RGBFORMAT);

	if (!os4video_PPFtoPF(&pf, rgbFormat))
		return -1;

	switch (attrib)
	{
		case SDL_GL_RED_SIZE:
			*value = 8-pf.Rloss;
			return 0;

		case SDL_GL_GREEN_SIZE:
			*value = 8-pf.Gloss;
			return 0;

		case SDL_GL_BLUE_SIZE:
			*value = 8-pf.Bloss;
			return 0;

		case SDL_GL_ALPHA_SIZE:
			if (rgbFormat == RGBFB_A8R8G8B8 || rgbFormat == RGBFB_A8B8G8R8
			 || rgbFormat ==  RGBFB_R8G8B8A8 || rgbFormat == RGBFB_B8G8R8A8)
				*value = 8;
			else
				*value = 0;
			return 0;

		case SDL_GL_DOUBLEBUFFER:
			*value = _this->gl_config.double_buffer;
			return 0;

		case SDL_GL_BUFFER_SIZE:
			*value = pf.BitsPerPixel;
			return 0;

		case SDL_GL_DEPTH_SIZE:
			*value = _this->gl_config.depth_size;
			return 0;

		case SDL_GL_STENCIL_SIZE:
			*value = _this->gl_config.stencil_size;
			return 0;

		case SDL_GL_STEREO:
		case SDL_GL_MULTISAMPLEBUFFERS:
		case SDL_GL_MULTISAMPLESAMPLES:
		case SDL_GL_ACCUM_RED_SIZE:
		case SDL_GL_ACCUM_GREEN_SIZE:
		case SDL_GL_ACCUM_BLUE_SIZE:
		case SDL_GL_ACCUM_ALPHA_SIZE:
			*value = 0;
			return -1;
	}

	return -1;
}

int	os4video_GL_MakeCurrent(_THIS)
{
	return 0;
}

void os4video_GL_SwapBuffers(_THIS)
{
	struct SDL_PrivateVideoData *hidden = (struct SDL_PrivateVideoData *)_this->driverdata;
	int w,h;
    GLint buf;
	struct BitMap *temp;

	struct SDL_GLDriverData *video = _this->gl_data;

	if (video)
	{
		mglUnlockDisplay();

		SDL_IIntuition->GetWindowAttrs(hidden->win, WA_InnerWidth, &w, WA_InnerHeight, &h, TAG_DONE);

		hidden->IGL->MGLWaitGL(); /* besure all has finished before we start blitting (testing to find lockup cause */

		//_this->FlipHWSurface(_this, video);

        glGetIntegerv(GL_DRAW_BUFFER,&buf);
        if(buf == GL_BACK)
        {
            SDL_IGraphics->BltBitMapRastPort(hidden->m_backBuffer,0,0,hidden->win->RPort,hidden->win->BorderLeft,hidden->win->BorderTop,w,h,0xC0);
        }
        else if(buf == GL_FRONT)
        {
            SDL_IGraphics->BltBitMapRastPort(hidden->m_frontBuffer,0,0,hidden->win->RPort,hidden->win->BorderLeft,hidden->win->BorderTop,w,h,0xC0);
        }

        /* copy back into front */
        SDL_IGraphics->BltBitMapTags(BLITA_Source,	hidden->m_backBuffer,
								 	 BLITA_SrcType,	BLITT_BITMAP,
 								 	 BLITA_SrcX,	0,
 								 	 BLITA_SrcY,	0,
								 	 BLITA_Dest,	hidden->m_frontBuffer,
								 	 BLITA_DestType,BLITT_BITMAP,
								 	 BLITA_DestX,	0,
								 	 BLITA_DestY,	0,
								 	 BLITA_Width,	w,
								 	 BLITA_Height,	h,
								 	 BLITA_Minterm,	0xC0,
								 	 TAG_DONE);

        temp = hidden->m_frontBuffer;
        hidden->m_frontBuffer = hidden->m_backBuffer;
        hidden->m_backBuffer = temp;

		hidden->IGL->MGLUpdateContextTags(
							MGLCC_FrontBuffer,hidden->m_frontBuffer,
							MGLCC_BackBuffer, hidden->m_backBuffer,
							TAG_DONE);

		//mglSetBitmap(hidden->screenHWData.bm);
	}
}

void *os4video_GL_GetProcAddress(_THIS, const char *proc) {
	void *func = NULL;

	if ( !_this->gl_config.driver_loaded )
	{
		if (os4video_GL_Init(_this) != 0)
		{
			return NULL;
		}
	}

	func = (void *)AmiGetGLProc(proc);
	return func;
}

int os4video_GL_LoadLibrary(_THIS, const char *path) {
	/* Library is always open */
	_this->gl_config.driver_loaded = 1;

	return 0;
}

/*
void glPopClientAttrib(void)
{
}

void glPushClientAttrib(void)
{
}
*/
#endif
