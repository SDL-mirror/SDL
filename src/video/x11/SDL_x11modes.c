/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

#include "SDL_x11video.h"


static int
get_visualinfo(Display * display, int screen, XVisualInfo * vinfo)
{
    const char *visual_id = SDL_getenv("SDL_VIDEO_X11_VISUALID");
    int use_directcolor = 1;
    int depth;

    /* Look for an exact visual, if requested */
    if (visual_id) {
        XVisualInfo *vi, template;
        int nvis;

        SDL_zero(template);
        template.visualid = SDL_strtol(visual_id, NULL, 0);
        vi = XGetVisualInfo(display, VisualIDMask, &template, &nvis);
        if (vi) {
            *vinfo = *vi;
            XFree(vi);
            return 0;
        }
    }

    depth = DefaultDepth(display, screen);
    if ((use_directcolor &&
         XMatchVisualInfo(display, screen, depth, DirectColor, vinfo)) ||
        XMatchVisualInfo(display, screen, depth, TrueColor, vinfo) ||
        XMatchVisualInfo(display, screen, depth, PseudoColor, vinfo) ||
        XMatchVisualInfo(display, screen, depth, StaticColor, vinfo)) {
        return 0;
    }
    return -1;
}

static Uint32
X11_GetPixelFormatFromVisualInfo(Display *display, XVisualInfo *vinfo)
{
    if (vinfo->class == DirectColor || vinfo->class == TrueColor) {
        int bpp;
        Uint32 Rmask, Gmask, Bmask, Amask;

        Rmask = vinfo->visual->red_mask;
        Gmask = vinfo->visual->green_mask;
        Bmask = vinfo->visual->blue_mask;
        if (vinfo->depth == 32) {
            Amask = (0xFFFFFFFF & ~(Rmask | Gmask | Bmask));
        } else {
            Amask = 0;
        }

        bpp = vinfo->depth;
        if (bpp == 24) {
            int i, n;
            XPixmapFormatValues *p = XListPixmapFormats(display, &n);
            if (p) {
                for (i = 0; i < n; ++i) {
                    if (p[i].depth == 24) {
                        bpp = p[i].bits_per_pixel;
                        break;
                    }
                }
                XFree(p);
            }
        }

        return SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
    }

    if (vinfo->class == PseudoColor || vinfo->class == StaticColor) {
        switch (vinfo->depth) {
        case 8:
            return SDL_PIXELTYPE_INDEX8;
        case 4:
            if (BitmapBitOrder(display) == LSBFirst) {
                return SDL_PIXELFORMAT_INDEX4LSB;
            } else {
                return SDL_PIXELFORMAT_INDEX4MSB;
            }
            break;
        case 1:
            if (BitmapBitOrder(display) == LSBFirst) {
                return SDL_PIXELFORMAT_INDEX1LSB;
            } else {
                return SDL_PIXELFORMAT_INDEX1MSB;
            }
            break;
        }
    }

    return SDL_PIXELFORMAT_UNKNOWN;
}

void
X11_InitModes(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int screen;

    for (screen = 0; screen < ScreenCount(data->display); ++screen) {
        XVisualInfo vinfo;
        SDL_VideoDisplay display;
        SDL_DisplayData *displaydata;
        SDL_DisplayMode mode;

        if (get_visualinfo(data->display, screen, &vinfo) < 0) {
            continue;
        }

        mode.format = X11_GetPixelFormatFromVisualInfo(data->display, &vinfo);
        mode.w = DisplayWidth(data->display, screen);
        mode.h = DisplayHeight(data->display, screen);
        mode.refresh_rate = 0;
        mode.driverdata = NULL;

        displaydata = (SDL_DisplayData *) SDL_malloc(sizeof(*displaydata));
        if (!displaydata) {
            continue;
        }
        displaydata->screen = screen;
        displaydata->visual = vinfo.visual;
        displaydata->depth = vinfo.depth;

        SDL_zero(display);
        display.desktop_mode = mode;
        display.current_mode = mode;
        display.driverdata = displaydata;
        SDL_AddVideoDisplay(&display);
    }
}

void
X11_GetDisplayModes(_THIS)
{
    Display *display = ((SDL_VideoData *) _this->driverdata)->display;
    SDL_DisplayData *data = (SDL_DisplayData *) SDL_CurrentDisplay.driverdata;
    SDL_DisplayMode mode;

}

int
X11_SetDisplayMode(_THIS, SDL_DisplayMode * mode)
{
    //SDL_DisplayModeData *data = (SDL_DisplayModeData *) mode->driverdata;
    return -1;
}

void
X11_QuitModes(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
