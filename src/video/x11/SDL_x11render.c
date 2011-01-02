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

#if SDL_VIDEO_RENDER_X11

#include <limits.h> /* For INT_MIN and INT_MAX */

#include "SDL_x11video.h"
#include "SDL_x11render.h"
#include "../SDL_rect_c.h"
#include "../SDL_pixels_c.h"
#include "../SDL_yuv_sw_c.h"

/* X11 renderer implementation */

static SDL_Renderer *X11_CreateRenderer(SDL_Window * window, Uint32 flags);
static int X11_DisplayModeChanged(SDL_Renderer * renderer);
static int X11_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int X11_QueryTexturePixels(SDL_Renderer * renderer,
                                  SDL_Texture * texture, void **pixels,
                                  int *pitch);
static int X11_SetTextureRGBAMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int X11_SetTextureBlendMode(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int X11_SetTextureScaleMode(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static int X11_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * rect, const void *pixels,
                             int pitch);
static int X11_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * rect, int markDirty,
                           void **pixels, int *pitch);
static void X11_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int X11_SetDrawBlendMode(SDL_Renderer * renderer);
static int X11_RenderDrawPoints(SDL_Renderer * renderer,
                                const SDL_Point * points, int count);
static int X11_RenderDrawLines(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int X11_RenderDrawRects(SDL_Renderer * renderer,
                               const SDL_Rect ** rects, int count);
static int X11_RenderFillRects(SDL_Renderer * renderer,
                               const SDL_Rect ** rects, int count);
static int X11_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static int X11_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                                Uint32 format, void * pixels, int pitch);
static int X11_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                                 Uint32 format, const void * pixels, int pitch);
static void X11_RenderPresent(SDL_Renderer * renderer);
static void X11_DestroyTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static void X11_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver X11_RenderDriver = {
    X11_CreateRenderer,
    {
     "x11",
     (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY |
      SDL_RENDERER_PRESENTFLIP2 | SDL_RENDERER_PRESENTFLIP3 |
      SDL_RENDERER_PRESENTDISCARD | SDL_RENDERER_ACCELERATED),
     SDL_TEXTUREMODULATE_NONE,
     SDL_BLENDMODE_NONE,
     SDL_SCALEMODE_NONE,
     0,
     {0},
     0,
     0}
};

typedef struct
{
    Display *display;
    int screen;
    Visual *visual;
    int depth;
    int scanline_pad;
    Window xwindow;
    Pixmap pixmaps[3];
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    Pixmap stencil;
    Pixmap brush;
    Picture brush_pict;
    Picture xwindow_pict;
    Picture pixmap_picts[3];
    Picture drawable_pict;
    Picture stencil_pict;
    int blend_op;
    XRenderPictFormat *xwindow_pict_fmt;
    XRenderPictFormat *drawable_pict_fmt;
    GC stencil_gc;
    SDL_bool use_xrender;
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
    SDL_bool use_xdamage;
    Damage stencil_damage;
    XserverRegion stencil_parts;
#endif
#endif
    int current_pixmap;
    Drawable drawable;
    SDL_PixelFormat format;
    GC gc;
    SDL_DirtyRectList dirty;
    SDL_bool makedirty;
} X11_RenderData;

typedef struct
{
    SDL_SW_YUVTexture *yuv;
    Uint32 format;
    Pixmap pixmap;
    int depth;
    Visual *visual;
    GC gc;
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    Picture picture;
    Pixmap modulated_pixmap;
    Picture modulated_picture;
    XRenderPictFormat* picture_fmt;
    int blend_op;
    const char* filter;
#endif
    XImage *image;
#ifndef NO_SHARED_MEMORY
    /* MIT shared memory extension information */
    XShmSegmentInfo shminfo;
#endif
    XImage *scaling_image;
    void *pixels;
    int pitch;
} X11_TextureData;

#ifndef NO_SHARED_MEMORY
/* Shared memory error handler routine */
static int shm_error;
static int (*X_handler) (Display *, XErrorEvent *) = NULL;
static int
shm_errhandler(Display * d, XErrorEvent * e)
{
    if (e->error_code == BadAccess) {
        shm_error = True;
        return (0);
    } else {
        return (X_handler(d, e));
    }
}
#endif /* ! NO_SHARED_MEMORY */

static void
UpdateYUVTextureData(SDL_Texture * texture)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;
    SDL_Rect rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    SDL_SW_CopyYUVToRGB(data->yuv, &rect, data->format, texture->w,
                        texture->h, data->pixels, data->pitch);
}

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
static SDL_bool
CheckXRender(Display *display, int *major, int *minor)
{
    const char *env;

    *major = *minor = 0;

    env = SDL_getenv("SDL_VIDEO_X11_XRENDER");

    if (env && !SDL_atoi(env)) {
        return SDL_FALSE;
    }

    if (!SDL_X11_HAVE_XRENDER) {
        return SDL_FALSE;
    }

    if (!XRenderQueryVersion(display, major, minor)) {
        return SDL_FALSE;
    }

    if (*major != 0 || *minor < 10) {
        return SDL_FALSE;
    }

    return SDL_TRUE;
}
#endif

#ifdef SDL_VIDEO_DRIVER_X11_XFIXES
static SDL_bool
CheckXFixes(Display *display, int *major, int *minor)
{
    const char *env;

    *major = *minor = 0;

    env = SDL_getenv("SDL_VIDEO_X11_XFIXES");

    if (env && !SDL_atoi(env)) {
        return SDL_FALSE;
    }

    if (!SDL_X11_HAVE_XFIXES) {
        return SDL_FALSE;
    }

    if (!XFixesQueryVersion(display, major, minor)) {
        return SDL_FALSE;
    }

    if (*major < 2) {
        return SDL_FALSE;
    }

    return SDL_TRUE;
}
#endif

#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
static SDL_bool
CheckXDamage(Display *display, int *major, int *minor)
{
    const char *env;

    *major = *minor = 0;

    env = SDL_getenv("SDL_VIDEO_X11_XDAMAGE");

    if (env && !SDL_atoi(env)) {
        return SDL_FALSE;
    }

    if (!SDL_X11_HAVE_XDAMAGE) {
        return SDL_FALSE;
    }

    if (!XDamageQueryVersion(display, major, minor)) {
        return SDL_FALSE;
    }

    return SDL_TRUE;
}
#endif

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
static Uint32
XRenderPictFormatToSDLPixelFormatEnum(XRenderPictFormat *pict_format)
{
    if (pict_format->type != PictTypeDirect) {
        SDL_SetError("Indexed pict formats not supported ATM");
        return 0;
    }
    Uint32 Amask, Rmask, Gmask, Bmask;
    int bpp;

    Rmask = pict_format->direct.redMask << pict_format->direct.red;
    Gmask = pict_format->direct.greenMask << pict_format->direct.green;
    Bmask = pict_format->direct.blueMask << pict_format->direct.blue;
    Amask = pict_format->direct.alphaMask << pict_format->direct.alpha;
    bpp = pict_format->depth;

    Uint32 format;
    format = SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
    return format;
}
#endif

void
X11_AddRenderDriver(_THIS)
{
    SDL_RendererInfo *info = &X11_RenderDriver.info;
    SDL_DisplayMode *mode = &SDL_CurrentDisplay->desktop_mode;
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int i;

    info->texture_formats[info->num_texture_formats++] = mode->format;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_YV12;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_IYUV;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_YUY2;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_UYVY;
    info->texture_formats[info->num_texture_formats++] = SDL_PIXELFORMAT_YVYU;

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    int major, minor;
    if (CheckXRender(data->display, &major, &minor)) {
        XRenderPictFormat templ;
        templ.type = PictTypeDirect;
        XRenderPictFormat *pict_format;
        Uint32 format;
        int i = 0;
        /* Convert each XRenderPictFormat into appropriate
         * SDLPixelFormatEnum. */
        while (info->num_texture_formats < 50) {
            pict_format =
                XRenderFindFormat(data->display, PictFormatType, &templ, i++);
            if (pict_format) {
                format = XRenderPictFormatToSDLPixelFormatEnum(pict_format);
                if (format != SDL_PIXELTYPE_UNKNOWN) {
                    info->texture_formats[info->num_texture_formats++] = format;
                }
            }
            else
                break;
        }
        /* Update the capabilities of the renderer. */
        info->blend_modes |= (SDL_BLENDMODE_BLEND | SDL_BLENDMODE_ADD |
                             SDL_BLENDMODE_MOD | SDL_BLENDMODE_MASK);
        info->scale_modes |= (SDL_SCALEMODE_FAST | SDL_SCALEMODE_SLOW |
                             SDL_SCALEMODE_BEST);
        info->mod_modes |= (SDL_TEXTUREMODULATE_COLOR | SDL_TEXTUREMODULATE_ALPHA);
    }
#endif

    for (i = 0; i < _this->num_displays; ++i) {
        SDL_AddRenderDriver(&_this->displays[i], &X11_RenderDriver);
    }
}

