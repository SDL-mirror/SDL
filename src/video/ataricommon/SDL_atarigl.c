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

/* Atari OSMesa.ldg implementation of SDL OpenGL support */

/*--- Includes ---*/

#ifdef HAVE_OPENGL
#include <GL/osmesa.h>
#endif

#include "SDL_video.h"
#include "SDL_error.h"
#include "SDL_endian.h"
#include "SDL_atarigl_c.h"

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void ConvertNull(SDL_Surface *surface);
static void Convert565To555be(SDL_Surface *surface);
static void Convert565To555le(SDL_Surface *surface);
static void Convert565le(SDL_Surface *surface);
static void ConvertBGRAToABGR(SDL_Surface *surface);

/*--- Public functions ---*/

int SDL_AtariGL_Init(_THIS, SDL_Surface *current)
{
#ifdef HAVE_OPENGL
	GLenum osmesa_format;
	SDL_PixelFormat *pixel_format;
	Uint32	redmask;

	SDL_AtariGL_Quit(this);	/* Destroy previous context if exist */

	/* Init OpenGL context using OSMesa */
	gl_convert = ConvertNull;

	pixel_format = current->format;
	redmask = pixel_format->Rmask;
	switch (pixel_format->BitsPerPixel) {
		case 15:
			/* 1555, big and little endian, unsupported */
			osmesa_format = OSMESA_RGB_565;
			if (redmask == 31<<10) {
				gl_convert = Convert565To555be;
			} else {
				gl_convert = Convert565To555le;
			}
			break;
		case 16:
			if (redmask == 31<<11) {
				osmesa_format = OSMESA_RGB_565;
			} else {
				/* 565, little endian, unsupported */
				osmesa_format = OSMESA_RGB_565;
				gl_convert = Convert565le;
			}
			break;
		case 24:
			if (redmask == 255<<16) {
				osmesa_format = OSMESA_RGB;
			} else {
				osmesa_format = OSMESA_BGR;
			}
			break;
		case 32:
			if (redmask == 255<<16) {
				osmesa_format = OSMESA_ARGB;
			} else if (redmask == 255<<8) {
				osmesa_format = OSMESA_BGRA;
			} else if (redmask == 255<<24) {
				osmesa_format = OSMESA_RGBA;
			} else {
				/* ABGR format unsupported */
				osmesa_format = OSMESA_BGRA;
				gl_convert = ConvertBGRAToABGR;
			}
			break;
		default:
			osmesa_format = OSMESA_COLOR_INDEX;
			break;
	}

	gl_ctx = OSMesaCreateContextExt( osmesa_format, this->gl_config.depth_size,
		this->gl_config.stencil_size, this->gl_config.accum_red_size +
		this->gl_config.accum_green_size + this->gl_config.accum_blue_size +
		this->gl_config.accum_alpha_size, NULL );

	gl_active = (gl_ctx != NULL);
	return (gl_active);
#else
	return 0;
#endif
}

void SDL_AtariGL_Quit(_THIS)
{
#ifdef HAVE_OPENGL
	/* Shutdown OpenGL context */
	if (gl_ctx) {
		OSMesaDestroyContext(gl_ctx);
		gl_ctx = NULL;
	}
#endif
	gl_active = 0;
}

int SDL_AtariGL_LoadLibrary(_THIS, const char *path)
{
#ifdef HAVE_OPENGL
	/* Library is always opened */
	this->gl_config.driver_loaded = 1;
#endif
	return 0;
}

void *SDL_AtariGL_GetProcAddress(_THIS, const char *proc)
{
	void *func = NULL;
#ifdef HAVE_OPENGL
	if (gl_ctx != NULL) {
		func = OSMesaGetProcAddress(proc);
	}
#endif
	return func;
}

