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
 * glSDL "SDL-over-OpenGL" video driver implemented by
 * David Olofson <david@olofson.net> and
 * Stephane Marchesin <stephane.marchesin@wanadoo.fr>
 */
#include <math.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"

#include "SDL_glsdl.h"

#undef	DEBUG_GLSDL
#undef	DEBUG_GLSDL_CHOP
#define	FAKE_MAXTEXSIZE	256
#undef GLSDL_GRAPHICAL_DEBUG

/* Initialization/Query functions */

/* Hardware surface functions */
static int glSDL_SetColors(_THIS, int firstcolor, int ncolors,
                           SDL_Color * colors);
static int glSDL_AllocHWSurface(_THIS, SDL_Surface * surface);
static int glSDL_LockHWSurface(_THIS, SDL_Surface * surface);
static int glSDL_FlipHWSurface(_THIS, SDL_Surface * surface);
static void glSDL_UnlockHWSurface(_THIS, SDL_Surface * surface);
static void glSDL_FreeHWSurface(_THIS, SDL_Surface * surface);
static int glSDL_FillHWRect(_THIS, SDL_Surface * dst, SDL_Rect * rect,
                            Uint32 color);
static int glSDL_CheckHWBlit(_THIS, SDL_Surface * src, SDL_Surface * dst);
static int glSDL_SetHWColorKey(_THIS, SDL_Surface * surface, Uint32 key);
static int glSDL_SetHWAlpha(_THIS, SDL_Surface * surface, Uint8 alpha);
static int glSDL_VideoInit(_THIS, SDL_PixelFormat * vformat);
static SDL_Rect **glSDL_ListModes(_THIS, SDL_PixelFormat * format,
                                  Uint32 flags);
static void glSDL_VideoQuit(_THIS);
static void glSDL_UpdateRects(_THIS, int numrects, SDL_Rect * rects);
static SDL_Surface *glSDL_SetVideoMode(_THIS, SDL_Surface * current,
                                       int width, int height, int bpp,
                                       Uint32 flags);

#define	IS_GLSDL_SURFACE(s)	((s) && glSDL_GetTexInfo(s))

#define	LOGIC_W(s)	( IS_GLSDL_SURFACE(this,s) ? TEXINFO(s)->lw : (s)->w )
#define	LOGIC_H(s)	( IS_GLSDL_SURFACE(this,s) ? TEXINFO(s)->lh : (s)->h )

#define	GLSDL_NOTEX	(~0)

/*
 * Special version for glSDL, which ignores the fake SDL_HWSURFACE
 * flags, so we don't have SDL calling us back whenever we want to
 * do some internal blitting...
 */
static void
glSDL_SoftBlit(SDL_Surface * src, SDL_Rect * srcrect,
               SDL_Surface * dst, SDL_Rect * dstrect)
{
    SDL_BlitInfo info;

    if (srcrect)
        if (!srcrect->w || !srcrect->h)
            return;

    /* Check to make sure the blit mapping is valid */
    if ((src->map->dst != dst) ||
        (src->map->dst->format_version != src->map->format_version))
        if (SDL_MapSurface(src, dst) < 0)
            return;

    /* Set up the blit information */
    if (srcrect) {
        info.s_pixels = (Uint8 *) src->pixels +
            (Uint16) srcrect->y * src->pitch +
            (Uint16) srcrect->x * src->format->BytesPerPixel;
        info.s_width = srcrect->w;
        info.s_height = srcrect->h;
    } else {
        info.s_pixels = (Uint8 *) src->pixels;
        info.s_width = src->w;
        info.s_height = src->h;
    }
    info.s_skip = src->pitch - info.s_width * src->format->BytesPerPixel;
    if (dstrect) {
        info.d_pixels = (Uint8 *) dst->pixels +
            (Uint16) dstrect->y * dst->pitch +
            (Uint16) dstrect->x * dst->format->BytesPerPixel;
        /*
         * NOTE: SDL_SoftBlit() uses the 'dstrect' for this!
         *       This version is more like SDL_BlitSurface().
         */
        info.d_width = srcrect->w;
        info.d_height = srcrect->h;
    } else {
        info.d_pixels = (Uint8 *) dst->pixels;
        info.d_width = dst->w;
        info.d_height = dst->h;
    }
    info.d_skip = dst->pitch - info.d_width * dst->format->BytesPerPixel;
    info.aux_data = src->map->sw_data->aux_data;
    info.src = src->format;
    info.table = src->map->table;
    info.dst = dst->format;

    src->map->sw_data->blit(&info);
}


/* 
 * Another special version. Doesn't lock/unlock, and doesn't mess
 * with flags and stuff. It just converts the surface, period.
 * Does not convert into palletized formats.
 */