SDL_Renderer *
X11_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = window->display;
    SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;
    SDL_WindowData *windowdata = (SDL_WindowData *) window->driverdata;
    SDL_Renderer *renderer;
    X11_RenderData *data;
    XGCValues gcv;
    gcv.graphics_exposures = False;
    int i, n;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (X11_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        X11_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    data->display = windowdata->videodata->display;
    data->screen = displaydata->screen;
    data->visual = displaydata->visual;
    data->depth = displaydata->depth;
    data->scanline_pad = displaydata->scanline_pad;
    data->xwindow = windowdata->xwindow;
    
    renderer->DisplayModeChanged = X11_DisplayModeChanged;
    renderer->CreateTexture = X11_CreateTexture;
    renderer->QueryTexturePixels = X11_QueryTexturePixels;
    renderer->SetTextureAlphaMod = X11_SetTextureRGBAMod;
    renderer->SetTextureColorMod = X11_SetTextureRGBAMod;
    renderer->SetTextureBlendMode = X11_SetTextureBlendMode;
    renderer->SetTextureScaleMode = X11_SetTextureScaleMode;
    renderer->UpdateTexture = X11_UpdateTexture;
    renderer->LockTexture = X11_LockTexture;
    renderer->UnlockTexture = X11_UnlockTexture;
    renderer->SetDrawBlendMode = X11_SetDrawBlendMode;
    renderer->RenderDrawPoints = X11_RenderDrawPoints;
    renderer->RenderDrawLines = X11_RenderDrawLines;
    renderer->RenderDrawRects = X11_RenderDrawRects;
    renderer->RenderFillRects = X11_RenderFillRects;
    renderer->RenderCopy = X11_RenderCopy;
    renderer->RenderReadPixels = X11_RenderReadPixels;
    renderer->RenderWritePixels = X11_RenderWritePixels;
    renderer->RenderPresent = X11_RenderPresent;
    renderer->DestroyTexture = X11_DestroyTexture;
    renderer->DestroyRenderer = X11_DestroyRenderer;
    renderer->info = X11_RenderDriver.info;
    renderer->window = window;
    renderer->driverdata = data;

    renderer->info.flags = SDL_RENDERER_ACCELERATED;

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    int major, minor;
    data->use_xrender = CheckXRender(data->display, &major, &minor);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
    if (CheckXDamage(data->display, &major, &minor)) {
        if (CheckXFixes(data->display, &major, &minor)) {
            data->use_xdamage = SDL_TRUE;
        }
    }
#endif
    if (data->use_xrender) {
        /* Find the PictFormat from the visual.
         * Should be an RGB PictFormat most of the time. */
        data->xwindow_pict_fmt = XRenderFindVisualFormat(data->display,
                                                         data->visual);
        if (!data->xwindow_pict_fmt) {
            SDL_SetError("XRenderFindVisualFormat() failed");
            return NULL;
        }
        data->xwindow_pict = XRenderCreatePicture(data->display,
                                                  data->xwindow,
                                                  data->xwindow_pict_fmt,
                                                  0, NULL);
        if (!data->xwindow_pict) {
            SDL_SetError("XRenderCreatePicture() failed");
            return NULL;
        }
        // FIXME: Clear the window. Is this required?
        XRenderComposite(data->display,
                         PictOpClear,
                         data->xwindow_pict,
                         None,
                         data->xwindow_pict,
                         0, 0,
                         0, 0,
                         0, 0,
                         window->w, window->h);
        /* Create a clip mask that is used for rendering primitives. */
        data->stencil = XCreatePixmap(data->display, data->xwindow,
                                   window->w, window->h, 32);
        if (!data->stencil) {
            SDL_SetError("XCreatePixmap() failed.");
            return NULL;
        }
        
        /* Create the GC for the clip mask. */
        data->stencil_gc = XCreateGC(data->display, data->stencil,
                                  GCGraphicsExposures, &gcv);
        /* Set the GC parameters. */
        XSetBackground(data->display, data->stencil_gc, 0);
        XSetForeground(data->display, data->stencil_gc, 0);
        XFillRectangle(data->display, data->stencil, data->stencil_gc,
                       0, 0, window->w, window->h);
        XSetForeground(data->display, data->stencil_gc, 0xFFFFFFFF);

        /* Create an XRender Picture for the clip mask. */
        data->stencil_pict =
            XRenderCreatePicture(data->display, data->stencil,
                                 XRenderFindStandardFormat(data->display,
                                                           PictStandardARGB32),
                                 0, NULL);
        if (!data->stencil_pict) {
            SDL_SetError("XRenderCreatePicture() failed.");
            return NULL;
        }
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
        if (data->use_xdamage) {
            data->stencil_damage =
                XDamageCreate(data->display, data->stencil, XDamageReportNonEmpty);
            if (!data->stencil_damage) {
                SDL_SetError("XDamageCreate() failed.");
                return NULL;
            }
            XDamageSubtract(data->display, data->stencil_damage, None, data->stencil_parts);
        }
#endif
        /* Create a brush pixmap for the color being
         * drawn at any given time. */
        data->brush =
            XCreatePixmap(data->display, data->xwindow, 1, 1, 32);
        if (!data->brush) {
            SDL_SetError("XCreatePixmap() failed.");
            return NULL;
        }

        /* Set some parameters for the brush. */
        XRenderPictureAttributes brush_attr;
        brush_attr.repeat = RepeatNormal;
        /* Create an XRender Picture for the brush
         * with the above parameters. */
        data->brush_pict =
            XRenderCreatePicture(data->display, data->brush,
                                 XRenderFindStandardFormat(data->display,
                                                           PictStandardARGB32),
                                 CPRepeat, &brush_attr);
        if (!data->brush_pict) {
            SDL_SetError("XRenderCreatePicture() failed.");
            return NULL;
        }
        // FIXME: Is the following necessary?
        /* Set the default blending mode. */
        renderer->blendMode = SDL_BLENDMODE_BLEND;
        data->blend_op = PictOpOver;
    }
#endif
    if (flags & SDL_RENDERER_SINGLEBUFFER) {
        renderer->info.flags |=
            (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY);
        n = 0;
    } else if (flags & SDL_RENDERER_PRESENTFLIP2) {
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP2;
        n = 2;
    } else if (flags & SDL_RENDERER_PRESENTFLIP3) {
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP3;
        n = 3;
    } else {
        renderer->info.flags |= SDL_RENDERER_PRESENTCOPY;
        n = 1;
    }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if (data->use_xrender) {
        if (n > 0)
            data->drawable_pict_fmt =
                XRenderFindStandardFormat(data->display, PictStandardARGB32);
        else
            data->drawable_pict_fmt = data->xwindow_pict_fmt;
    }
#endif
    for (i = 0; i < n; ++i) {
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (data->use_xrender) {
            data->pixmaps[i] = XCreatePixmap(data->display,
                                             data->xwindow,
                                             window->w,
                                             window->h,
                                             32);
        }
        else
#endif
        {
            data->pixmaps[i] =
                XCreatePixmap(data->display, data->xwindow, window->w, window->h,
                              displaydata->depth);
        }
        if (!data->pixmaps[i]) {
            X11_DestroyRenderer(renderer);
            SDL_SetError("XCreatePixmap() failed");
            return NULL;
        }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (data->use_xrender) {
            /* Create XRender pictures for each of the pixmaps
             * and clear the pixmaps. */
            data->pixmap_picts[i] = 
                XRenderCreatePicture(data->display,
                                     data->pixmaps[i],
                                     XRenderFindStandardFormat(data->display,
                                                               PictStandardARGB32),
                                     0, None);
            if (!data->pixmap_picts[i]) {
                SDL_SetError("XRenderCreatePicture() failed");
                return NULL;
            }

            XRenderComposite(data->display,
                             PictOpClear,
                             data->pixmap_picts[i],
                             None,
                             data->pixmap_picts[i],
                             0, 0,
                             0, 0,
                             0, 0,
                             window->w, window->h);
        }
#endif
    }
    if (n > 0) {
        data->drawable = data->pixmaps[0];
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if(data->use_xrender == SDL_TRUE)
            data->drawable_pict = data->pixmap_picts[0];
#endif
        data->makedirty = SDL_TRUE;
    } else {
        data->drawable = data->xwindow;
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if(data->use_xrender == SDL_TRUE)
            data->drawable_pict = data->xwindow_pict;
#endif
        data->makedirty = SDL_FALSE;
    }
    data->current_pixmap = 0;

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    /* When using XRender the drawable format
     * is not the same as the screen format. */
    if (data->use_xrender) {
        bpp = data->drawable_pict_fmt->depth;
        Rmask = ((data->drawable_pict_fmt->direct.redMask)
                    << (data->drawable_pict_fmt->direct.red));
        Gmask = ((data->drawable_pict_fmt->direct.greenMask)
                    << (data->drawable_pict_fmt->direct.green));
        Bmask = ((data->drawable_pict_fmt->direct.blueMask)
                    << (data->drawable_pict_fmt->direct.blue));
        Amask = ((data->drawable_pict_fmt->direct.alphaMask)
                    << (data->drawable_pict_fmt->direct.alpha));
    }
    else
#endif
    {
        /* Get the format of the window */
        if (!SDL_PixelFormatEnumToMasks
            (display->current_mode.format, &bpp, &Rmask, &Gmask, &Bmask,
             &Amask)) {
            SDL_SetError("Unknown display format");
            X11_DestroyRenderer(renderer);
            return NULL;
        }
    }
    SDL_InitFormat(&data->format, bpp, Rmask, Gmask, Bmask, Amask);

    /* Create the drawing context */
    gcv.graphics_exposures = False;
    data->gc =
        XCreateGC(data->display, data->drawable, GCGraphicsExposures, &gcv);
    if (!data->gc) {
        X11_DestroyRenderer(renderer);
        SDL_SetError("XCreateGC() failed");
        return NULL;
    }

    return renderer;
}