int SDL_AtariGL_GetAttribute(_THIS, SDL_GLattr attrib, int* value)
{
#ifdef HAVE_OPENGL
	GLenum mesa_attrib;
	SDL_Surface *surface;

	if (gl_ctx == NULL) {
		return -1;
	}

	switch(attrib) {
		case SDL_GL_RED_SIZE:
			mesa_attrib = GL_RED_BITS;
			break;
		case SDL_GL_GREEN_SIZE:
			mesa_attrib = GL_GREEN_BITS;
			break;
		case SDL_GL_BLUE_SIZE:
			mesa_attrib = GL_BLUE_BITS;
			break;
		case SDL_GL_ALPHA_SIZE:
			mesa_attrib = GL_ALPHA_BITS;
			break;
		case SDL_GL_DOUBLEBUFFER:
			surface = this->screen;
			*value = ((surface->flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF);
			return 0;
		case SDL_GL_DEPTH_SIZE:
			mesa_attrib = GL_DEPTH_BITS;
			break;
		case SDL_GL_STENCIL_SIZE:
			mesa_attrib = GL_STENCIL_BITS;
			break;
		case SDL_GL_ACCUM_RED_SIZE:
			mesa_attrib = GL_ACCUM_RED_BITS;
			break;
		case SDL_GL_ACCUM_GREEN_SIZE:
			mesa_attrib = GL_ACCUM_GREEN_BITS;
			break;
		case SDL_GL_ACCUM_BLUE_SIZE:
			mesa_attrib = GL_ACCUM_BLUE_BITS;
			break;
		case SDL_GL_ACCUM_ALPHA_SIZE:
			mesa_attrib = GL_ACCUM_ALPHA_BITS;
			break;
		default :
			return -1;
	}

	glGetIntegerv(mesa_attrib, value);
	return 0;
#else
	return -1;
#endif
}

int SDL_AtariGL_MakeCurrent(_THIS)
{
#ifdef HAVE_OPENGL
	SDL_Surface *surface;
	GLenum type;

	if (gl_ctx == NULL) {
		SDL_SetError("Invalid OpenGL context");
		return -1;
	}

	surface = this->screen;
	
	if ((surface->format->BitsPerPixel == 15) || (surface->format->BitsPerPixel == 16)) {
		type = GL_UNSIGNED_SHORT_5_6_5;
	} else {
		type = GL_UNSIGNED_BYTE;
	}

	if (!OSMesaMakeCurrent(gl_ctx, surface->pixels, type, surface->w, surface->h)) {
		SDL_SetError("Can not make OpenGL context current");
		return -1;
	}

	/* OSMesa draws upside down */
	OSMesaPixelStore(OSMESA_Y_UP, 0);

	return 0;
#else
	return -1;
#endif
}

void SDL_AtariGL_SwapBuffers(_THIS)
{
#ifdef HAVE_OPENGL
	if (gl_ctx == NULL) {
		return;
	}

	gl_convert(this->screen);
#endif
}

/*--- Private functions ---*/

static void ConvertNull(SDL_Surface *surface)
{
}

static void Convert565To555be(SDL_Surface *surface)
{
	int x,y, pitch;
	unsigned short *line, *pixel;

	line = surface->pixels;
	pitch = surface->pitch >> 1;
	for (y=0; y<surface->h; y++) {
		pixel = line;
		for (x=0; x<surface->w; x++) {
			unsigned short color = *pixel;

			*pixel++ = (color & 0x1f)|((color>>1) & 0xffe0);
		}

		line += pitch;
	}
}

static void Convert565To555le(SDL_Surface *surface)
{
	int x,y, pitch;
	unsigned short *line, *pixel;

	line = surface->pixels;
	pitch = surface->pitch >>1;
	for (y=0; y<surface->h; y++) {
		pixel = line;
		for (x=0; x<surface->w; x++) {
			unsigned short color = *pixel;

			color = (color & 0x1f)|((color>>1) & 0xffe0);
			*pixel++ = SDL_Swap16(color);
		}

		line += pitch;
	}
}

static void Convert565le(SDL_Surface *surface)
{
	int x,y, pitch;
	unsigned short *line, *pixel;

	line = surface->pixels;
	pitch = surface->pitch >>1;
	for (y=0; y<surface->h; y++) {
		pixel = line;
		for (x=0; x<surface->w; x++) {
			unsigned short color = *pixel;

			*pixel++ = SDL_Swap16(color);
		}

		line += pitch;
	}
}

static void ConvertBGRAToABGR(SDL_Surface *surface)
{
	int x,y, pitch;
	unsigned long *line, *pixel;

	line = surface->pixels;
	pitch = surface->pitch >>2;
	for (y=0; y<surface->h; y++) {
		pixel = line;
		for (x=0; x<surface->w; x++) {
			unsigned long color = *pixel;

			*pixel++ = (color<<24)|(color>>8);
		}

		line += pitch;
	}
}