static SDL_Surface *
glSDL_ConvertSurface(SDL_Surface * surface,
                     SDL_PixelFormat * format, Uint32 flags)
{
    SDL_Surface *convert;
    Uint32 colorkey = 0;
    Uint8 alpha = 0;
    Uint32 surface_flags;
    SDL_Rect bounds;

    /* Create a new surface with the desired format */
    convert = SDL_CreateRGBSurface(flags,
                                   surface->w, surface->h,
                                   format->BitsPerPixel, format->Rmask,
                                   format->Gmask, format->Bmask,
                                   format->Amask);
    if (convert == NULL) {
        return (NULL);
    }

    /* Save the original surface color key and alpha */
    surface_flags = surface->flags;
    if ((surface_flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY) {
        /* Convert colourkeyed surfaces to RGBA if requested */
        if ((flags & SDL_SRCCOLORKEY) != SDL_SRCCOLORKEY && format->Amask) {
            surface_flags &= ~SDL_SRCCOLORKEY;
        } else {
            colorkey = surface->format->colorkey;
            SDL_SetColorKey(surface, 0, 0);
        }
    }
    if ((surface_flags & SDL_SRCALPHA) == SDL_SRCALPHA) {
        /* Copy over the alpha channel to RGBA if requested */
        if (format->Amask) {
            surface->flags &= ~SDL_SRCALPHA;
        } else {
            alpha = surface->format->alpha;
            SDL_SetAlpha(surface, 0, 0);
        }
    }

    /* Copy over the image data */
    bounds.x = 0;
    bounds.y = 0;
    bounds.w = surface->w;
    bounds.h = surface->h;
    glSDL_SoftBlit(surface, &bounds, convert, &bounds);

    /* Clean up the original surface, and update converted surface */
    if (convert != NULL) {
        SDL_SetClipRect(convert, &surface->clip_rect);
    }
    if ((surface_flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY) {
        Uint32 cflags = surface_flags & (SDL_SRCCOLORKEY | SDL_RLEACCELOK);
        if (convert != NULL) {
            Uint8 keyR, keyG, keyB;

            SDL_GetRGB(colorkey, surface->format, &keyR, &keyG, &keyB);
            SDL_SetColorKey(convert, cflags | (flags & SDL_RLEACCELOK),
                            SDL_MapRGB(convert->format, keyR, keyG, keyB));
        }
        SDL_SetColorKey(surface, cflags, colorkey);
    }
    if ((surface_flags & SDL_SRCALPHA) == SDL_SRCALPHA) {
        Uint32 aflags = surface_flags & (SDL_SRCALPHA | SDL_RLEACCELOK);
        if (convert != NULL) {
            SDL_SetAlpha(convert, aflags | (flags & SDL_RLEACCELOK), alpha);
        }
        if (format->Amask) {
            surface->flags |= SDL_SRCALPHA;
        } else {
            SDL_SetAlpha(surface, aflags, alpha);
        }
    }

    /* We're ready to go! */
    return (convert);
}


/*----------------------------------------------------------
  Some OpenGL function wrappers
  ----------------------------------------------------------*/

static struct
{
    int do_blend;
    int do_texture;
    GLuint texture;
    GLenum sfactor, dfactor;
} glstate;

static void
glSDL_reset(void)
{
    glstate.do_blend = -1;
    glstate.do_blend = -1;
    glstate.texture = GLSDL_NOTEX;
    glstate.sfactor = 0xffffffff;
    glstate.dfactor = 0xffffffff;
}

static __inline__ void
glSDL_do_blend(_THIS, int on)
{
    if (glstate.do_blend == on)
        return;

    if (on)
        this->glEnable(GL_BLEND);
    else
        this->glDisable(GL_BLEND);
    glstate.do_blend = on;
}

static __inline__ void
glSDL_do_texture(_THIS, int on)
{
    if (glstate.do_texture == on)
        return;

    if (on)
        this->glEnable(GL_TEXTURE_2D);
    else
        this->glDisable(GL_TEXTURE_2D);
    glstate.do_texture = on;
}

static __inline__ void
glSDL_blendfunc(_THIS, GLenum sfactor, GLenum dfactor)
{
    if ((sfactor == glstate.sfactor) && (dfactor == glstate.dfactor))
        return;

    this->glBlendFunc(sfactor, dfactor);

    glstate.sfactor = sfactor;
    glstate.dfactor = dfactor;
}

static __inline__ void
glSDL_texture(_THIS, GLuint tx)
{
    if (tx == glstate.texture)
        return;

    this->glBindTexture(GL_TEXTURE_2D, tx);
    glstate.texture = tx;
}




/*----------------------------------------------------------
  glSDL specific data types
  ----------------------------------------------------------*/

typedef enum
{
    GLSDL_TM_SINGLE,
    GLSDL_TM_HORIZONTAL,
    GLSDL_TM_VERTICAL,
    GLSDL_TM_HUGE
} GLSDL_TileModes;


typedef struct private_hwdata
{
    /* Size of surface in logic screen pixels */
    int lw, lh;

    int textures;
    GLuint *texture;
    int texsize;                /* width/height of OpenGL texture */
    GLSDL_TileModes tilemode;
    int tilew, tileh;           /* At least one must equal texsize! */
    int tilespertex;
    SDL_Rect virt;              /* Total size of assembled surface */

    /* Area of surface to upload when/after unlocking */
    SDL_Rect invalid_area;

    int temporary;              /* Throw away after one use. */

    SDL_Surface *next;          /* The next Surface in our linked list of hardware surfaces ; == NULL if first surface */
    SDL_Surface *prev;          /* The prev Surface in our linked list of hardware surfaces ; == NULL if last surface */
} private_hwdata;

/* some function prototypes */
static void glSDL_Invalidate(SDL_Surface * surface, SDL_Rect * area);
static void glSDL_SetLogicSize(_THIS, SDL_Surface * surface, int w, int h);
static private_hwdata *glSDL_UploadSurface(_THIS, SDL_Surface * surface);
static private_hwdata *glSDL_GetTexInfo(SDL_Surface * surface);
static void glSDL_init_formats(_THIS);
static private_hwdata *glSDL_AddTexInfo(_THIS, SDL_Surface * surface);
static void glSDL_RemoveTexInfo(_THIS, SDL_Surface * surface);
static void glSDL_UnloadTexture(_THIS, private_hwdata * txi);
static int glSDL_BlitGL(_THIS, SDL_Surface * src,
                        SDL_Rect * srcrect, SDL_Rect * dstrect);

/* some variables */
static GLint maxtexsize = -1;
static SDL_PixelFormat *RGBfmt = NULL;
static SDL_PixelFormat *RGBAfmt = NULL;
static void *mirrorbuf = NULL;
/* the raw 888 opengl surface, hidden from the application */
SDL_Surface *OpenGL_Surface;

/* pointer to the beggining of the list used for memory allocation */
SDL_Surface *first = NULL;

#ifdef DEBUG_GLSDL
static __inline__ int
GLERET(const char *txt)
{
    fprintf(stderr, "glSDL ERROR: '%s'\n", txt);
    return -1;
}
static __inline__ void
GLERR(const char *txt)
{
    fprintf(stderr, "glSDL ERROR: '%s'\n", txt);
}
#else
#define	GLERET(x)	(-1)
#define	GLERR(x)
#endif

static SDL_VideoDevice underlying_device;
static int old_screen_flags;

/* 
 * List of video drivers known to support OpenGL 
 * The purpose of this is to make glSDL "portable" across
 * all video backends that support OpenGL
 */
static VideoBootStrap *opengl_bootstrap =
#if SDL_VIDEO_DRIVER_QUARTZ
    &QZ_bootstrap;
#elif SDL_VIDEO_DRIVER_X11
    &X11_bootstrap;
#elif SDL_VIDEO_DRIVER_WINDIB
    &WINDIB_bootstrap;
#elif SDL_VIDEO_DRIVER_BWINDOW
    &BWINDOW_bootstrap;
#elif SDL_VIDEO_DRIVER_TOOLBOX
    &TOOLBOX_bootstrap;
#elif SDL_VIDEO_DRIVER_CYBERGRAPHICS
    &CGX_bootstrap;
#elif SDL_VIDEO_DRIVER_PHOTON
    &ph_bootstrap;
#elif SDL_VIDEO_DRIVER_DC
    &DC_bootstrap;
#else
    NULL;
#endif

static int
glSDL_Available(void)
{
#ifdef DEBUG_GLSDL
    fprintf(stderr, "available\n");
#endif
    if (opengl_bootstrap == NULL)
        return 0;
    return (opengl_bootstrap->available());
}

static void
glSDL_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

/* Create a glSDL device */
static SDL_VideoDevice *
glSDL_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;
#ifdef DEBUG_GLSDL
    fprintf(stderr, "entering createdevice\n");
#endif

    /* Create the device with the underlying driver */
    device = opengl_bootstrap->create(devindex);

    /* Save the video device contents for future use */
    SDL_memcpy(&underlying_device, device, sizeof(SDL_VideoDevice));

    /* Hook glSDL on the video device */
    device->VideoInit = glSDL_VideoInit;
    device->ListModes = glSDL_ListModes;
    device->VideoQuit = glSDL_VideoQuit;
    device->UpdateRects = glSDL_UpdateRects;
    device->FillHWRect = glSDL_FillHWRect;
    device->SetHWColorKey = glSDL_SetHWColorKey;
    device->SetHWAlpha = glSDL_SetHWAlpha;
    device->AllocHWSurface = glSDL_AllocHWSurface;
    device->LockHWSurface = glSDL_LockHWSurface;
    device->UnlockHWSurface = glSDL_UnlockHWSurface;
    device->FlipHWSurface = glSDL_FlipHWSurface;
    device->FreeHWSurface = glSDL_FreeHWSurface;
    device->CheckHWBlit = glSDL_CheckHWBlit;
    device->SetColors = glSDL_SetColors;
    device->SetVideoMode = glSDL_SetVideoMode;
    device->info.hw_available = 1;
    device->info.blit_hw = 1;
    device->info.blit_hw_CC = 1;
    device->info.blit_hw_A = 1;
    device->info.blit_sw = 1;
    device->info.blit_sw_CC = 1;
    device->info.blit_sw_A = 1;
    device->info.blit_fill = 1;

    /* These functions are not supported by glSDL, so we NULLify them */
    device->SetGamma = NULL;
    device->GetGamma = NULL;
    device->SetGammaRamp = NULL;
    device->GetGammaRamp = NULL;
    device->ToggleFullScreen = NULL;

    device->free = glSDL_DeleteDevice;

#ifdef DEBUG_GLSDL
    fprintf(stderr, "leaving createdevice\n");
#endif

    return device;
}

/* Our bootstraping structure */
VideoBootStrap glSDL_bootstrap = {
    "glSDL", "glSDL - SDL over OpenGL",
    glSDL_Available, glSDL_CreateDevice
};

static int
glSDL_VideoInit(_THIS, SDL_PixelFormat * vformat)
{
    int r;
    printf("glSDL videoinit\n");
#ifdef DEBUG_GLSDL
    fprintf(stderr, "videoinit\n");
#endif
    r = underlying_device.VideoInit(this, vformat);
    this->info.hw_available = 1;
    this->info.blit_hw = 1;
    this->info.blit_hw_CC = 1;
    this->info.blit_hw_A = 1;
    this->info.blit_sw = 1;
    this->info.blit_sw_CC = 1;
    this->info.blit_sw_A = 1;
    this->info.blit_fill = 1;

    return r;
}

SDL_Rect **
glSDL_ListModes(_THIS, SDL_PixelFormat * format, Uint32 flags)
{
    return ((SDL_Rect **) - 1);
}

static void
glSDL_VideoQuit(_THIS)
{
    SDL_Surface *scr;

    /* free all hwdata structures */
    while (first != NULL)
        glSDL_RemoveTexInfo(this, first);

    SDL_free(mirrorbuf);
    mirrorbuf = NULL;

    SDL_FreeFormat(RGBfmt);
    SDL_FreeFormat(RGBAfmt);
    RGBfmt = RGBAfmt = NULL;

    SDL_FreeFormat(this->displayformatalphapixel);
    this->displayformatalphapixel = NULL;

    SDL_FreeSurface(OpenGL_Surface);
    OpenGL_Surface = NULL;

    /* restore the flags to gracefully exit from fullscreen */
    this->screen->flags = old_screen_flags;

    /* keep the screen */
    scr = this->screen;

    /* we cleaned up our stuff, now restore the underlying video driver */
    SDL_memcpy(this, &underlying_device, sizeof(SDL_VideoDevice));

    this->screen = scr;

    /* call the underlying video driver's VideoQuit function */
    this->VideoQuit(this);
}

static SDL_Surface *
glSDL_SetVideoMode(_THIS, SDL_Surface * current, int width, int height,
                   int bpp, Uint32 flags)
{
    SDL_Surface *hooked_screen;
    int i;
    int flag_doublebuf = 0;

    if (opengl_bootstrap == NULL) {
        GLERR("No bootstrap for glSDL compiled in !\n");
        return NULL;
    }

    /* we don't have OpenGL */
    if ((flags & SDL_INTERNALOPENGL) == SDL_INTERNALOPENGL) {
        GLERR("OpenGL video modes are not supported by glSDL !\n");
        return (NULL);
    }

    /* 
     * Adjust the flags
     */
    flags &= ~SDL_HWPALETTE;
    flags |= SDL_INTERNALOPENGL;

    /* remember whether the user requested DOUBLEBUF */

    if (flags & SDL_DOUBLEBUF) {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        flag_doublebuf = 1;
    } else {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
        flag_doublebuf = 0;
    }

    hooked_screen =
        underlying_device.SetVideoMode(this, current, width, height, 0,
                                       flags);

    if (!hooked_screen) {
        GLERR("Unable to open an OpenGL window !\n");
        return (NULL);
    }

    /* save the screen flags for restore time */
    old_screen_flags = hooked_screen->flags;

#ifdef DEBUG_GLSDL
    fprintf(stderr, "got %d bpp\n", bpp);
#endif

    /* setup the public surface format
     * glSDL always returns the bpp its asked
     */
    switch (bpp) {
    case 32:
        this->is_32bit = 1;
        this->screen = SDL_CreateRGBSurface(flags, width, height, bpp,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                            0x00FF0000,
                                            0x0000FF00, 0x000000FF, 0x00000000
#else
                                            0x0000FF00,
                                            0x00FF0000, 0xFF000000, 0x00000000
#endif
            );
        break;
    case 24:
        this->is_32bit = 0;
        this->screen = SDL_CreateRGBSurface(flags, width, height, bpp,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                            0x00FF0000,
                                            0x0000FF00, 0x000000FF, 0x00000000
#else
                                            0x0000FF00,
                                            0x00FF0000, 0xFF000000, 0x00000000
#endif
            );
        break;
    case 16:
        this->is_32bit = 0;
        this->screen = SDL_CreateRGBSurface(flags, width, height, bpp,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                            0x0000F800,
                                            0x000007E0, 0x0000001F, 0x00000000
#else
                                            0x0000001F,
                                            0x000007E0, 0x0000F800, 0x00000000
#endif
            );
        break;
    case 15:
        this->is_32bit = 0;
        this->screen = SDL_CreateRGBSurface(flags, width, height, bpp,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                            0x00007C00,
                                            0x000003E0, 0x0000001F, 0x00000000
#else
                                            0x0000001F,
                                            0x000003E0, 0x00007C00, 0x00000000
#endif
            );
        break;
    case 8:
    default:
        this->is_32bit = 0;
        this->screen =
            SDL_CreateRGBSurface(flags, width, height, bpp, 0, 0, 0, 0);
        /* give it a default palette if 8 bpp
         * note : SDL already takes care of the palette for 4 bits & 1 bit surfaces 
         */
/*			if (bpp==8)
			{
				this->screen->format->palette->ncolors=255;
				SDL_DitherColors(this->screen->format->palette->colors,bpp);
			}*/
        break;
    }

    /* also, we add SDL_HWSURFACE all the time, and let SDL create a shadow surface accordingly */
    this->screen->flags =
        hooked_screen->flags | SDL_HWSURFACE | SDL_INTERNALOPENGL;
    /* add SDL_DOUBLEBUF if it was requested */
    if (flag_doublebuf)
        this->screen->flags |= SDL_DOUBLEBUF;

    /* Tell SDL the alpha pixel format we'd like to have */
    this->displayformatalphapixel = SDL_AllocFormat(32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                                                    0xFF000000,
                                                    0x00FF0000,
                                                    0x0000FF00, 0x000000FF
#else
                                                    0x000000FF,
                                                    0x0000FF00,
                                                    0x00FF0000, 0xFF000000
#endif
        );

    /* Now create the raw OpenGL surface */
    OpenGL_Surface = SDL_CreateRGBSurface(flags, width, height, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                          0x000000FF,
                                          0x0000FF00, 0x00FF0000, 0x00000000
#else
                                          0xFF000000,
                                          0x00FF0000, 0x0000FF00, 0x00000000
#endif
        );

    /* Here we have to setup OpenGL funcs ourselves */
#ifndef __QNXNTO__
#define SDL_PROC(ret,func,params) \
	do { \
		this->func = SDL_GL_GetProcAddress(#func); \
			if ( ! this->func ) { \
				SDL_SetError("Couldn't load GL function: %s\n", #func); \
					return(NULL); \
			} \
	} while ( 0 );
#else
#define SDL_PROC(ret,func,params) this->func=func;
#endif /* __QNXNTO__ */
#include "../SDL_glfuncs.h"
#undef SDL_PROC

    if (this->GL_MakeCurrent(this) < 0)
        return (NULL);
#define SDL_PROC(ret,func,params) \
	do { \
		this->func = SDL_GL_GetProcAddress(#func); \
			if ( ! this->func ) { \
				SDL_SetError("Couldn't load GL function: %s\n", #func); \
					return(NULL); \
			} \
	} while ( 0 );
#include "../SDL_glfuncs.h"
#undef SDL_PROC


#ifdef	FAKE_MAXTEXSIZE
    maxtexsize = FAKE_MAXTEXSIZE;
#else
    this->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexsize);
#endif
#ifdef DEBUG_GLSDL
    fprintf(stderr, "glSDL: Max texture size: %d\n", maxtexsize);
#endif

    glSDL_init_formats(this);

    if (flag_doublebuf)
        this->glDrawBuffer(GL_BACK);
    else
        this->glDrawBuffer(GL_FRONT);

    this->glDisable(GL_DITHER);

    if (glSDL_AddTexInfo(this, this->screen) < 0) {
        GLERR("HookDevice() failed to add info to screen surface!");
        return NULL;
    }

    glSDL_SetLogicSize(this, this->screen, this->screen->w, this->screen->h);

    glSDL_do_texture(this, 0);
    glSDL_do_blend(this, 0);

    for (i = 0; i < 1 + flag_doublebuf; ++i) {
        this->glBegin(GL_TRIANGLE_FAN);
        this->glColor3ub(0, 0, 0);
        this->glVertex2i(0, 0);
        this->glVertex2i(this->screen->w, 0);
        this->glVertex2i(this->screen->w, this->screen->h);
        this->glVertex2i(0, this->screen->h);
        this->glEnd();
        if (!i)
            this->GL_SwapBuffers(this);
    }

    mirrorbuf = SDL_malloc(this->screen->h * this->screen->pitch);
    if (!mirrorbuf) {
        GLERR("HookDevice() failed to allocate temp buffer for mirroring!");
        return NULL;
    }

    return this->screen;
}

static int
glSDL_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color * colors)
{
    /* We don't need to fill this one */
    return 0;
}


#ifdef DEBUG_GLSDL
static void
glSDL_print_glerror(_THIS, int point)
{
    const char *err = "<unknown>";
    switch (this->glGetError()) {
    case GL_NO_ERROR:
        return;
    case GL_INVALID_ENUM:
        err = "GL_INVALID_ENUM";
        break;
    case GL_INVALID_VALUE:
        err = "GL_INVALID_VALUE";
        break;
    case GL_INVALID_OPERATION:
        err = "GL_INVALID_OPERATION";
        break;
    case GL_STACK_OVERFLOW:
        err = "GL_STACK_OVERFLOW";
        break;
    case GL_STACK_UNDERFLOW:
        err = "GL_STACK_UNDERFLOW";
        break;
    case GL_OUT_OF_MEMORY:
        err = "GL_OUT_OF_MEMORY";
    default:
        break;
    }
    fprintf(stderr, "OpenGL error \"%s\" at point %d.\n", err, point);
}
#endif

/* Get texinfo for a surface. */
static __inline__ private_hwdata *
glSDL_GetTexInfo(SDL_Surface * surface)
{
    if (!surface)
        return NULL;
    return surface->hwdata;
}


/* Allocate a "blank" texinfo for a suface. */
static private_hwdata *
glSDL_AllocTexInfo(SDL_Surface * surface)
{
    private_hwdata *txi;
    if (!surface)
        return NULL;

    txi = glSDL_GetTexInfo(surface);
    if (txi)
        return txi;             /* There already is one! --> */

    /* ...and hook a new texinfo struct up to it. */
    txi = (private_hwdata *) SDL_calloc(1, sizeof(private_hwdata));
    if (!txi) {
        GLERR("AllocTexInfo(): Failed allocating TexInfo struct!");
        return NULL;
    }
    txi->temporary = 1;
#ifdef DEBUG_GLSDL
    fprintf(stderr, "glSDL: Allocated TexInfo %p.\n", txi);
#endif
    return txi;
}


static void
glSDL_FreeTexInfo(_THIS, private_hwdata * txi)
{
    if (!txi)
        return;

    glSDL_UnloadTexture(this, txi);
    SDL_free(txi->texture);
    SDL_free(txi);
#ifdef DEBUG_GLSDL
    fprintf(stderr, "glSDL: Freed TexInfo %p.\n", txi);
#endif
}


/* Detach and free the texinfo of a surface. */
static void
glSDL_RemoveTexInfo(_THIS, SDL_Surface * surface)
{
    SDL_Surface *next, *prev;
    if (!glSDL_GetTexInfo(surface))
        return;

    /* maintain our doubly linked list */
    next = surface->hwdata->next;
    prev = surface->hwdata->prev;
    if (prev != NULL) {
        prev->hwdata->next = next;
    } else {
        first = next;
    }
    if (next != NULL) {
        next->hwdata->prev = prev;
    }

    glSDL_FreeTexInfo(this, surface->hwdata);
    surface->hwdata = NULL;
}


/*
 * Calculate chopping/tiling of a surface to
 * fit it into the smallest possible OpenGL
 * texture.
 */
static int
glSDL_CalcChop(private_hwdata * txi)
{
    int rows, vw, vh;
    int vertical = 0;
    int texsize;
    int lastw, lasth, minsize;

    vw = txi->virt.w;
    vh = txi->virt.h;

#ifdef DEBUG_GLSDL_CHOP
    fprintf(stderr, "w=%d, h=%d ", vw, vh);
#endif
    if (vh > vw) {
        int t = vw;
        vw = vh;
        vh = t;
        vertical = 1;
#ifdef DEBUG_GLSDL_CHOP
        fprintf(stderr, "(vertical) \t");
#endif
    }

    /*
     * Check whether this is a "huge" surface - at least one dimension
     * must be <= than the maximum texture size, or we'll have to chop
     * in both directions.
     */
#ifdef DEBUG_GLSDL
    if (maxtexsize < 0)
        return GLERET("glSDL_CalcChop() called before OpenGL init!");
#endif
    if (vh > maxtexsize) {
        /*
         * Very simple hack for now; we just tile
         * both ways with maximum size textures.
         */
        texsize = maxtexsize;

        txi->tilemode = GLSDL_TM_HUGE;
        txi->texsize = texsize;
        txi->tilew = texsize;
        txi->tileh = texsize;
        txi->tilespertex = 1;

        /* Calculate number of textures needed */
        txi->textures = (vw + texsize - 1) / texsize;
        txi->textures *= (vh + texsize - 1) / texsize;
        txi->texture = SDL_malloc(txi->textures * sizeof(int));
        SDL_memset(txi->texture, -1, txi->textures * sizeof(int));
#ifdef DEBUG_GLSDL
        fprintf(stderr, "two-way tiling; textures=%d\n", txi->textures);
#endif
        if (!txi->texture) {
            fprintf(stderr, "glSDL: INTERNAL ERROR: Failed to allocate"
                    " texture name table!\n");
            return -3;
        }
        return 0;
    }

    /* Calculate minimum size */
    rows = 1;
    lastw = vw;
    lasth = vh;
    minsize = lastw > lasth ? lastw : lasth;
    while (1) {
        int w, h, size;
        ++rows;
        w = vw / rows;
        h = rows * vh;
        size = w > h ? w : h;
        if (size >= minsize) {
            --rows;
            break;
        }
        lastw = w;
        lasth = h;
        minsize = size;
    }
    if (minsize > maxtexsize) {
        /* Handle multiple textures for very wide/tall surfaces. */
        minsize = maxtexsize;
        rows = (vw + minsize - 1) / minsize;
    }
#ifdef DEBUG_GLSDL_CHOP
    fprintf(stderr, "==> minsize=%d ", minsize);
    fprintf(stderr, "(rows=%d) \t", rows);
#endif

    /* Recalculate with nearest higher power-of-2 width. */
    for (texsize = 1; texsize < minsize; texsize <<= 1);
    txi->texsize = texsize;
    rows = (vw + texsize - 1) / texsize;
#ifdef DEBUG_GLSDL_CHOP
    fprintf(stderr, "==> texsize=%d (rows=%d) \t", texsize, rows);
#endif

    /* Calculate number of tiles per texture */
    txi->tilespertex = txi->texsize / vh;
#ifdef DEBUG_GLSDL_CHOP
    fprintf(stderr, "tilespertex=%d \t", txi->tilespertex);
#endif

    /* Calculate number of textures needed */
    txi->textures = (rows + txi->tilespertex - 1) / txi->tilespertex;
    txi->texture = (GLuint *) SDL_malloc(txi->textures * sizeof(GLuint));
    SDL_memset(txi->texture, GLSDL_NOTEX, txi->textures * sizeof(GLuint));
#ifdef DEBUG_GLSDL_CHOP
    fprintf(stderr, "textures=%d, ", txi->textures);
#endif
    if (!txi->texture)
        return GLERET("Failed to allocate texture name table!");

    /* Set up tile size. (Only one axis supported here!) */
    if (1 == rows) {
        txi->tilemode = GLSDL_TM_SINGLE;
        if (vertical) {
            txi->tilew = vh;
            txi->tileh = vw;
        } else {
            txi->tilew = vw;
            txi->tileh = vh;
        }
    } else if (vertical) {
        txi->tilemode = GLSDL_TM_VERTICAL;
        txi->tilew = vh;
        txi->tileh = texsize;
    } else {
        txi->tilemode = GLSDL_TM_HORIZONTAL;
        txi->tilew = texsize;
        txi->tileh = vh;
    }

#ifdef DEBUG_GLSDL_CHOP
    fprintf(stderr, "tilew=%d, tileh=%d\n", txi->tilew, txi->tileh);
#endif
    return 0;
}


/* Create a temporary TexInfo struct for an SDL_Surface */
static private_hwdata *
glSDL_CreateTempTexInfo(_THIS, SDL_Surface * surface)
{
    private_hwdata *txi;
    if (!surface) {
        GLERR("CreateTempTexInfo(); no surface!");
        return NULL;
    }
    if (IS_GLSDL_SURFACE(surface))
        return glSDL_GetTexInfo(surface);       /* Do nothing */

    txi = glSDL_AllocTexInfo(surface);
    if (!txi) {
        GLERR("CreateTempTexInfo(); Could not alloc TexInfo!");
        return NULL;
    }
    txi->virt.w = txi->lw = surface->w;
    txi->virt.h = txi->lh = surface->h;

    if (glSDL_CalcChop(txi) < 0) {
        glSDL_FreeTexInfo(this, txi);
        GLERR("CreateTempTexInfo(); CalcChop() failed!");
        return NULL;
    }

    return txi;
}

/* Add a glSDL_TexInfo struct to an SDL_Surface */
static private_hwdata *
glSDL_AddTexInfo(_THIS, SDL_Surface * surface)
{
    private_hwdata *txi = glSDL_CreateTempTexInfo(this, surface);
    if (!txi)
        return NULL;

    /* Connect the surface to the new TexInfo. */
    txi->temporary = 0;
    surface->hwdata = txi;

    /* add this new surface in front of the list of hw surfaces */
    txi->next = first;
    txi->prev = NULL;
    first = surface;
    if (txi->next != NULL) {
        txi->next->hwdata->prev = surface;
    }

    SDL_SetClipRect(surface, &txi->virt);
    return txi;
}


/* Create a surface of the prefered OpenGL RGB texture format */
/*static SDL_Surface *glSDL_CreateRGBSurface(int w, int h)
{
	SDL_Surface *s;
	Uint32 rmask, gmask, bmask;
	int bits = 24;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	rmask = 0x000000FF;
	gmask = 0x0000FF00;
	bmask = 0x00FF0000;
#else
	rmask = 0x00FF0000;
	gmask = 0x0000FF00;
	bmask = 0x000000FF;
#endif
	s = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
			bits, rmask, gmask, bmask, 0);
	if(s)
		s->flags |= SDL_HWACCEL;

	return s;
}
*/

/* Create a surface of the prefered OpenGL RGBA texture format */
static SDL_Surface *
glSDL_CreateRGBASurface(int w, int h)
{
    SDL_Surface *s;
    Uint32 rmask, gmask, bmask, amask;
    int bits = 32;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    rmask = 0x000000FF;
    gmask = 0x0000FF00;
    bmask = 0x00FF0000;
    amask = 0xFF000000;
#else
    rmask = 0xFF000000;
    gmask = 0x00FF0000;
    bmask = 0x0000FF00;
    amask = 0x000000FF;
#endif
    s = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
                             bits, rmask, gmask, bmask, amask);
    if (s)
        s->flags |= SDL_HWACCEL;

    return s;
}


static void
glSDL_init_formats(_THIS)
{
    RGBfmt = SDL_AllocFormat(24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                             0x000000FF, 0x0000FF00, 0x00FF0000, 0);
#else
                             0x00FF0000, 0x0000FF00, 0x000000FF, 0);
#endif
    RGBAfmt = SDL_AllocFormat(32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                              0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
#else
                              0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
#endif
}


static int
glSDL_FormatIsOk(SDL_Surface * surface)
{
    SDL_PixelFormat *pf;
    if (!surface)
        return 1;               /* Well, there ain't much we can do anyway... */

    pf = surface->format;

    /* Colorkeying requires an alpha channel! */
    if (surface->flags & SDL_SRCCOLORKEY)
        if (!pf->Amask)
            return 0;

    /* We need pitch == (width * BytesPerPixel) for glTex[Sub]Image2D() */
    if (surface->pitch != (surface->w * pf->BytesPerPixel))
        return 0;

    if (pf->Amask) {
        if (pf->BytesPerPixel != RGBAfmt->BytesPerPixel)
            return 0;
        if (pf->Rmask != RGBAfmt->Rmask)
            return 0;
        if (pf->Gmask != RGBAfmt->Gmask)
            return 0;
        if (pf->Bmask != RGBAfmt->Bmask)
            return 0;
        if (pf->Amask != RGBAfmt->Amask)
            return 0;
    } else {
        if (pf->BytesPerPixel != RGBfmt->BytesPerPixel)
            return 0;
        if (pf->Rmask != RGBfmt->Rmask)
            return 0;
        if (pf->Gmask != RGBfmt->Gmask)
            return 0;
        if (pf->Bmask != RGBfmt->Bmask)
            return 0;
    }
    return 1;
}

static void
glSDL_key2alpha(SDL_Surface * surface)
{
    int x, y;
    Uint32 ckey = surface->format->colorkey;

#ifdef DEBUG_GLSDL
    fprintf(stderr, "glSDL_key2alpha()\n");
#endif
    for (y = 0; y < surface->h; ++y) {
        Uint32 *px =
            (Uint32 *) ((char *) surface->pixels + y * surface->pitch);
        for (x = 0; x < surface->w; ++x)
            if (px[x] == ckey)
                px[x] = 0;
    }
}



/*----------------------------------------------------------
  SDL style API
  ----------------------------------------------------------*/

static int
glSDL_FlipHWSurface(_THIS, SDL_Surface * surface)
{
#ifdef GLSDL_GRAPHICAL_DEBUG
    this->glDisable(GL_TEXTURE_2D);
    this->glBegin(GL_LINE_LOOP);
    this->glColor4ub(0, 0, 255, 128);
    this->glVertex2i(0, 0);
    this->glVertex2i(surface->w, 0);
    this->glVertex2i(surface->w, surface->h);
    this->glVertex2i(0, surface->h);
    this->glEnd();
    this->glEnable(GL_TEXTURE_2D);
#endif
    if (this->screen->flags & SDL_DOUBLEBUF)
        this->GL_SwapBuffers(this);
    else
        this->glFinish();
    return 0;
}


static void
glSDL_UpdateRects(_THIS, int numrects, SDL_Rect * rects)
{
#ifdef GLSDL_GRAPHICAL_DEBUG
    int i;
    this->glDisable(GL_TEXTURE_2D);
    for (i = 0; i < numrects; i++) {
        this->glColor4ub(255, 0, 0, 128);
        this->glBegin(GL_LINE_LOOP);
        this->glVertex2i(rects[i].x, rects[i].y);
        this->glVertex2i(rects[i].x + rects[i].w, rects[i].y);
        this->glVertex2i(rects[i].x + rects[i].w, rects[i].y + rects[i].h);
        this->glVertex2i(rects[i].x, rects[i].y + rects[i].h);
        this->glEnd();
    }
    this->glEnable(GL_TEXTURE_2D);
#endif
    if (this->screen->flags & SDL_DOUBLEBUF)
        this->GL_SwapBuffers(this);
    else
        this->glFinish();
}


static int
glSDL_AllocHWSurface(_THIS, SDL_Surface * surface)
{
    surface->flags |= (SDL_HWSURFACE | SDL_HWACCEL);

    surface->pixels = SDL_malloc(surface->h * surface->pitch);
    if (surface->pixels == NULL) {
        SDL_FreeSurface(surface);
        SDL_OutOfMemory();
        return (-1);
    }
    SDL_memset(surface->pixels, 0, surface->h * surface->pitch);
    return 0;
}


static void
glSDL_FreeHWSurface(_THIS, SDL_Surface * surface)
{
    if (!surface)
        return;
    glSDL_RemoveTexInfo(this, surface);
}


static int
glSDL_LockHWSurface(_THIS, SDL_Surface * surface)
{
    int y;

    if (!surface)
        return -1;

#ifdef DEBUG_GLSDL
    fprintf(stderr, "glSDL: Lock Surface.\n");
#endif

    if (SDL_VideoSurface == surface) {
        glSDL_Invalidate(surface, NULL);
        this->glPixelStorei(GL_UNPACK_ROW_LENGTH,
                            surface->pitch / surface->format->BytesPerPixel);
        this->glReadPixels(0, 0, OpenGL_Surface->w, OpenGL_Surface->h,
                           GL_RGB, GL_UNSIGNED_BYTE, OpenGL_Surface->pixels);
        for (y = 0; y < OpenGL_Surface->h / 2; ++y) {
            void *upper = (Uint8 *) OpenGL_Surface->pixels +
                OpenGL_Surface->pitch * y;
            void *lower = (Uint8 *) OpenGL_Surface->pixels +
                OpenGL_Surface->pitch * (OpenGL_Surface->h - y - 1);
            SDL_memcpy(mirrorbuf, upper, OpenGL_Surface->pitch);
            SDL_memcpy(upper, lower, OpenGL_Surface->pitch);
            SDL_memcpy(lower, mirrorbuf, OpenGL_Surface->pitch);
        }
        /* the mapping has to be invalidated on 8bpp video surfaces in case of a hw palette change. 
         * Now if someone could tell me why this is not handled by SDL... */
        if (SDL_VideoSurface->format->BitsPerPixel == 8)
            SDL_InvalidateMap(OpenGL_Surface->map);

        /* convert this raw surface to the application-requested format 
         * FIXME this is sometimes overkill, we could use glPixelStore smartly
         * But this would be slow anyway :) */

        glSDL_SoftBlit(OpenGL_Surface, NULL, SDL_VideoSurface, NULL);
    } else
        glSDL_Invalidate(surface, NULL);

    return 0;
}


static void
glSDL_UnlockHWSurface(_THIS, SDL_Surface * surface)
{
    private_hwdata *txi;

    if (!surface)
        return;

    /* upload this surface ONLY if this is a glSDL surface
     * because sometimes (during displayformating for ex.) surfaces are unlocked that aren't glSDL
     */
    if (!IS_GLSDL_SURFACE(surface))
        return;

#ifdef DEBUG_GLSDL
    fprintf(stderr, "glSDL: Unlock Surface.\n");
#endif

    txi = glSDL_UploadSurface(this, surface);

    if (!txi) {
        GLERR("glSDL_UnlockHWSurface() failed to upload surface!");
        return;
    }
    if (txi->temporary) {
        GLERR
            ("Weirdness... glSDL_UnlockHWSurface() got a temporary TexInfo.");
        return;
    }
    if (surface == SDL_VideoSurface)
        glSDL_BlitGL(this, SDL_VideoSurface, NULL, NULL);
}


static int
glSDL_SetHWColorKey(_THIS, SDL_Surface * surface, Uint32 key)
{
    /*
     * If an application does this *after* SDL_DisplayFormat,
     * we're basically screwed, unless we want to do an
     * in-place surface conversion hack here.
     *
     * What we do is just kill the glSDL texinfo... No big
     * deal in most cases, as glSDL only converts once anyway,
     * *unless* you keep modifying the surface.
     */
    if (IS_GLSDL_SURFACE(surface))
        glSDL_RemoveTexInfo(this, surface);
    return 0;
}


static int
glSDL_SetHWAlpha(_THIS, SDL_Surface * surface, Uint8 alpha)
{
    /*
     * If an application does this *after* SDL_DisplayFormat,
     * we're basically screwed, unless we want to do an
     * in-place surface conversion hack here.
     *
     * What we do is just kill the glSDL texinfo... No big
     * deal in most cases, as glSDL only converts once anyway,
     * *unless* you keep modifying the surface.
     */
    if (IS_GLSDL_SURFACE(surface))
        glSDL_RemoveTexInfo(this, surface);
    return 0;
}

static SDL_bool
glSDL_SetClipRect(_THIS, SDL_Surface * surface, SDL_Rect * rect)
{
    SDL_bool res;
    if (!surface)
        return SDL_FALSE;

    res = SDL_SetClipRect(surface, rect);
    if (!res)
        return SDL_FALSE;

    rect = &surface->clip_rect;

    if (surface == SDL_VideoSurface) {
        SDL_Rect r;
        float xscale, yscale;
        private_hwdata *txi;

        r.x = rect->x;
        r.y = rect->y;
        r.w = rect->w;
        r.h = rect->h;
        SDL_SetClipRect(surface, rect);

        txi = glSDL_GetTexInfo(surface);
        if (!txi)
            return GLERET("SetClipRect(): Could not get TexInfo!");

        this->glViewport(rect->x,
                         surface->h - (rect->y + rect->h), rect->w, rect->h);
        /*
         * Note that this projection is upside down in
         * relation to the OpenGL coordinate system.
         */
        this->glMatrixMode(GL_PROJECTION);
        this->glLoadIdentity();
        xscale = (float) txi->lw / (float) surface->w;
        yscale = (float) txi->lh / (float) surface->h;
        this->glOrtho(xscale * (float) rect->x,
                      xscale * (float) (rect->w + rect->x),
                      yscale * (float) (rect->h + rect->y),
                      yscale * (float) rect->y, -1.0, 1.0);
        return SDL_TRUE;
    }
    return res;
}

static int
glSDL_BlitFromGL(_THIS, SDL_Rect * srcrect,
                 SDL_Surface * dst, SDL_Rect * dstrect)
{
    SDL_Rect sr, dr;

    /* In case the destination has an OpenGL texture... */
    glSDL_Invalidate(dst, dstrect);

    /* Abuse the fake screen buffer a little. */
    this->glPixelStorei(GL_UNPACK_ROW_LENGTH, SDL_VideoSurface->pitch /
                        SDL_VideoSurface->format->BytesPerPixel);
    if (srcrect)
        this->glReadPixels(srcrect->x,
                           OpenGL_Surface->h - (srcrect->y + srcrect->h - 1),
                           srcrect->w, srcrect->h, GL_RGB, GL_UNSIGNED_BYTE,
                           OpenGL_Surface->pixels);
    else
        this->glReadPixels(0, 0, OpenGL_Surface->w, OpenGL_Surface->h,
                           GL_RGB, GL_UNSIGNED_BYTE, OpenGL_Surface->pixels);
    sr = *srcrect;
    dr = *dstrect;
    glSDL_SoftBlit(OpenGL_Surface, &sr, dst, &dr);
    return 0;
}

static __inline__ void
glSDL_BlitGL_single(_THIS, private_hwdata * txi,
                    float sx1, float sy1, SDL_Rect * dst, unsigned char alpha)
{
    float sx2, sy2, texscale;
    if (!txi->textures)
        return;
    if (-1 == txi->texture[0])
        return;
    glSDL_texture(this, txi->texture[0]);

    texscale = 1.0 / (float) txi->texsize;
    sx2 = (sx1 + (float) dst->w) * texscale;
    sy2 = (sy1 + (float) dst->h) * texscale;
    sx1 *= texscale;
    sy1 *= texscale;

#ifdef GLSDL_GRAPHICAL_DEBUG
    this->glDisable(GL_TEXTURE_2D);
    this->glBegin(GL_LINE_LOOP);
    this->glColor4ub(0, 255, 0, 128);
    this->glVertex2i(dst->x, dst->y);
    this->glVertex2i(dst->x + dst->w, dst->y);
    this->glVertex2i(dst->x + dst->w, dst->y + dst->h);
    this->glVertex2i(dst->x, dst->y + dst->h);
    this->glEnd();
    this->glEnable(GL_TEXTURE_2D);
#endif

    this->glBegin(GL_TRIANGLE_FAN);
    this->glColor4ub(255, 255, 255, alpha);
    this->glTexCoord2f(sx1, sy1);
    this->glVertex2i(dst->x, dst->y);
    this->glTexCoord2f(sx2, sy1);
    this->glVertex2i(dst->x + dst->w, dst->y);
    this->glTexCoord2f(sx2, sy2);
    this->glVertex2i(dst->x + dst->w, dst->y + dst->h);
    this->glTexCoord2f(sx1, sy2);
    this->glVertex2i(dst->x, dst->y + dst->h);
    this->glEnd();
}


static void
glSDL_BlitGL_htile(_THIS, private_hwdata * txi,
                   float sx1, float sy1, SDL_Rect * dst, unsigned char alpha)
{
    int tex;
    float tile, sx2, sy2, yo;
    float texscale = 1.0 / (float) txi->texsize;
    float tileh = (float) txi->tileh * texscale;
    sx2 = (sx1 + (float) dst->w) * texscale;
    sy2 = (sy1 + (float) dst->h) * texscale;
    sx1 *= texscale;
    sy1 *= texscale;
    tile = floor(sx1);
    tex = (int) tile / txi->tilespertex;
    yo = ((int) tile % txi->tilespertex) * tileh;

    if (tex >= txi->textures)
        return;
    if (-1 == txi->texture[tex])
        return;
    glSDL_texture(this, txi->texture[tex]);

    while (tile < sx2) {
        int tdx1 = dst->x;
        int tdx2 = dst->x + dst->w;
        float tsx1 = sx1 - tile;
        float tsx2 = sx2 - tile;

        /* Clip to current tile */
        if (tsx1 < 0.0) {
            tdx1 -= tsx1 * txi->texsize;
            tsx1 = 0.0;
        }
        if (tsx2 > 1.0) {
            tdx2 -= (tsx2 - 1.0) * txi->texsize;
            tsx2 = 1.0;
        }

        /* Maybe select next texture? */
        if (yo + tileh > 1.0) {
            ++tex;
            if (tex >= txi->textures)
                return;
            if (-1 == txi->texture[tex])
                return;
            glSDL_texture(this, txi->texture[tex]);
            yo = 0.0;
        }
#ifdef GLSDL_GRAPHICAL_DEBUG
        this->glDisable(GL_TEXTURE_2D);
        this->glBegin(GL_LINE_LOOP);
        this->glColor4ub(0, 255, 0, 128);
        this->glVertex2i(tdx1, dst->y);
        this->glVertex2i(tdx2, dst->y);
        this->glVertex2i(tdx2, dst->y + dst->h);
        this->glVertex2i(tdx1, dst->y + dst->h);
        this->glEnd();
        this->glEnable(GL_TEXTURE_2D);
#endif

        this->glBegin(GL_TRIANGLE_FAN);
        this->glColor4ub(255, 255, 255, alpha);
        this->glTexCoord2f(tsx1, yo + sy1);
        this->glVertex2i(tdx1, dst->y);
        this->glTexCoord2f(tsx2, yo + sy1);
        this->glVertex2i(tdx2, dst->y);
        this->glTexCoord2f(tsx2, yo + sy2);
        this->glVertex2i(tdx2, dst->y + dst->h);
        this->glTexCoord2f(tsx1, yo + sy2);
        this->glVertex2i(tdx1, dst->y + dst->h);
        this->glEnd();
        tile += 1.0;
        yo += tileh;
    }
}


static void
glSDL_BlitGL_vtile(_THIS, private_hwdata * txi,
                   float sx1, float sy1, SDL_Rect * dst, unsigned char alpha)
{
    int tex;
    float tile, sx2, sy2, xo;
    float texscale = 1.0 / (float) txi->texsize;
    float tilew = (float) txi->tilew * texscale;
    sx2 = (sx1 + (float) dst->w) * texscale;
    sy2 = (sy1 + (float) dst->h) * texscale;
    sx1 *= texscale;
    sy1 *= texscale;
    tile = floor(sy1);
    tex = (int) tile / txi->tilespertex;
    xo = ((int) tile % txi->tilespertex) * tilew;

    if (tex >= txi->textures)
        return;
    if (-1 == txi->texture[tex])
        return;
    glSDL_texture(this, txi->texture[tex]);

    while (tile < sy2) {
        int tdy1 = dst->y;
        int tdy2 = dst->y + dst->h;
        float tsy1 = sy1 - tile;
        float tsy2 = sy2 - tile;

        /* Clip to current tile */
        if (tsy1 < 0.0) {
            tdy1 -= tsy1 * txi->texsize;
            tsy1 = 0.0;
        }
        if (tsy2 > 1.0) {
            tdy2 -= (tsy2 - 1.0) * txi->texsize;
            tsy2 = 1.0;
        }

        /* Maybe select next texture? */
        if (xo + tilew > 1.0) {
            ++tex;
            if (tex >= txi->textures)
                return;
            if (-1 == txi->texture[tex])
                return;
            glSDL_texture(this, txi->texture[tex]);
            xo = 0.0;
        }
#ifdef GLSDL_GRAPHICAL_DEBUG
        this->glDisable(GL_TEXTURE_2D);
        this->glBegin(GL_LINE_LOOP);
        this->glColor4ub(0, 255, 0, 128);
        this->glVertex2i(dst->x, tdy1);
        this->glVertex2i(dst->x + dst->w, tdy1);
        this->glVertex2i(dst->x + dst->w, tdy2);
        this->glVertex2i(dst->x, tdy2);
        this->glEnd();
        this->glEnable(GL_TEXTURE_2D);
#endif

        this->glBegin(GL_TRIANGLE_FAN);
        this->glColor4ub(255, 255, 255, alpha);
        this->glTexCoord2f(xo + sx1, tsy1);
        this->glVertex2i(dst->x, tdy1);
        this->glTexCoord2f(xo + sx2, tsy1);
        this->glVertex2i(dst->x + dst->w, tdy1);
        this->glTexCoord2f(xo + sx2, tsy2);
        this->glVertex2i(dst->x + dst->w, tdy2);
        this->glTexCoord2f(xo + sx1, tsy2);
        this->glVertex2i(dst->x, tdy2);
        this->glEnd();

        tile += 1.0;
        xo += tilew;
    }
}


static void
glSDL_BlitGL_hvtile(_THIS, SDL_Surface * src, private_hwdata * txi,
                    float sx1, float sy1, SDL_Rect * dst, unsigned char alpha)
{
    int x, y, last_tex, tex;
    float sx2, sy2;
    float texscale = 1.0 / (float) txi->texsize;
    int tilesperrow = (src->w + txi->tilew - 1) / txi->tilew;
    sx2 = (sx1 + (float) dst->w) * texscale;
    sy2 = (sy1 + (float) dst->h) * texscale;
    sx1 *= texscale;
    sy1 *= texscale;

    last_tex = tex = floor(sy1) * tilesperrow + floor(sx1);
    if (tex >= txi->textures)
        return;
    if (-1 == txi->texture[tex])
        return;
    glSDL_texture(this, txi->texture[tex]);

    for (y = floor(sy1); y < sy2; ++y) {
        int tdy1 = dst->y;
        int tdy2 = dst->y + dst->h;
        float tsy1 = sy1 - y;
        float tsy2 = sy2 - y;

        /* Clip to current tile */
        if (tsy1 < 0.0) {
            tdy1 -= tsy1 * txi->texsize;
            tsy1 = 0.0;
        }
        if (tsy2 > 1.0) {
            tdy2 -= (tsy2 - 1.0) * txi->texsize;
            tsy2 = 1.0;
        }
        for (x = floor(sx1); x < sx2; ++x) {
            int tdx1 = dst->x;
            int tdx2 = dst->x + dst->w;
            float tsx1 = sx1 - x;
            float tsx2 = sx2 - x;

            /* Clip to current tile */
            if (tsx1 < 0.0) {
                tdx1 -= tsx1 * txi->texsize;
                tsx1 = 0.0;
            }
            if (tsx2 > 1.0) {
                tdx2 -= (tsx2 - 1.0) * txi->texsize;
                tsx2 = 1.0;
            }

            /* Select texture */
            tex = y * tilesperrow + x;
            if (tex != last_tex) {
                if (tex >= txi->textures)
                    return;
                if (-1 == txi->texture[tex])
                    return;
                glSDL_texture(this, txi->texture[tex]);
                last_tex = tex;
            }
#ifdef GLSDL_GRAPHICAL_DEBUG
            this->glDisable(GL_TEXTURE_2D);
            this->glBegin(GL_LINE_LOOP);
            this->glColor4ub(0, 255, 0, 128);
            this->glVertex2i(tdx1, tdy1);
            this->glVertex2i(tdx2, tdy1);
            this->glVertex2i(tdx2, tdy2);
            this->glVertex2i(tdx1, tdy2);
            this->glEnd();
            this->glEnable(GL_TEXTURE_2D);
#endif

            this->glBegin(GL_TRIANGLE_FAN);
            this->glColor4ub(255, 255, 255, alpha);
            this->glTexCoord2f(tsx1, tsy1);
            this->glVertex2i(tdx1, tdy1);
            this->glTexCoord2f(tsx2, tsy1);
            this->glVertex2i(tdx2, tdy1);
            this->glTexCoord2f(tsx2, tsy2);
            this->glVertex2i(tdx2, tdy2);
            this->glTexCoord2f(tsx1, tsy2);
            this->glVertex2i(tdx1, tdy2);
            this->glEnd();
        }
    }
}

/*
 * Calculate the actual blit rectangle and source offset
 * for a blit from a rectangle in a surface with specified
 * size to a surface with a cliprect.
 *
 * In:	rect	source rectangle
 *	w, h	source surface size
 *	(x, y)	destination coordinate
 *	clip	destination clip rectangle
 *
 * Out:	(x, y)	source top-left offset
 *	rect	destination rectangle
 *
 * Returns 1 if the result is visible, otherwise 0.
 */
static __inline__ int
blitclip(SDL_Rect * rect, int w, int h, int *x, int *y, SDL_Rect * clip)
{
    int sx1, sy1, sx2, sy2;
    int dx1, dy1, dx2, dy2;

    /* Get source and destination coordinates */
    sx1 = rect->x;
    sy1 = rect->y;
    sx2 = sx1 + rect->w;
    sy2 = sy1 + rect->h;
    dx1 = *x;
    dy1 = *y;

    /* Keep source rect inside source surface */
    if (sx1 < 0) {
        dx1 -= sx1;
        sx1 = 0;
    }
    if (sy1 < 0) {
        dy1 -= sy1;
        sy1 = 0;
    }
    if (sx2 > w)
        sx2 = w;
    if (sy2 > h)
        sy2 = h;

    /* Cull blits from void space */
    if (sx1 >= sx2 || sy1 >= sy2)
        return 0;

    /* Calculate destination lower-right */
    dx2 = dx1 + (sx2 - sx1);
    dy2 = dy1 + (sy2 - sy1);

    /* Clip to destination cliprect */
    if (dx1 < clip->x) {
        sx1 += clip->x - dx1;
        dx1 = clip->x;
    }
    if (dy1 < clip->y) {
        sy1 += clip->y - dy1;
        dy1 = clip->y;
    }
    if (dx2 > clip->x + clip->w)
        dx2 = clip->x + clip->w;
    if (dy2 > clip->y + clip->h)
        dy2 = clip->y + clip->h;

    /* Cull nop/off-screen blits */
    if (dx1 >= dx2 || dy1 >= dy2)
        return 0;

    *x = sx1;
    *y = sy1;
    rect->x = dx1;
    rect->y = dy1;
    rect->w = dx2 - dx1;
    rect->h = dy2 - dy1;
    return 1;
}

static int
glSDL_BlitGL(_THIS, SDL_Surface * src, SDL_Rect * srcrect, SDL_Rect * dstrect)
{
    private_hwdata *txi;
    float x1, y1;
    unsigned char alpha;
    SDL_Rect d;
    int x, y;
    SDL_Rect r;

    if (!src)
        return GLERET("BlitGL(): No src surface!");

    /* Get source and destination coordinates */
    if (srcrect)
        r = *srcrect;
    else {
        r.x = r.y = 0;
        r.w = src->w;
        r.h = src->h;
    }
    if (dstrect) {
        x = dstrect->x;
        y = dstrect->y;
    } else
        x = y = 0;

    /* Clip! */
    if (!blitclip(&r, src->w, src->h, &x, &y, &this->screen->clip_rect)) {
        if (dstrect)
            dstrect->w = dstrect->h = 0;
        return 0;
    }

    /* Write back the resulting cliprect */
    if (dstrect)
        *dstrect = r;

    /* Make sure we have a source with a valid texture */
    txi = glSDL_UploadSurface(this, src);
    if (!txi)
        return GLERET("BlitGL(): Could not get a TexInfo!");

    /* Set up blending */
    if (src->flags & (SDL_SRCALPHA | SDL_SRCCOLORKEY)) {
        glSDL_blendfunc(this, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glSDL_do_blend(this, 1);
    } else
        glSDL_do_blend(this, 0);

    /* Enable texturing */
    glSDL_do_texture(this, 1);

    /* Calculate texcoords */
    if (!srcrect)
        srcrect = &txi->virt;
    x1 = (float) srcrect->x;
    y1 = (float) srcrect->y;

    /* Calculate screen coords. */
    if (dstrect) {
        d.x = dstrect->x;
        d.y = dstrect->y;
        d.w = (int) (srcrect->w * (float) txi->lw / (float) txi->virt.w);
        d.h = (int) (srcrect->h * (float) txi->lh / (float) txi->virt.h);
    } else {
        d.x = 0;
        d.y = 0;
        d.w = (int) (srcrect->w * (float) txi->lw / (float) txi->virt.w);
        d.h = (int) (srcrect->h * (float) txi->lh / (float) txi->virt.h);
    }

    /*
     * Note that we actually *prevent* the use of "full surface alpha"
     * and alpha channel in combination - to stay SDL 2D compatible.
     */
    if ((src->flags & SDL_SRCALPHA) && (src->format->Amask))
        alpha = 255;
    else
        alpha = src->format->alpha;

    /* Render! */
    switch (txi->tilemode) {
    case GLSDL_TM_SINGLE:
        glSDL_BlitGL_single(this, txi, x1, y1, &d, alpha);
        break;
    case GLSDL_TM_HORIZONTAL:
        glSDL_BlitGL_htile(this, txi, x1, y1, &d, alpha);
        break;
    case GLSDL_TM_VERTICAL:
        glSDL_BlitGL_vtile(this, txi, x1, y1, &d, alpha);
        break;
    case GLSDL_TM_HUGE:
        glSDL_BlitGL_hvtile(this, src, txi, x1, y1, &d, alpha);
        break;
    }

    if (txi->temporary)
        glSDL_FreeTexInfo(this, txi);

    return 0;
}


static int
glSDL_HWAccelBlit(SDL_Surface * src, SDL_Rect * srcrect,
                  SDL_Surface * dst, SDL_Rect * dstrect)
{
    SDL_Surface *vs;

    if (!src)
        return GLERET("HWAccelBlit(): No src surface!");
    if (!dst)
        return GLERET("HWAccelBlit(): No dst surface!");

    /*
     * Figure out what to do:
     *      screen->screen:         glSDL_BlitFromGL() + glSDL_BlitGL()
     *      surface->screen:        glSDL_BlitGL()
     *      screen->surface:        glSDL_BlitFromGL()
     *      surface->surface:       glSDL_SoftBlit()
     */
    vs = SDL_VideoSurface;
    if (src == vs) {
        if (dst == vs) {
            /*
               FIXME: Try glCopyPixels() instead...
             */
            glSDL_BlitFromGL(current_video, srcrect, vs, dstrect);
            return glSDL_BlitGL(current_video, vs, srcrect, dstrect);
        } else {
            return glSDL_BlitFromGL(current_video, srcrect, dst, dstrect);
        }
    } else {
        if (dst == vs) {
            return glSDL_BlitGL(current_video, src, srcrect, dstrect);
        } else {
            glSDL_Invalidate(dst, dstrect);
            glSDL_SoftBlit(src, srcrect, dst, dstrect);
            return 0;
        }
    }
}


static int
glSDL_FillHWRect(_THIS, SDL_Surface * dst, SDL_Rect * dstrect, Uint32 color)
{
    SDL_Surface *vs = SDL_VideoSurface;
    int dx1, dy1, dx2, dy2;
    Uint32 r, g, b;
    Uint8 br, bg, bb;

    /*
     * Some ugly reverse conversion for compatibility...
     * (We must do this before losing the dst pointer,
     * as the pixel formats of the screen and
     * SDL_VideoSurface may differ!)
     */

    if (dst->format->palette) {
        /* this a paletted color */
        SDL_GetRGB(color, dst->format, &br, &bg, &bb);
    } else {
        /* this a RGB color */
        r = color & dst->format->Rmask;
        r = r >> dst->format->Rshift;
        r = r << dst->format->Rloss;
        br = r;

        g = color & dst->format->Gmask;
        g = g >> dst->format->Gshift;
        g = g << dst->format->Gloss;
        bg = g;

        b = color & dst->format->Bmask;
        b = b >> dst->format->Bshift;
        b = b << dst->format->Bloss;
        bb = b;
    }

    if (vs != dst) {
        /* draw a rect offscreen */
        glSDL_Invalidate(dst, dstrect);
        /* software-fill the surface by faking it as a SW_SURFACE */
        dst->flags &= ~SDL_HWSURFACE;
        SDL_FillRect(dst, dstrect, color);
        dst->flags |= SDL_HWSURFACE;
    } else {
        /* draw a rect onscreen */
        glSDL_do_texture(this, 0);
        glSDL_do_blend(this, 0);

        dx1 = dstrect->x;
        dy1 = dstrect->y;
        dx2 = dx1 + dstrect->w;
        dy2 = dy1 + dstrect->h;

        this->glBegin(GL_TRIANGLE_FAN);
        this->glColor3ub(br, bg, bb);
        this->glVertex2i(dx1, dy1);
        this->glVertex2i(dx2, dy1);
        this->glVertex2i(dx2, dy2);
        this->glVertex2i(dx1, dy2);
        this->glEnd();
    }
    return 0;
}

static int
glSDL_CheckHWBlit(_THIS, SDL_Surface * src, SDL_Surface * dst)
{
    src->flags |= SDL_HWACCEL;
    src->map->hw_blit = glSDL_HWAccelBlit;
    return 1;
}


static SDL_Surface *
glSDL_DisplayFormat(SDL_Surface * surface)
{
    SDL_Surface *tmp;
    int use_rgba = (surface->flags & SDL_SRCCOLORKEY) ||
        ((surface->flags & SDL_SRCALPHA) && surface->format->Amask);
#ifdef DEBUG_GLSDL
    fprintf(stderr, "#### glSDL_DisplayFormat()\n");
#endif
    if (use_rgba)
        tmp = glSDL_ConvertSurface(surface, RGBAfmt, SDL_SWSURFACE);
    else
        tmp = glSDL_ConvertSurface(surface, RGBfmt, SDL_SWSURFACE);
    if (!tmp) {
        GLERR("glSDL_DisplayFormat() could not convert surface!");
        return NULL;
    }
    SDL_SetAlpha(tmp, 0, 0);

    if (surface->flags & SDL_SRCCOLORKEY) {
        /*
         * We drop colorkey data here, but we have to,
         * or we'll run into trouble when converting,
         * in particular from indexed color formats.
         */
        SDL_SetColorKey(tmp, SDL_SRCCOLORKEY, surface->format->colorkey);
        glSDL_key2alpha(tmp);
        SDL_SetColorKey(tmp, 0, 0);
    }

    return tmp;
}


static SDL_Surface *
glSDL_DisplayFormatAlpha(SDL_Surface * surface)
{
    SDL_Surface *s, *tmp;
    tmp = glSDL_ConvertSurface(surface, RGBAfmt, SDL_SWSURFACE);
#ifdef DEBUG_GLSDL
    fprintf(stderr, "#### glSDL_DisplayFormatAlpha()\n");
#endif
    if (!tmp)
        return NULL;

    SDL_SetAlpha(tmp, 0, 0);
    SDL_SetColorKey(tmp, 0, 0);
    s = glSDL_CreateRGBASurface(surface->w, surface->h);
    if (!s) {
        SDL_FreeSurface(tmp);
        return NULL;
    }
    glSDL_SoftBlit(tmp, NULL, s, NULL);
    SDL_FreeSurface(tmp);

    if (surface->flags & SDL_SRCCOLORKEY) {
        SDL_SetColorKey(s, SDL_SRCCOLORKEY, surface->format->colorkey);
        glSDL_key2alpha(s);
        SDL_SetColorKey(s, 0, 0);
    }

    if (surface->flags & SDL_SRCALPHA)
        SDL_SetAlpha(s, SDL_SRCALPHA, surface->format->alpha);
    return s;
}


/*----------------------------------------------------------
  glSDL specific API extensions
  ----------------------------------------------------------*/

static void
glSDL_Invalidate(SDL_Surface * surface, SDL_Rect * area)
{
    private_hwdata *txi;
    if (!surface)
        return;
    txi = glSDL_GetTexInfo(surface);
    if (!txi)
        return;
    if (!area) {
        txi->invalid_area.x = 0;
        txi->invalid_area.y = 0;
        txi->invalid_area.w = surface->w;
        txi->invalid_area.h = surface->h;
        return;
    }
    txi->invalid_area = *area;
}


static void
glSDL_SetLogicSize(_THIS, SDL_Surface * surface, int w, int h)
{
    SDL_Rect r;
    private_hwdata *txi;
    if (!IS_GLSDL_SURFACE(surface))
        return;

    txi = glSDL_GetTexInfo(surface);

    txi->lw = w;
    txi->lh = h;

    if (SDL_VideoSurface != surface)
        return;

    r.x = r.y = 0;
    r.w = w;
    r.h = h;
    glSDL_SetClipRect(this, surface, &r);

    this->glMatrixMode(GL_MODELVIEW);
    this->glLoadIdentity();
    this->glTranslated(0.0f, 0.0f, 0.0f);

    this->glDisable(GL_DEPTH_TEST);
    this->glDisable(GL_CULL_FACE);

    glSDL_reset();
}

static int
glSDL_InitTexture(_THIS, SDL_Surface * datasurf, private_hwdata * txi,
                  int tex)
{
    this->glGenTextures(1, (GLuint *) & txi->texture[tex]);
    this->glBindTexture(GL_TEXTURE_2D, txi->texture[tex]);
    glstate.texture = txi->texture[tex];
    this->glPixelStorei(GL_UNPACK_ROW_LENGTH, datasurf->pitch /
                        datasurf->format->BytesPerPixel);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexImage2D(GL_TEXTURE_2D, 0,
                       datasurf->format->Amask ? GL_RGBA8 : GL_RGB8,
                       txi->texsize, txi->texsize, 0,
                       datasurf->format->Amask ? GL_RGBA : GL_RGB,
                       GL_UNSIGNED_BYTE, NULL);
#ifdef DEBUG_GLSDL
    glSDL_print_glerror(this, 1);
#endif
    return 0;
}


/* Image tiled horizontally (wide surface), or not at all */
static int
glSDL_UploadHoriz(_THIS, SDL_Surface * datasurf, private_hwdata * txi)
{
    int bpp = datasurf->format->BytesPerPixel;
    int res;
    int tex = 0;
    int fromx = 0;
    int toy = txi->texsize;     /* To init first texture */
    while (1) {
        int thistw = datasurf->w - fromx;
        if (thistw > txi->tilew)
            thistw = txi->tilew;
        else if (thistw <= 0)
            break;
        if (toy + txi->tileh > txi->texsize) {
            toy = 0;
            res = glSDL_InitTexture(this, datasurf, txi, tex);
            if (res < 0)
                return res;
            ++tex;
        }
        this->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, toy,
                              thistw, txi->tileh,
                              datasurf->format->Amask ? GL_RGBA : GL_RGB,
                              GL_UNSIGNED_BYTE,
                              (char *) datasurf->pixels + bpp * fromx);
#ifdef DEBUG_GLSDL
        glSDL_print_glerror(this, 2);
#endif
        fromx += txi->tilew;
        toy += txi->tileh;
    }
    return 0;
}


/* Image tiled vertically (tall surface) */
static int
glSDL_UploadVert(_THIS, SDL_Surface * datasurf, private_hwdata * txi)
{
    int res;
    int tex = 0;
    int fromy = 0;
    int tox = txi->texsize;     /* To init first texture */
    while (1) {
        int thisth = datasurf->h - fromy;
        if (thisth > txi->tileh)
            thisth = txi->tileh;
        else if (thisth <= 0)
            break;
        if (tox + txi->tilew > txi->texsize) {
            tox = 0;
            res = glSDL_InitTexture(this, datasurf, txi, tex);
            if (res < 0)
                return res;
            ++tex;
        }
        this->glTexSubImage2D(GL_TEXTURE_2D, 0, tox, 0,
                              txi->tilew, thisth,
                              datasurf->format->Amask ? GL_RGBA : GL_RGB,
                              GL_UNSIGNED_BYTE,
                              (char *) datasurf->pixels +
                              datasurf->pitch * fromy);
#ifdef DEBUG_GLSDL
        glSDL_print_glerror(this, 3);
#endif
        fromy += txi->tileh;
        tox += txi->tilew;
    }
    return 0;
}


/* Image tiled two-way (huge surface) */
static int
glSDL_UploadHuge(_THIS, SDL_Surface * datasurf, private_hwdata * txi)
{
    int bpp = datasurf->format->BytesPerPixel;
    int res;
    int tex = 0;
    int y = 0;
    while (y < datasurf->h) {
        int x;
        int thisth = datasurf->h - y;
        if (thisth > txi->tileh)
            thisth = txi->tileh;
        x = 0;
        while (x < datasurf->w) {
            int thistw = datasurf->w - x;
            if (thistw > txi->tilew)
                thistw = txi->tilew;
            res = glSDL_InitTexture(this, datasurf, txi, tex++);
            if (res < 0)
                return res;
            this->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                                  thistw, thisth,
                                  datasurf->format->
                                  Amask ? GL_RGBA : GL_RGB,
                                  GL_UNSIGNED_BYTE,
                                  (char *) datasurf->pixels +
                                  datasurf->pitch * y + bpp * x);
#ifdef DEBUG_GLSDL
            fprintf(stderr,
                    "glTexSubImage(x = %d, y = %d, w = %d, h = %d)\n", x,
                    y, thistw, thisth);
            glSDL_print_glerror(this, 4);
#endif
            x += txi->tilew;
        }
        y += txi->tileh;
    }
    return 0;
}


/* Upload all textures for a surface. */
static int
glSDL_UploadTextures(_THIS, SDL_Surface * datasurf, private_hwdata * txi)
{
    switch (txi->tilemode) {
    case GLSDL_TM_SINGLE:
    case GLSDL_TM_HORIZONTAL:
        glSDL_UploadHoriz(this, datasurf, txi);
        break;
    case GLSDL_TM_VERTICAL:
        glSDL_UploadVert(this, datasurf, txi);
        break;
    case GLSDL_TM_HUGE:
        glSDL_UploadHuge(this, datasurf, txi);
        break;
    }
    return 0;
}


/*
 * IMPORTANT:
 *	This function will try various ways of giving you
 *	a TexInfo, and will succeed most of the time.
 *
 *	However, the TexInfo returned may be temporary,
 *	(as opposed to connected to 'surface'). A temporary
 *	TexInfo must be used only once and then thrown away,
 *	since it means that glSDL cannot track changes in
 *	the pixel data of 'texture'.
 */
static private_hwdata *
glSDL_UploadSurface(_THIS, SDL_Surface * surface)
{
    int i;
    int converted = 0;
    private_hwdata *txi = glSDL_GetTexInfo(surface);

    if (IS_GLSDL_SURFACE(surface)) {
        /*
         * Ok, this is a glSDL surface, and it *might* be
         * in texture memory already. If so, it may need
         * an update.
         */
        if (txi->invalid_area.w) {
            glSDL_UnloadTexture(this, txi);
        } else {
            int missing = 0;
            if (txi->textures) {
                for (i = 0; i < txi->textures; ++i)
                    if (GLSDL_NOTEX == txi->texture[i]) {
                        missing = 1;
                        break;
                    }
                if (!missing)
                    return txi; /* They're already there! */
            }
        }
    } else {
        /*
         * Nope, this isn't (yet) a glSDL surface. Let's
         * try to either make it one, or set up a temporary
         * TexInfo for it, valid for only one blit.
         */
        if ((surface->flags & SDL_HWSURFACE) == SDL_HWSURFACE) {
            txi = glSDL_AddTexInfo(this, surface);
            if (!txi) {
                GLERR("UploadSurface(): Could not add TexInfo!");
                return NULL;
            }
            surface->flags |= SDL_HWSURFACE;
            surface->flags |= SDL_HWACCEL;
        } else {
            /* 
             * FIXME
             * here if the surface is small enough, it's a good 
             * candidate for a blit using glDrawPixels instead 
             * of a texture blit
             */
            txi = glSDL_CreateTempTexInfo(this, surface);
            if (!txi) {
                GLERR("UploadSurface(): Could not create temp TexInfo!");
                return NULL;
            }
        }
    }

    if (txi->texsize > maxtexsize) {
        /* This surface wasn't tiled properly... */
        if (txi->temporary)
            glSDL_FreeTexInfo(this, txi);
        GLERR("UploadSurface(): Too large texture!");
        return NULL;
    }

    /*
     * Kludge: Convert if not of preferred RGB or RGBA format.
     *
     *      Conversion should only be done when *really* needed.
     *      That is, it should rarely have to be done with OpenGL
     *      1.2+.
     *
     *      Besides, any surface that's been SDL_DisplayFormat()ed
     *      should already be in the best known OpenGL format -
     *      preferably one that makes DMA w/o conversion possible.
     */
    if (!glSDL_FormatIsOk(surface)) {
#ifdef DEBUG_GLSDL
        fprintf(stderr, "glSDL: WARNING: On-the-fly conversion performed!\n");
#endif
        converted = 1;
        /* NOTE: We forget about the original surface here. */
        if (surface->format->Amask)
            surface = glSDL_DisplayFormatAlpha(surface);
        else
            surface = glSDL_DisplayFormat(surface);
        if (!surface) {
            GLERR("UploadSurface(): Could not convert surface!");
            if (txi->temporary)
                glSDL_FreeTexInfo(this, txi);
            return NULL;
        }
    }

    glSDL_UploadTextures(this, surface, txi);

    if (converted)
        SDL_FreeSurface(surface);

    return txi;
}


static void
glSDL_UnloadTexture(_THIS, private_hwdata * txi)
{
    int i;
    for (i = 0; i < txi->textures; ++i)
        if (txi->texture[i] != GLSDL_NOTEX)
            this->glDeleteTextures(1, &txi->texture[i]);
    SDL_memset(&txi->invalid_area, 0, sizeof(txi->invalid_area));
}

/* vi: set ts=4 sw=4 expandtab: */