static int
X11_DisplayModeChanged(SDL_Renderer * renderer)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    int i, n;

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if (data->use_xrender) {
        XRenderFreePicture(data->display, data->xwindow_pict);
        
        data->xwindow_pict_fmt =
            XRenderFindVisualFormat(data->display, data->visual);
        if (!data->xwindow_pict_fmt) {
            SDL_SetError("XRenderFindVisualFormat() failed.");
            return -1;
        }

        data->xwindow_pict =
            XRenderCreatePicture(data->display, data->xwindow,
                                 data->xwindow_pict_fmt, 0, NULL);
        if (!data->xwindow_pict) {
            SDL_SetError("XRenderCreatePicture() failed.");
            return -1;
        }

        XRenderComposite(data->display,
                         PictOpClear,
                         data->xwindow_pict,
                         None,
                         data->xwindow_pict,
                         0, 0,
                         0, 0,
                         0, 0,
                         window->w, window->h);
        
        XFreePixmap(data->display, data->stencil);
        data->stencil = XCreatePixmap(data->display, data->xwindow,
                                   window->w, window->h, 32);
        if (!data->stencil) {
            SDL_SetError("XCreatePixmap() failed.");
            return -1;
        }

        XRenderFreePicture(data->display, data->stencil_pict);
        data->stencil_pict =
            XRenderCreatePicture(data->display, data->stencil,
                                 XRenderFindStandardFormat(data->display,
                                                           PictStandardARGB32),
                                 0, NULL);
        if (!data->stencil_pict) {
            SDL_SetError("XRenderCreatePicture() failed.");
            return -1;
        }
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
        if (data->use_xdamage) {
            XDamageDestroy(data->display, data->stencil_damage);
            data->stencil_damage =
                XDamageCreate(data->display, data->stencil, XDamageReportNonEmpty);
            if (!data->stencil_damage) {
                SDL_SetError("XDamageCreate() failed.");
                return -1;
            }
            XDamageSubtract(data->display, data->stencil_damage, None, data->stencil_parts);
        }
#endif
    }
#endif
    if (renderer->info.flags & SDL_RENDERER_SINGLEBUFFER) {
        n = 0;
    } else if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP2) {
        n = 2;
    } else if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP3) {
        n = 3;
    } else {
        n = 1;
    }
    for (i = 0; i < n; ++i) {
        if (data->pixmaps[i] != None) {
            XFreePixmap(data->display, data->pixmaps[i]);
            data->pixmaps[i] = None;
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
            XRenderFreePicture(data->display, data->pixmap_picts[i]);
            data->pixmap_picts[i] = None;
#endif
        }
    }
    for (i = 0; i < n; ++i) {
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (data->use_xrender) {
            data->pixmaps[i] =
                XCreatePixmap(data->display,
                              data->xwindow,
                              window->w,
                              window->h,
                              32);
        }
        else
#endif
        {
            data->pixmaps[i] =
                XCreatePixmap(data->display, data->xwindow, window->w, window->h,
                              data->depth);
        }
        if (data->pixmaps[i] == None) {
            SDL_SetError("XCreatePixmap() failed");
            return -1;
        }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (data->use_xrender) {
            data->pixmap_picts[i] = 
                XRenderCreatePicture(data->display,
                                     data->pixmaps[i],
                                     XRenderFindStandardFormat(data->display,
                                                               PictStandardARGB32),
                                     0, None);
            if (!data->pixmap_picts[i]) {
                SDL_SetError("XRenderCreatePicture() failed");
                return -1;
            }
            XRenderComposite(data->display,
                             PictOpClear,
                             data->pixmap_picts[i],
                             None,
                             data->pixmap_picts[i],
                             0, 0,
                             0, 0,
                             0, 0,
                             window->w, window->h);

       }
#endif
    }
    if (n > 0) {
        data->drawable = data->pixmaps[0];
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        data->drawable_pict = data->pixmap_picts[0];
#endif
    }
    data->current_pixmap = 0;

    return 0;
}

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
static void
SDLMaskToXRenderMask(Uint32 sdl_mask, short *comp, short *compMask)
{
    if (sdl_mask == 0) {
        *comp = 0;
        *compMask = 0;
    } else {
        (*comp) = 0;
        (*compMask) = 0;
        while(!(sdl_mask & 1)) {
            (*comp)++;
            sdl_mask >>= 1;
        }
        while(sdl_mask & 1) {
            (*compMask) = ((*compMask) << 1) | 1;
            sdl_mask >>= 1;
        }
    }
}

static XRenderPictFormat*
PixelFormatEnumToXRenderPictFormat(SDL_Renderer * renderer, Uint32 format)
{
    XRenderPictFormat* pict_fmt = NULL;
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    
    if (data->use_xrender) {

        int bpp;
        Uint32 Amask, Rmask, Gmask, Bmask;
        if(!SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
            SDL_SetError("Unknown pixel format");
            return NULL;
        }
        XRenderPictFormat templ;
        unsigned long mask = (PictFormatType | PictFormatDepth | PictFormatRed |
                              PictFormatRedMask | PictFormatGreen | PictFormatGreenMask |
                              PictFormatBlue | PictFormatBlueMask | PictFormatAlpha |
                              PictFormatAlphaMask);

        templ.type = PictTypeDirect;
        templ.depth = bpp;
        SDLMaskToXRenderMask(Amask, &(templ.direct.alpha), &(templ.direct.alphaMask));
        SDLMaskToXRenderMask(Rmask, &(templ.direct.red), &(templ.direct.redMask));
        SDLMaskToXRenderMask(Gmask, &(templ.direct.green), &(templ.direct.greenMask));
        SDLMaskToXRenderMask(Bmask, &(templ.direct.blue), &(templ.direct.blueMask));
        pict_fmt = XRenderFindFormat(data->display, mask, &templ, 0);
    }
    
    return pict_fmt;
}

static Visual*
PixelFormatEnumToVisual(SDL_Renderer * renderer, Uint32 format)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;

    if (data->use_xrender) {
        int bpp;
        Uint32 Amask, Rmask, Gmask, Bmask;
        SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);

        XVisualInfo vinfo_templ;
        long vinfo_mask;
        int nitems_return;

        vinfo_mask = (VisualDepthMask | VisualRedMaskMask |
                      VisualGreenMaskMask | VisualBlueMaskMask);
        vinfo_templ.depth = bpp;
        vinfo_templ.red_mask = Rmask;
        vinfo_templ.green_mask = Gmask;
        vinfo_templ.blue_mask = Bmask;

        XVisualInfo * ret = XGetVisualInfo(data->display, vinfo_mask,
                                           &vinfo_templ, &nitems_return);
        
        if (nitems_return) {
            return ret[0].visual;
        }
    }

    return NULL;
}

static XRenderColor
SDLColorToXRenderColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    double rd, gd, bd, ad;
    XRenderColor ret;
    rd = r / 255.0;
    gd = g / 255.0;
    bd = b / 255.0;
    ad = a / 255.0;
    
    ret.red = (unsigned short) (rd * ad * 0xFFFF);
    ret.green = (unsigned short) (gd * ad * 0xFFFF);
    ret.blue = (unsigned short) (bd * ad * 0xFFFF);
    ret.alpha = (unsigned short) (ad * 0xFFFF);

    return ret;
}
#endif

static int
X11_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    X11_TextureData *data;
    int pitch_alignmask = ((renderdata->scanline_pad / 8) - 1);
    XGCValues gcv;
    
    data = (X11_TextureData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    data->depth = renderdata->depth;
    data->visual = renderdata->visual;
    data->gc = renderdata->gc;

    texture->driverdata = data;
    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        data->yuv =
            SDL_SW_CreateYUVTexture(texture->format, texture->w, texture->h);
        if (!data->yuv) {
            return -1;
        }
        data->format = display->current_mode.format;
    } else {
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (renderdata->use_xrender)
        {
            Uint32 Amask, Rmask, Gmask, Bmask;
            SDL_PixelFormatEnumToMasks(texture->format, &(data->depth),
                                       &Rmask, &Gmask, &Bmask, &Amask);
            data->visual = PixelFormatEnumToVisual(renderer, texture->format);
        }
        else
#endif
        {
            if (texture->format != display->current_mode.format)
            {
                SDL_SetError("Texture format doesn't match window format");
                return -1;
            }
        }
        data->format = texture->format;
    }
    
    if (data->yuv || texture->access == SDL_TEXTUREACCESS_STREAMING) {
#ifndef NO_SHARED_MEMORY
        XShmSegmentInfo *shminfo = &data->shminfo;

        shm_error = True;

        if (SDL_X11_HAVE_SHM) {
            data->image =
                XShmCreateImage(renderdata->display, data->visual,
                                data->depth, ZPixmap, NULL,
                                shminfo, texture->w, texture->h);
            if (data->image) {
                shminfo->shmid =
                    shmget(IPC_PRIVATE, texture->h * data->image->bytes_per_line,
                           IPC_CREAT | 0777);
                if (shminfo->shmid >= 0) {
                    shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);
                    shminfo->readOnly = False;
                    if (shminfo->shmaddr != (char *) -1) {
                        shm_error = False;
                        X_handler = XSetErrorHandler(shm_errhandler);
                        XShmAttach(renderdata->display, shminfo);
                        XSync(renderdata->display, False);
                        XSetErrorHandler(X_handler);
                        if (shm_error) {
                            XShmDetach(renderdata->display, shminfo);
                            shmdt(shminfo->shmaddr);
                            XDestroyImage(data->image);
                            XSync(renderdata->display, False);
                        }
                        else {
                            data->pixels = data->image->data = shminfo->shmaddr;
                            shmctl(shminfo->shmid, IPC_RMID, NULL);
                            data->pixmap =
                                XCreatePixmap(renderdata->display, renderdata->xwindow,
                                              texture->w, texture->h, data->depth);
                            if (!data->pixmap) {
                                SDL_SetError("XCreatePixmap() failed");
                                return -1;
                            }
                        }
                    }
                }
            }
        }
        if (shm_error) {
            shminfo->shmaddr = NULL;
        }
        if (!data->image)
#endif /* not NO_SHARED_MEMORY */
        {
            data->image =
                XCreateImage(renderdata->display, data->visual,
                             data->depth, ZPixmap, 0, NULL,
                             texture->w, texture->h,
                             SDL_BYTESPERPIXEL(data->format) * 8,
                             0);
            if (!data->image) {
                X11_DestroyTexture(renderer, texture);
                SDL_SetError("XCreateImage() failed");
                return -1;
            }
            data->pixels = SDL_malloc(texture->h * data->image->bytes_per_line);
            if (!data->pixels) {
                X11_DestroyTexture(renderer, texture);
                SDL_OutOfMemory();
                return -1;
            }
            data->image->data = data->pixels;
            data->pixmap =
                XCreatePixmap(renderdata->display, renderdata->xwindow, texture->w,
                              texture->h, data->depth);
            if (data->pixmap == None) {
                X11_DestroyTexture(renderer, texture);
                SDL_SetError("XCreatePixmap() failed");
                return -1;
            }
        }
    } else {
        data->image =
            XCreateImage(renderdata->display, data->visual,
                         data->depth, ZPixmap, 0, NULL,
                         texture->w, texture->h,
                         SDL_BYTESPERPIXEL(data->format) * 8,
                         0);
        if (!data->image) {
            X11_DestroyTexture(renderer, texture);
            SDL_SetError("XCreateImage() failed");
            return -1;
        }
        data->pixmap =
            XCreatePixmap(renderdata->display, renderdata->xwindow, texture->w,
                          texture->h, data->depth);
        if (data->pixmap == None) {
            X11_DestroyTexture(renderer, texture);
            SDL_SetError("XCreatePixmap() failed");
            return -1;
        }
    }

    data->pitch = data->image->bytes_per_line;

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if(renderdata->use_xrender && !data->yuv) {
        gcv.graphics_exposures = False;
        data->gc =
            XCreateGC(renderdata->display, data->pixmap, GCGraphicsExposures, &gcv);
        if (!data->gc) {
            SDL_SetError("XCreateGC() failed");
            return -1;
        }
        data->picture_fmt =
            PixelFormatEnumToXRenderPictFormat(renderer, texture->format);
        if (data->picture_fmt == NULL) {
            X11_DestroyTexture(renderer, texture);
            SDL_SetError("Texture format not supported by driver");
            return -1;
        }
        data->picture =
            XRenderCreatePicture(renderdata->display, data->pixmap,
                                 data->picture_fmt, 0, NULL);
        if (!data->picture) {
            X11_DestroyTexture(renderer, texture);
            SDL_SetError("XRenderCreatePicture() failed");
            return -1;
        }
        data->modulated_pixmap =
            XCreatePixmap(renderdata->display, renderdata->xwindow,
                          texture->w, texture->h, data->depth);
        if (!data->modulated_pixmap) {
            X11_DestroyTexture(renderer, texture);
            SDL_SetError("XCreatePixmap() failed");
            return -1;
        }
        data->modulated_picture =
            XRenderCreatePicture(renderdata->display, data->modulated_pixmap,
                                 data->picture_fmt, 0, NULL);
        if (!data->modulated_picture) {
            X11_DestroyTexture(renderer, texture);
            SDL_SetError("XRenderCreatePicture() failed");
            return -1;
        }
        // FIXME: Is the following required?
        /* Set the default blending and scaling modes. */
        texture->blendMode = SDL_BLENDMODE_NONE;
        texture->scaleMode = SDL_SCALEMODE_NONE;
        data->blend_op = PictOpSrc;
        data->filter = NULL;
    }
#endif
    return 0;
}

static int
X11_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                       void **pixels, int *pitch)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (data->yuv) {
        return SDL_SW_QueryYUVTexturePixels(data->yuv, pixels, pitch);
    } else {
        *pixels = data->pixels;
        *pitch = data->pitch;
        return 0;
    }
}

static int
X11_SetTextureRGBAMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;
    X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;

    if (renderdata->use_xrender) {

        Uint8 r = 0xFF, g = 0xFF, b = 0xFF, a = 0xFF;

        /* Check if alpha modulation is required. */
        if (texture->modMode & SDL_TEXTUREMODULATE_ALPHA) {
            a = texture->a;
        }

        /* Check if color modulation is required. */
        if (texture->modMode & SDL_TEXTUREMODULATE_COLOR) {
            r = texture->r;
            g = texture->g;
            b = texture->b;
        }

        /* We can save some labour if no modulation is required. */
        if (texture->modMode != SDL_TEXTUREMODULATE_NONE) {
            XRenderColor mod_color =
                SDLColorToXRenderColor(r, g, b, a);
            XRenderFillRectangle(renderdata->display, PictOpSrc,
                                 renderdata->brush_pict, &mod_color,
                                 0, 0, 1, 1);
        }

        /* We can save some labour dealing with component alpha
         * if color modulation is not required. */
        XRenderPictureAttributes attr;
        if (texture->modMode & SDL_TEXTUREMODULATE_COLOR) {
            attr.component_alpha = True;
            XRenderChangePicture(renderdata->display, renderdata->brush_pict,
                                 CPComponentAlpha, &attr);
        }

        /* Again none of this is necessary is no modulation
         * is required. */
        if (texture->modMode != SDL_TEXTUREMODULATE_NONE) {
            XRenderComposite(renderdata->display, PictOpSrc,
                             data->picture, renderdata->brush_pict,
                             data->modulated_picture,
                             0, 0, 0, 0, 0, 0, texture->w, texture->h);
        }

        /* We only need to reset the component alpha
         * attribute if color modulation is required. */
        if (texture->modMode & SDL_TEXTUREMODULATE_COLOR) { 
            attr.component_alpha = False;
            XRenderChangePicture(renderdata->display, renderdata->brush_pict,
                                 CPComponentAlpha, &attr);
        }

        return 0;
    } else {
        SDL_Unsupported();
        return -1;
    }
#else
    SDL_Unsupported();
    return -1;
#endif
}

static int
X11_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;
    X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;
    switch (texture->blendMode) {
    case SDL_BLENDMODE_NONE:
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (renderdata->use_xrender) {
            data->blend_op = PictOpSrc;
            return 0;
        }
    case SDL_BLENDMODE_MOD:
    case SDL_BLENDMODE_MASK:
    case SDL_BLENDMODE_BLEND:
        if (renderdata->use_xrender) {
            data->blend_op = PictOpOver;
            return 0;
        }
    case SDL_BLENDMODE_ADD:
        if (renderdata->use_xrender) {
            data->blend_op = PictOpAdd;
            return 0;
        }
#endif
        return 0;
    default:
        SDL_Unsupported();
        texture->blendMode = SDL_BLENDMODE_NONE;
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (renderdata->use_xrender) {
            texture->blendMode = SDL_BLENDMODE_BLEND;
            data->blend_op = PictOpOver;
        }
#endif
        return -1;
    }
}

static int
X11_SetTextureScaleMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;
    X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;

    switch (texture->scaleMode) {
    case SDL_SCALEMODE_NONE:
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (renderdata->use_xrender) {
            data->filter = NULL;
        }
#endif
        return 0;
    case SDL_SCALEMODE_FAST:
        /* We can sort of fake it for streaming textures */
        if (data->yuv || texture->access == SDL_TEXTUREACCESS_STREAMING) {
            return 0;
        }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (renderdata->use_xrender) {
            data->filter = FilterFast;
            return 0;
        }
    case SDL_SCALEMODE_SLOW:
        if (renderdata->use_xrender) {
            data->filter = FilterGood;
            return 0;
        }
    case SDL_SCALEMODE_BEST:
        if (renderdata->use_xrender) {
            data->filter = FilterBest;
            return 0;
        }
#endif
    /* Fall through to unsupported case */
    default:
        SDL_Unsupported();
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (renderdata->use_xrender) {
            texture->scaleMode = SDL_SCALEMODE_NONE;
            data->filter = NULL;
        }
        else
#endif
            texture->scaleMode = SDL_SCALEMODE_NONE;
        return -1;
    }
    return 0;
}

static int
X11_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * rect, const void *pixels, int pitch)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (data->yuv) {
        if (SDL_SW_UpdateYUVTexture(data->yuv, rect, pixels, pitch) < 0) {
            return -1;
        }
        UpdateYUVTextureData(texture);
        return 0;
    } else {
        X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;

        if (data->pixels) {
            Uint8 *src, *dst;
            int row;
            size_t length;

            src = (Uint8 *) pixels;
            dst =
                (Uint8 *) data->pixels + rect->y * data->pitch +
                rect->x * SDL_BYTESPERPIXEL(texture->format);
            length = rect->w * SDL_BYTESPERPIXEL(texture->format);
            for (row = 0; row < rect->h; ++row) {
                SDL_memcpy(dst, src, length);
                src += pitch;
                dst += data->pitch;
            }
        } else {
            data->image->width = rect->w;
            data->image->height = rect->h;
            data->image->data = (char *) pixels;
            data->image->bytes_per_line = pitch;
            XPutImage(renderdata->display, data->pixmap, data->gc,
                      data->image, 0, 0, rect->x, rect->y, rect->w, rect->h);
        }
        return 0;
    }
}

static int
X11_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * rect, int markDirty, void **pixels,
                int *pitch)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (data->yuv) {
        return SDL_SW_LockYUVTexture(data->yuv, rect, markDirty, pixels,
                                     pitch);
    } else if (data->pixels) {
        *pixels =
            (void *) ((Uint8 *) data->pixels + rect->y * data->pitch +
                      rect->x * SDL_BYTESPERPIXEL(texture->format));
        *pitch = data->pitch;
        return 0;
    } else {
        SDL_SetError("No pixels available");
        return -1;
    }
}

static void
X11_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (data->yuv) {
        SDL_SW_UnlockYUVTexture(data->yuv);
        UpdateYUVTextureData(texture);
    }
}

static int
X11_SetDrawBlendMode(SDL_Renderer * renderer)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    switch (renderer->blendMode) {
    case SDL_BLENDMODE_NONE:
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        //PictOpSrc
        data->blend_op = PictOpSrc;
        return 0;
    case SDL_BLENDMODE_MOD:
    case SDL_BLENDMODE_MASK:
    case SDL_BLENDMODE_BLEND: // PictOpOver
        data->blend_op = PictOpOver;
        return 0;
    case SDL_BLENDMODE_ADD: // PictOpAdd
        data->blend_op = PictOpAdd;
        return 0;
    /* FIXME case SDL_BLENDMODE_MOD: */
#endif
        return 0;
    default:
        SDL_Unsupported();
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if(data->use_xrender) {
            renderer->blendMode = SDL_BLENDMODE_BLEND;
            data->blend_op = PictOpOver;
        }
        else
#endif
        {
            renderer->blendMode = SDL_BLENDMODE_NONE;
        }
        return -1;
    }
}

static Uint32
renderdrawcolor(SDL_Renderer * renderer, int premult)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    Uint8 r = renderer->r;
    Uint8 g = renderer->g;
    Uint8 b = renderer->b;
    Uint8 a = renderer->a;
    if (premult)
        return SDL_MapRGBA(&data->format, ((int) r * (int) a) / 255,
                           ((int) g * (int) a) / 255,
                           ((int) b * (int) a) / 255, 255);
    else
        return SDL_MapRGBA(&data->format, r, g, b, a);
}

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
static XRenderColor
xrenderdrawcolor(SDL_Renderer *renderer)
{
    XRenderColor xrender_color;
    if(renderer->blendMode == SDL_BLENDMODE_NONE) {
        xrender_color =
            SDLColorToXRenderColor(renderer->r, renderer->g, renderer->b, 0xFF);
    }
    else {
        xrender_color =
            SDLColorToXRenderColor(renderer->r, renderer->g, renderer->b, renderer->a);
    }
    return xrender_color;
}
#endif

static int
X11_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                     int count)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    XPoint *xpoints, *xpoint;
    int i, xcount;
    SDL_Rect clip;

    clip.x = 0;
    clip.y = 0;
    clip.w = window->w;
    clip.h = window->h;
    if (data->makedirty) {
        SDL_Rect rect;

        /* Get the smallest rectangle that contains everything */
        rect.x = 0;
        rect.y = 0;
        rect.w = window->w;
        rect.h = window->h;
        if (!SDL_EnclosePoints(points, count, &rect, &rect)) {
            /* Nothing to draw */
            return 0;
        }
        SDL_AddDirtyRect(&data->dirty, &rect);
    }
    {
        xpoint = xpoints = SDL_stack_alloc(XPoint, count);
        xcount = 0;
        for (i = 0; i < count; ++i) {
            int x = points[i].x;
            int y = points[i].y;
            if (x < 0 || x >= window->w || y < 0 || y >= window->h) {
                continue;
            }
            xpoint->x = (short)x;
            xpoint->y = (short)y;
            ++xpoint;
            ++xcount;
        }

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (data->use_xrender &&
            (renderer->blendMode != SDL_BLENDMODE_NONE) &&
            !(renderer->a == 0xFF &&
              renderer->blendMode != SDL_BLENDMODE_ADD &&
              renderer->blendMode != SDL_BLENDMODE_MOD))
        {
            XSetForeground(data->display, data->stencil_gc, 0);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage)
            {
                /* Update only those parts which were changed
                * in the previous drawing operation */
                XFixesSetGCClipRegion(data->display, data->stencil_gc,
                                      0, 0, data->stencil_parts);
            }
#endif
            XFillRectangle(data->display, data->stencil, data->stencil_gc,
                           0, 0, window->w, window->h);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage)
            {
                XFixesSetGCClipRegion(data->display, data->stencil_gc, 0, 0, None);
            }
#endif
            XSetForeground(data->display, data->stencil_gc, 0xFFFFFFFF);

            XDrawPoints(data->display, data->stencil, data->stencil_gc, xpoints, xcount,
                        CoordModeOrigin);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage)
            {
                /* Store the damaged region in stencil_parts */
                XDamageSubtract(data->display, data->stencil_damage, None, data->stencil_parts);
            }
#endif
        }
#endif
    }

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if (data->use_xrender &&
            (renderer->blendMode != SDL_BLENDMODE_NONE) &&
            !(renderer->a == 0xFF &&
              renderer->blendMode != SDL_BLENDMODE_ADD &&
              renderer->blendMode != SDL_BLENDMODE_MOD))
    {
        XRenderColor foreground;
        foreground = xrenderdrawcolor(renderer);

        XRenderFillRectangle(data->display, PictOpSrc, data->brush_pict,
                             &foreground, 0, 0, 1, 1);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
        if (data->use_xdamage)
        {
            /* Update only those parts which drawn
             * to in the current drawing operation */
            XFixesSetPictureClipRegion(data->display, data->drawable_pict,
                                       0, 0, data->stencil_parts);
        }
#endif
        XRenderComposite(data->display, data->blend_op, data->brush_pict,
                         data->stencil_pict, data->drawable_pict,
                         0, 0, 0, 0, 0, 0, window->w, window->h);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
        if (data->use_xdamage)
        {
            XFixesSetPictureClipRegion(data->display, data->drawable_pict, 0, 0, None);
        }
#endif
    }
    else
#endif
    {
        unsigned long foreground = renderdrawcolor(renderer, 1);
        XSetForeground(data->display, data->gc, foreground);


        if (xcount > 0) {
            XDrawPoints(data->display, data->drawable, data->gc, xpoints, xcount,
                        CoordModeOrigin);
        }
    }

    SDL_stack_free(xpoints);

    return 0;
}

static int
X11_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                    int count)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_Rect clip;
    unsigned long foreground;
    XPoint *xpoints, *xpoint;
    int i, xcount;
    int minx, miny;
    int maxx, maxy;

    clip.x = 0;
    clip.y = 0;
    clip.w = window->w;
    clip.h = window->h;
    {
        Pixmap drawable;
        GC gc;
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
       if (data->use_xrender &&
            (renderer->blendMode != SDL_BLENDMODE_NONE) &&
            !(renderer->a == 0xFF &&
              renderer->blendMode != SDL_BLENDMODE_ADD &&
              renderer->blendMode != SDL_BLENDMODE_MOD))
       { 
            drawable = data->stencil;
            gc = data->stencil_gc;

            XSetForeground(data->display, data->stencil_gc, 0);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage)
                XFixesSetGCClipRegion(data->display, data->stencil_gc,
                                      0, 0, data->stencil_parts);
#endif
            XFillRectangle(data->display, data->stencil, data->stencil_gc,
                           0, 0, window->w, window->h);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage)
                XFixesSetGCClipRegion(data->display, data->stencil_gc,
                                      0, 0, None);
#endif
            XSetForeground(data->display, data->stencil_gc, 0xFFFFFFFF);
        }
        else
#endif
        {
            drawable = data->drawable;
            gc = data->gc;
        }

        foreground = renderdrawcolor(renderer, 1);
        XSetForeground(data->display, data->gc, foreground);

        xpoint = xpoints = SDL_stack_alloc(XPoint, count);
        xcount = 0;
        minx = INT_MAX;
        miny = INT_MAX;
        maxx = INT_MIN;
        maxy = INT_MIN;
        for (i = 0; i < count; ++i) {
            int x = points[i].x;
            int y = points[i].y;

            /* If the point is inside the window, add it to the list */
            if (x >= 0 && x < window->w && y >= 0 && y < window->h) {
                if (x < minx) {
                    minx = x;
                } else if (x > maxx) {
                    maxx = x;
                }
                if (y < miny) {
                    miny = y;
                } else if (y > maxy) {
                    maxy = y;
                }
                xpoint->x = (short)x;
                xpoint->y = (short)y;
                ++xpoint;
                ++xcount;
                continue;
            }

            /* We need to clip the line segments joined by this point */
            if (xcount > 0) {
                int x1 = xpoint[-1].x;
                int y1 = xpoint[-1].y;
                int x2 = x;
                int y2 = y;
                if (SDL_IntersectRectAndLine(&clip, &x1, &y1, &x2, &y2)) {
                    if (x2 < minx) {
                        minx = x2;
                    } else if (x2 > maxx) {
                        maxx = x2;
                    }
                    if (y2 < miny) {
                        miny = y2;
                    } else if (y2 > maxy) {
                        maxy = y2;
                    }
                    xpoint->x = (short)x2;
                    xpoint->y = (short)y2;
                    ++xpoint;
                    ++xcount;
                }
                XDrawLines(data->display, drawable, gc,
                           xpoints, xcount, CoordModeOrigin);
                if (xpoints[0].x != x2 || xpoints[0].y != y2) {
                    XDrawPoint(data->display, drawable, gc, x2, y2);
                }
                if (data->makedirty) {
                    SDL_Rect rect;

                    rect.x = minx;
                    rect.y = miny;
                    rect.w = (maxx - minx) + 1;
                    rect.h = (maxy - miny) + 1;
                    SDL_AddDirtyRect(&data->dirty, &rect);
                }
                xpoint = xpoints;
                xcount = 0;
                minx = INT_MAX;
                miny = INT_MAX;
                maxx = INT_MIN;
                maxy = INT_MIN;
            }
            if (i < (count-1)) {
                int x1 = x;
                int y1 = y;
                int x2 = points[i+1].x;
                int y2 = points[i+1].y;
                if (SDL_IntersectRectAndLine(&clip, &x1, &y1, &x2, &y2)) {
                    if (x1 < minx) {
                        minx = x1;
                    } else if (x1 > maxx) {
                        maxx = x1;
                    }
                    if (y1 < miny) {
                        miny = y1;
                    } else if (y1 > maxy) {
                        maxy = y1;
                    }
                    xpoint->x = (short)x1;
                    xpoint->y = (short)y1;
                    ++xpoint;
                    ++xcount;
                }
            }
        }
        if (xcount > 1) {
            int x2 = xpoint[-1].x;
            int y2 = xpoint[-1].y;
            XDrawLines(data->display, drawable, gc, xpoints, xcount,
                       CoordModeOrigin);
            if (xpoints[0].x != x2 || xpoints[0].y != y2) {
                XDrawPoint(data->display, drawable, gc, x2, y2);
            }
            if (data->makedirty) {
                SDL_Rect rect;

                rect.x = minx;
                rect.y = miny;
                rect.w = (maxx - minx) + 1;
                rect.h = (maxy - miny) + 1;
                SDL_AddDirtyRect(&data->dirty, &rect);
            }
        }
    }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if (data->use_xrender &&
            (renderer->blendMode != SDL_BLENDMODE_NONE) &&
            !(renderer->a == 0xFF &&
              renderer->blendMode != SDL_BLENDMODE_ADD &&
              renderer->blendMode != SDL_BLENDMODE_MOD))
    {
        XRenderColor xrforeground = xrenderdrawcolor(renderer);
        XRenderFillRectangle(data->display, PictOpSrc, data->brush_pict,
                             &xrforeground, 0, 0, 1, 1);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
        if (data->use_xdamage)
        {
            XDamageSubtract(data->display, data->stencil_damage, None, data->stencil_parts);

            XFixesSetPictureClipRegion(data->display, data->drawable_pict,
                                       0, 0, data->stencil_parts);
        }
#endif
        XRenderComposite(data->display, data->blend_op, data->brush_pict,
                         data->stencil_pict, data->drawable_pict,
                         0, 0, 0, 0, 0, 0, window->w, window->h);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
        if (data->use_xdamage)
            XFixesSetPictureClipRegion(data->display, data->drawable_pict,
                                       0, 0, None);
#endif
    }
#endif
    SDL_stack_free(xpoints);

    return 0;
}

static int
X11_RenderDrawRects(SDL_Renderer * renderer, const SDL_Rect ** rects, int count)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_Rect clip, rect;
    int i, xcount;
    XRectangle *xrects, *xrect;
    xrect = xrects = SDL_stack_alloc(XRectangle, count);
    xcount = 0;
    
    clip.x = 0;
    clip.y = 0;
    clip.w = window->w;
    clip.h = window->h;
    {

        for (i = 0; i < count; ++i) {
            if (!SDL_IntersectRect(rects[i], &clip, &rect)) {
                continue;
            }

            xrect->x = (short)rect.x;
            xrect->y = (short)rect.y;
            xrect->width = (unsigned short)rect.w - 1;
            xrect->height = (unsigned short)rect.h - 1;
            ++xrect;
            ++xcount;

            if (data->makedirty) {
                SDL_AddDirtyRect(&data->dirty, &rect);
            }
        }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (data->use_xrender &&
            (renderer->blendMode != SDL_BLENDMODE_NONE) &&
            !(renderer->a == 0xFF &&
              renderer->blendMode != SDL_BLENDMODE_ADD &&
              renderer->blendMode != SDL_BLENDMODE_MOD))
        {
            XSetForeground(data->display, data->stencil_gc, 0);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage)
                XFixesSetGCClipRegion(data->display, data->stencil_gc,
                                      0, 0, data->stencil_parts);
#endif
            XFillRectangle(data->display, data->stencil, data->stencil_gc,
                           0, 0, window->w, window->h);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage)
                XFixesSetGCClipRegion(data->display, data->stencil_gc,
                                      0, 0, None);
#endif
            XSetForeground(data->display, data->stencil_gc, 0xFFFFFFFF);

            XDrawRectangles(data->display, data->stencil, data->stencil_gc, xrects, xcount);

#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage)
                XDamageSubtract(data->display, data->stencil_damage,
                                None, data->stencil_parts);
#endif
        }
#endif
    }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if (data->use_xrender &&
            (renderer->blendMode != SDL_BLENDMODE_NONE) &&
            !(renderer->a == 0xFF &&
              renderer->blendMode != SDL_BLENDMODE_ADD &&
              renderer->blendMode != SDL_BLENDMODE_MOD))
    {
        XRenderColor foreground;
        foreground = xrenderdrawcolor(renderer);
        XRenderFillRectangle(data->display, PictOpSrc, data->brush_pict,
                             &foreground, 0, 0, 1, 1);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
        if (data->use_xdamage)
            XFixesSetPictureClipRegion(data->display, data->drawable_pict,
                                       0, 0, data->stencil_parts);
#endif
        XRenderComposite(data->display, data->blend_op, data->brush_pict,
                         data->stencil_pict, data->drawable_pict,
                         0, 0, 0, 0, 0, 0, window->w, window->h);
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
        if (data->use_xdamage)
            XFixesSetPictureClipRegion(data->display, data->drawable_pict,
                                       0, 0, None);
#endif
    }
    else
#endif
    {
        unsigned long foreground;
        
        foreground = renderdrawcolor(renderer, 1);
        XSetForeground(data->display, data->gc, foreground);
    
        if (xcount > 0) {
            XDrawRectangles(data->display, data->drawable, data->gc,
                            xrects, xcount);
        }
    }
    SDL_stack_free(xrects);

    return 0;
}

static int
X11_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects, int count)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_Rect clip, rect;
    
    clip.x = 0;
    clip.y = 0;
    clip.w = window->w;
    clip.h = window->h;
    
    int i, xcount;
    XRectangle *xrects, *xrect;
    xrect = xrects = SDL_stack_alloc(XRectangle, count);
    xcount = 0;
    for (i = 0; i < count; ++i) {
        if (!SDL_IntersectRect(rects[i], &clip, &rect)) {
            continue;
        }

        xrect->x = (short)rect.x;
        xrect->y = (short)rect.y;
        xrect->width = (unsigned short)rect.w;
        xrect->height = (unsigned short)rect.h;
        ++xrect;
        ++xcount;

        if (data->makedirty) {
            SDL_AddDirtyRect(&data->dirty, &rect);
        }
    }

#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if (data->use_xrender) {
        XRenderColor foreground;
        foreground = xrenderdrawcolor(renderer);
        if (xcount == 1) {
            XRenderFillRectangle(data->display, data->blend_op, data->drawable_pict,
                                 &foreground, xrects[0].x, xrects[0].y,
                                 xrects[0].width, xrects[0].height);
        }
        else if (xcount > 1) {
            XRenderFillRectangles(data->display, data->blend_op, data->drawable_pict,
                                  &foreground, xrects, xcount);
        }
    }
    else
#endif
    {
        unsigned long foreground;
        
        foreground = renderdrawcolor(renderer, 1);
        XSetForeground(data->display, data->gc, foreground);
 
        XFillRectangles(data->display, data->drawable, data->gc,
                        xrects, xcount);
    }

    SDL_stack_free(xrects);
    return 0;
}

static int
X11_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    X11_TextureData *texturedata = (X11_TextureData *) texture->driverdata;

    if (data->makedirty) {
        SDL_AddDirtyRect(&data->dirty, dstrect);
    }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if (data->use_xrender && !texturedata->yuv) {
        if(texture->access == SDL_TEXTUREACCESS_STREAMING) {
#ifndef NO_SHARED_MEMORY
            if(texturedata->shminfo.shmaddr) {
                XShmPutImage(data->display, texturedata->pixmap, texturedata->gc,
                             texturedata->image, srcrect->x, srcrect->y,
                             srcrect->x, srcrect->y, srcrect->w, srcrect->h,
                             False);
            }
            else
#endif
            if (texturedata->pixels) {
                XPutImage(data->display, texturedata->pixmap, texturedata->gc,
                          texturedata->image, srcrect->x, srcrect->y, srcrect->x,
                          srcrect->y, srcrect->w, srcrect->h);
            }
            XSync(data->display, False);
        }
        Picture src, mask;
        XRenderPictureAttributes attr;
        const SDL_Rect *mrect;
        /* mrect is the rectangular area of the mask
         * picture that is aligned with the source. */

        if (texture->modMode == SDL_TEXTUREMODULATE_NONE) {
            src = texturedata->picture;
        }
        else {
            src = texturedata->modulated_picture;
        }

        if(texture->blendMode == SDL_BLENDMODE_NONE) 
        {
            mask = None;
            mrect = srcrect;
        }
        else if (texture->blendMode == SDL_BLENDMODE_MOD)
        {
            /* SDL_BLENDMODE_MOD requires a temporary buffer
             * i.e. stencil_pict */
            mask = data->stencil_pict;
            mrect = dstrect;
        }
        else
        {
            /* This trick allows on-the-fly multiplication
             * of the src color channels with it's alpha
             * channel. */
            mask = src;
            mrect = srcrect;
        }

        if(srcrect->w == dstrect->w && srcrect->h == dstrect->h) {
            if (texture->blendMode == SDL_BLENDMODE_MOD) {
                XRenderComposite(data->display, PictOpSrc, data->drawable_pict,
                             src, data->stencil_pict,
                             dstrect->x, dstrect->y, srcrect->x, srcrect->y,
                             dstrect->x, dstrect->y, dstrect->w, dstrect->h);
                attr.component_alpha = True;
                XRenderChangePicture(data->display, data->stencil_pict,
                                     CPComponentAlpha, &attr);
            }
            XRenderComposite(data->display, texturedata->blend_op,
                            src, mask, data->drawable_pict, srcrect->x, srcrect->y,
                            mrect->x, mrect->y, dstrect->x, dstrect->y,
                            dstrect->w, dstrect->h);
        } else {
            /* The transformation is from the dst to src picture. */
            double xscale = ((double) srcrect->w) / dstrect->w;
            double yscale = ((double) srcrect->h) / dstrect->h;
            XTransform xform = {{
                    {XDoubleToFixed(xscale), XDoubleToFixed(0), XDoubleToFixed(0)},
                    {XDoubleToFixed(0), XDoubleToFixed(yscale), XDoubleToFixed(0)},
                    {XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1)}}};
            XRenderSetPictureTransform(data->display, src, &xform);
            
            /* Black magic follows. */
            if (texture->blendMode == SDL_BLENDMODE_MOD) {
                /* Copy the dst to a temp buffer. */
                XRenderComposite(data->display, PictOpSrc, data->drawable_pict,
                             src, data->stencil_pict,
                             dstrect->x, dstrect->y, srcrect->x, srcrect->y,
                             dstrect->x, dstrect->y, dstrect->w, dstrect->h);
                /* Set the compnent alpha flag on the temp buffer. */
                attr.component_alpha = True;
                XRenderChangePicture(data->display, data->stencil_pict,
                                     CPComponentAlpha, &attr);
            }

            /* Set the picture filter only if a scaling mode is set. */
            if (texture->scaleMode != SDL_SCALEMODE_NONE) {
                XRenderSetPictureFilter(data->display, src,
                                        texturedata->filter, 0, 0);
            }

            XRenderComposite(data->display, texturedata->blend_op,
                             src, mask, data->drawable_pict,
                             srcrect->x, srcrect->y, mrect->x, mrect->y,
                             dstrect->x, dstrect->y, dstrect->w, dstrect->h);
           /* Set the texture transformation back to the identity matrix. */ 
            XTransform identity = {{
                    {XDoubleToFixed(1), XDoubleToFixed(0), XDoubleToFixed(0)},
                    {XDoubleToFixed(0), XDoubleToFixed(1), XDoubleToFixed(0)},
                    {XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1)}}};
            XRenderSetPictureTransform(data->display, src, &identity);
        }
        
        /* Reset the component alpha flag only when
         * the blending mode is SDL_BLENDMODE_MOD. */
        if (renderer->blendMode == SDL_BLENDMODE_MOD) {
            attr.component_alpha = False;
            XRenderChangePicture(data->display, data->stencil_pict,
                                 CPComponentAlpha, &attr);
        }
    }
    else
#endif
    {
        if (srcrect->w == dstrect->w && srcrect->h == dstrect->h) {
#ifndef NO_SHARED_MEMORY
            if (texturedata->shminfo.shmaddr) {
                XShmPutImage(data->display, data->drawable, data->gc,
                             texturedata->image, srcrect->x, srcrect->y,
                             dstrect->x, dstrect->y, srcrect->w, srcrect->h,
                             False);
            } else
#endif
            if (texturedata->pixels) {
                XPutImage(data->display, data->drawable, data->gc,
                          texturedata->image, srcrect->x, srcrect->y, dstrect->x,
                          dstrect->y, srcrect->w, srcrect->h);
            } else {
                XCopyArea(data->display, texturedata->pixmap, data->drawable,
                          data->gc, srcrect->x, srcrect->y, dstrect->w,
                          dstrect->h, dstrect->x, dstrect->y);
            }
        } else if (texturedata->yuv
                   || texture->access == SDL_TEXTUREACCESS_STREAMING) {
            SDL_Surface src, dst;
            SDL_PixelFormat fmt;
            SDL_Rect rect;
            XImage *image = texturedata->scaling_image;

            if (!image) {
                void *pixels;
                int pitch;

                pitch = dstrect->w * SDL_BYTESPERPIXEL(texturedata->format);
                pixels = SDL_malloc(dstrect->h * pitch);
                if (!pixels) {
                    SDL_OutOfMemory();
                    return -1;
                }

                image =
                    XCreateImage(data->display, data->visual, data->depth,
                                 ZPixmap, 0, pixels, dstrect->w, dstrect->h,
                                 SDL_BYTESPERPIXEL(texturedata->format) * 8,
                                 pitch);
                if (!image) {
                    SDL_SetError("XCreateImage() failed");
                    return -1;
                }
                texturedata->scaling_image = image;

            } else if (image->width != dstrect->w || image->height != dstrect->h
                       || !image->data) {
                image->width = dstrect->w;
                image->height = dstrect->h;
                image->bytes_per_line =
                    image->width * SDL_BYTESPERPIXEL(texturedata->format);
                image->data =
                    (char *) SDL_realloc(image->data,
                                         image->height * image->bytes_per_line);
                if (!image->data) {
                    SDL_OutOfMemory();
                    return -1;
                }
            }

            /* Set up fake surfaces for SDL_SoftStretch() */
            SDL_zero(src);
            src.format = &fmt;
            src.w = texture->w;
            src.h = texture->h;
#ifndef NO_SHARED_MEMORY
            if (texturedata->shminfo.shmaddr) {
                src.pixels = texturedata->shminfo.shmaddr;
            } else
#endif
                src.pixels = texturedata->pixels;
            src.pitch = texturedata->pitch;

            SDL_zero(dst);
            dst.format = &fmt;
            dst.w = image->width;
            dst.h = image->height;
            dst.pixels = image->data;
            dst.pitch = image->bytes_per_line;

            fmt.BytesPerPixel = SDL_BYTESPERPIXEL(texturedata->format);

            rect.x = 0;
            rect.y = 0;
            rect.w = dstrect->w;
            rect.h = dstrect->h;
            if (SDL_SoftStretch(&src, srcrect, &dst, &rect) < 0) {
                return -1;
            }
            XPutImage(data->display, data->drawable, data->gc, image, 0, 0,
                      dstrect->x, dstrect->y, dstrect->w, dstrect->h);
        } else {
            XCopyArea(data->display, texturedata->pixmap, data->drawable,
                      data->gc, srcrect->x, srcrect->y, dstrect->w, dstrect->h,
                      srcrect->x, srcrect->y);
        }
    }
    return 0;
}

static int
X11_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                     Uint32 format, void * pixels, int pitch)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    Uint32 screen_format = display->current_mode.format;
    XImage *image;

    image = XGetImage(data->display, data->drawable, rect->x, rect->y,
                      rect->w, rect->h, AllPlanes, ZPixmap);

    SDL_ConvertPixels(rect->w, rect->h,
                      screen_format, image->data, image->bytes_per_line,
                      format, pixels, pitch);

    XDestroyImage(image);
    return 0;
}

static int
X11_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                      Uint32 format, const void * pixels, int pitch)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    Uint32 screen_format = display->current_mode.format;
    XImage *image;
    void *image_pixels;
    int image_pitch;

    image_pitch = rect->w * SDL_BYTESPERPIXEL(screen_format);
    image_pixels = SDL_malloc(rect->h * image_pitch);
    if (!image_pixels) {
        SDL_OutOfMemory();
        return -1;
    }

    image = XCreateImage(data->display, data->visual,
                         data->depth, ZPixmap, 0, image_pixels,
                         rect->w, rect->h,
                         SDL_BYTESPERPIXEL(screen_format) * 8,
                         image_pitch);
    if (!image) {
        SDL_SetError("XCreateImage() failed");
        return -1;
    }

    SDL_ConvertPixels(rect->w, rect->h,
                      format, pixels, pitch,
                      screen_format, image->data, image->bytes_per_line);

    XPutImage(data->display, data->drawable, data->gc,
              image, 0, 0, rect->x, rect->y, rect->w, rect->h);

    image->data = NULL;
    XDestroyImage(image);

    SDL_free(image_pixels);
    return 0;
}

static void
X11_RenderPresent(SDL_Renderer * renderer)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    SDL_DirtyRect *dirty;

    /* Send the data to the display */
    if (!(renderer->info.flags & SDL_RENDERER_SINGLEBUFFER)) {
        for (dirty = data->dirty.list; dirty; dirty = dirty->next) {
            const SDL_Rect *rect = &dirty->rect;
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
            if (data->use_xrender)
            {
                XRenderComposite(data->display,
                                 data->blend_op,
                                 data->drawable_pict,
                                 None,
                                 data->xwindow_pict,
                                 rect->x, rect->y,
                                 0, 0,
                                 rect->x, rect->y,
                                 rect->w, rect->h);
            }
            else
#endif
            {
            XCopyArea(data->display, data->drawable, data->xwindow,
                      data->gc, rect->x, rect->y, rect->w, rect->h,
                      rect->x, rect->y);
            }
        }
        SDL_ClearDirtyRects(&data->dirty);
    }
    XSync(data->display, False);

    /* Update the flipping chain, if any */
    if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP2) {
        data->current_pixmap = (data->current_pixmap + 1) % 2;
        data->drawable = data->pixmaps[data->current_pixmap];
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        data->drawable_pict = data->pixmap_picts[data->current_pixmap];
#endif
    } else if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP3) {
        data->current_pixmap = (data->current_pixmap + 1) % 3;
        data->drawable = data->pixmaps[data->current_pixmap];
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        data->drawable_pict = data->pixmap_picts[data->current_pixmap];
#endif
    }
}

static void
X11_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    X11_RenderData *renderdata = (X11_RenderData *) renderer->driverdata;
    X11_TextureData *data = (X11_TextureData *) texture->driverdata;

    if (!data) {
        return;
    }
    if (data->yuv) {
        SDL_SW_DestroyYUVTexture(data->yuv);
    }
    if (data->pixmap != None) {
        XFreePixmap(renderdata->display, data->pixmap);
    }
    if (data->image) {
        data->image->data = NULL;
        XDestroyImage(data->image);
    }
#ifndef NO_SHARED_MEMORY
    if (data->shminfo.shmaddr) {
        XShmDetach(renderdata->display, &data->shminfo);
        XSync(renderdata->display, False);
        shmdt(data->shminfo.shmaddr);
        data->pixels = NULL;
    }
#endif
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
    if (renderdata->use_xrender) {
        if (data->picture) {
            XRenderFreePicture(renderdata->display, data->picture);
        }
        if (data->modulated_pixmap) {
            XFreePixmap(renderdata->display, data->modulated_pixmap);
        }
        if (data->modulated_picture) {
            XRenderFreePicture(renderdata->display, data->modulated_picture);
        }
    }
#endif
    if (data->scaling_image) {
        SDL_free(data->scaling_image->data);
        data->scaling_image->data = NULL;
        XDestroyImage(data->scaling_image);
    }
    if (data->pixels) {
        SDL_free(data->pixels);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
X11_DestroyRenderer(SDL_Renderer * renderer)
{
    X11_RenderData *data = (X11_RenderData *) renderer->driverdata;
    int i;

    if (data) {
        for (i = 0; i < SDL_arraysize(data->pixmaps); ++i) {
            if (data->pixmaps[i] != None) {
                XFreePixmap(data->display, data->pixmaps[i]);
            }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
            if (data->use_xrender && data->pixmap_picts[i]) {
                XRenderFreePicture(data->display, data->pixmap_picts[i]);
            }
#endif
        }
        if (data->gc) {
            XFreeGC(data->display, data->gc);
        }
#ifdef SDL_VIDEO_DRIVER_X11_XRENDER
        if (data->use_xrender) {
            if (data->stencil_gc) {
                XFreeGC(data->display, data->stencil_gc);
            }
            if (data->stencil) {
                XFreePixmap(data->display, data->stencil);
            }
            if (data->stencil_pict) {
                XRenderFreePicture(data->display, data->stencil_pict);
            }
            if (data->xwindow_pict) {
                XRenderFreePicture(data->display, data->xwindow_pict);
            }
#ifdef SDL_VIDEO_DRIVER_X11_XDAMAGE
            if (data->use_xdamage && data->stencil_damage) {
                XDamageDestroy(data->display, data->stencil_damage);
            }
#endif
        }
#endif
        SDL_FreeDirtyRects(&data->dirty);
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_X11 */

/* vi: set ts=4 sw=4 expandtab: */
