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

#include "SDL_syswm.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"

#include "SDL_x11video.h"
#include "../Xext/extensions/StdCmap.h"

static int
SetupWindowData(_THIS, SDL_Window * window, Window w, BOOL created)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    SDL_WindowData *data;
    int numwindows = videodata->numwindows;
    int windowlistlength = videodata->windowlistlength;
    SDL_WindowData **windowlist = videodata->windowlist;
    int index;

    /* Allocate the window data */
    data = (SDL_WindowData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    data->windowID = window->id;
    data->window = w;
#ifdef X_HAVE_UTF8_STRING
    if (SDL_X11_HAVE_UTF8) {
        data->ic =
            pXCreateIC(videodata->im, XNClientWindow, w, XNFocusWindow, w,
                       XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                       XNResourceName, videodata->classname, XNResourceClass,
                       videodata->classname, NULL);
    }
#endif
    data->created = created;
    data->videodata = videodata;

    /* Associate the data with the window */

    if (numwindows < windowlistlength) {
        windowlist[numwindows] = data;
        videodata->numwindows++;
    } else {
        windowlist =
            (SDL_WindowData **) SDL_realloc(windowlist,
                                            (numwindows +
                                             1) * sizeof(*windowlist));
        if (!windowlist) {
            SDL_OutOfMemory();
            SDL_free(data);
            return -1;
        }
        windowlist[numwindows] = data;
        videodata->numwindows++;
        videodata->windowlistlength++;
        videodata->windowlist = windowlist;
    }

    /* Fill in the SDL window with the window data */
    {
        XWindowAttributes attrib;

        XGetWindowAttributes(data->videodata->display, w, &attrib);
        window->x = attrib.x;
        window->y = attrib.y;
        window->w = attrib.width;
        window->h = attrib.height;
        if (attrib.map_state != IsUnmapped) {
            window->flags |= SDL_WINDOW_SHOWN;
        } else {
            window->flags &= ~SDL_WINDOW_SHOWN;
        }
    }
    /* FIXME: How can I tell?
       {
       DWORD style = GetWindowLong(hwnd, GWL_STYLE);
       if (style & WS_VISIBLE) {
       if (style & (WS_BORDER | WS_THICKFRAME)) {
       window->flags &= ~SDL_WINDOW_BORDERLESS;
       } else {
       window->flags |= SDL_WINDOW_BORDERLESS;
       }
       if (style & WS_THICKFRAME) {
       window->flags |= SDL_WINDOW_RESIZABLE;
       } else {
       window->flags &= ~SDL_WINDOW_RESIZABLE;
       }
       if (style & WS_MAXIMIZE) {
       window->flags |= SDL_WINDOW_MAXIMIZED;
       } else {
       window->flags &= ~SDL_WINDOW_MAXIMIZED;
       }
       if (style & WS_MINIMIZE) {
       window->flags |= SDL_WINDOW_MINIMIZED;
       } else {
       window->flags &= ~SDL_WINDOW_MINIMIZED;
       }
       }
       if (GetFocus() == hwnd) {
       int index = data->videodata->keyboard;
       window->flags |= SDL_WINDOW_INPUT_FOCUS;
       SDL_SetKeyboardFocus(index, data->windowID);

       if (window->flags & SDL_WINDOW_INPUT_GRABBED) {
       RECT rect;
       GetClientRect(hwnd, &rect);
       ClientToScreen(hwnd, (LPPOINT) & rect);
       ClientToScreen(hwnd, (LPPOINT) & rect + 1);
       ClipCursor(&rect);
       }
       }
     */

    /* All done! */
    window->driverdata = data;
    return 0;
}

int
X11_CreateWindow(_THIS, SDL_Window * window)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_DisplayData *displaydata =
        (SDL_DisplayData *) SDL_GetDisplayFromWindow(window)->driverdata;
    Visual *visual;
    int depth;
    XSetWindowAttributes xattr;
    int x, y;
    Window w;
    XSizeHints *sizehints;
    XWMHints *wmhints;
    XClassHint *classhints;

#if SDL_VIDEO_DRIVER_X11_XINERAMA
/* FIXME
    if ( use_xinerama ) {
        x = xinerama_info.x_org;
        y = xinerama_info.y_org;
    }
*/
#endif
#ifdef SDL_VIDEO_OPENGL_GLX
    if (window->flags & SDL_WINDOW_OPENGL) {
        XVisualInfo *vinfo;

        if (X11_GL_Initialize(_this) < 0) {
            return -1;
        }
        vinfo = X11_GL_GetVisual(_this, data->display, displaydata->screen);
        if (!vinfo) {
            return -1;
        }
        visual = vinfo->visual;
        depth = vinfo->depth;
        XFree(vinfo);
    } else
#endif
    {
        visual = displaydata->visual;
        depth = displaydata->depth;
    }

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        xattr.override_redirect = True;
    } else {
        xattr.override_redirect = False;
    }
    xattr.background_pixel = 0;
    xattr.border_pixel = 0;

    if (visual->class == DirectColor || visual->class == PseudoColor) {
        int nmaps;
        XStandardColormap cmap;
        XStandardColormap *stdmaps;
        XColor *colorcells;
        Colormap colormap;
        Bool found = False;
        int i;
        int ncolors;
        int rmax, gmax, bmax;
        int rmul, gmul, bmul;

        if (colormap =
            X11_LookupColormap(data->display, displaydata->screen,
                               visual->visualid)) {
            xattr.colormap = colormap;
        } else {
            /* check to see if the colormap we need already exists */
            if (0 != XGetRGBColormaps(data->display,
                                      RootWindow(data->display,
                                                 displaydata->screen),
                                      &stdmaps, &nmaps, XA_RGB_BEST_MAP)) {
                for (i = 0; i < nmaps; i++) {
                    if (stdmaps[i].visualid == visual->visualid) {
                        SDL_memcpy(&cmap, &stdmaps[i],
                                   sizeof(XStandardColormap));
                        found = True;
                        break;
                    }
                }
                XFree(stdmaps);
            }

            /* it doesn't exist, so create it */
            if (!found) {
                int max = visual->map_entries - 1;
                stdmaps =
                    XmuStandardColormap(data->display, displaydata->screen,
                                        visual->visualid, depth,
                                        XA_RGB_BEST_MAP, None, max, max, max);
                if (NULL == stdmaps || stdmaps->visualid != visual->visualid) {
                    SDL_SetError
                        ("Couldn't create window:XA_RGB_BEST_MAP not found and could not be created");
                    return -1;
                }
                SDL_memcpy(&cmap, stdmaps, sizeof(XStandardColormap));
                XFree(stdmaps);
            }

            /* OK, we have the best color map, now copy it for use by the
               program */

            colorcells = SDL_malloc(visual->map_entries * sizeof(XColor));
            if (NULL == colorcells) {
                SDL_SetError("out of memory in X11_CreateWindow");
                return -1;
            }
            ncolors = visual->map_entries;
            rmax = cmap.red_max + 1;
            gmax = cmap.blue_max + 1;
            bmax = cmap.green_max + 1;

            rmul = cmap.red_mult;
            gmul = cmap.blue_mult;
            bmul = cmap.green_mult;

            /* build the color table pixel values */
            for (i = 0; i < ncolors; i++) {
                Uint32 red = (rmax * i) / ncolors;
                Uint32 green = (gmax * i) / ncolors;
                Uint32 blue = (bmax * i) / ncolors;

                colorcells[i].pixel =
                    (red * rmul) | (green * gmul) | (blue * bmul);
            }
            XQueryColors(data->display, cmap.colormap, colorcells, ncolors);
            colormap = XCreateColormap(data->display,
                                       RootWindow(data->display,
                                                  displaydata->screen),
                                       visual, AllocAll);
            XStoreColors(data->display, colormap, colorcells, ncolors);
            SDL_free(colorcells);

            xattr.colormap = colormap;
            X11_TrackColormap(data->display, displaydata->screen, colormap,
                              &cmap, visual);
        }
    } else {
        xattr.colormap =
            XCreateColormap(data->display,
                            RootWindow(data->display, displaydata->screen),
                            visual, AllocNone);
    }

    if (window->x == SDL_WINDOWPOS_CENTERED) {
        x = (DisplayWidth(data->display, displaydata->screen) -
             window->w) / 2;
    } else if (window->x == SDL_WINDOWPOS_UNDEFINED) {
        x = 0;
    } else {
        x = window->x;
    }
    if (window->y == SDL_WINDOWPOS_CENTERED) {
        y = (DisplayHeight(data->display, displaydata->screen) -
             window->h) / 2;
    } else if (window->y == SDL_WINDOWPOS_UNDEFINED) {
        y = 0;
    } else {
        y = window->y;
    }

    w = XCreateWindow(data->display,
                      RootWindow(data->display, displaydata->screen), x, y,
                      window->w, window->h, 0, depth, InputOutput, visual,
                      (CWOverrideRedirect | CWBackPixel | CWBorderPixel |
                       CWColormap), &xattr);
    if (!w) {
#ifdef SDL_VIDEO_OPENGL_GLX
        if (window->flags & SDL_WINDOW_OPENGL) {
            X11_GL_Shutdown(_this);
        }
#endif
        SDL_SetError("Couldn't create window");
        return -1;
    }

    sizehints = XAllocSizeHints();
    if (sizehints) {
        if (window->flags & SDL_WINDOW_RESIZABLE) {
            sizehints->min_width = 32;
            sizehints->min_height = 32;
            sizehints->max_height = 4096;
            sizehints->max_width = 4096;
        } else {
            sizehints->min_width = sizehints->max_width = window->w;
            sizehints->min_height = sizehints->max_height = window->h;
        }
        sizehints->flags = PMaxSize | PMinSize;
        if (!(window->flags & SDL_WINDOW_FULLSCREEN)
            && window->x != SDL_WINDOWPOS_UNDEFINED
            && window->y != SDL_WINDOWPOS_UNDEFINED) {
            sizehints->x = x;
            sizehints->y = y;
            sizehints->flags |= USPosition;
        }
        XSetWMNormalHints(data->display, w, sizehints);
        XFree(sizehints);
    }

    if (window->flags & SDL_WINDOW_BORDERLESS) {
        SDL_bool set;
        Atom WM_HINTS;

        /* We haven't modified the window manager hints yet */
        set = SDL_FALSE;

        /* First try to set MWM hints */
        WM_HINTS = XInternAtom(data->display, "_MOTIF_WM_HINTS", True);
        if (WM_HINTS != None) {
            /* Hints used by Motif compliant window managers */
            struct
            {
                unsigned long flags;
                unsigned long functions;
                unsigned long decorations;
                long input_mode;
                unsigned long status;
            } MWMHints = {
            (1L << 1), 0, 0, 0, 0};

            XChangeProperty(data->display, w, WM_HINTS, WM_HINTS, 32,
                            PropModeReplace, (unsigned char *) &MWMHints,
                            sizeof(MWMHints) / sizeof(long));
            set = SDL_TRUE;
        }
        /* Now try to set KWM hints */
        WM_HINTS = XInternAtom(data->display, "KWM_WIN_DECORATION", True);
        if (WM_HINTS != None) {
            long KWMHints = 0;

            XChangeProperty(data->display, w,
                            WM_HINTS, WM_HINTS, 32,
                            PropModeReplace,
                            (unsigned char *) &KWMHints,
                            sizeof(KWMHints) / sizeof(long));
            set = SDL_TRUE;
        }
        /* Now try to set GNOME hints */
        WM_HINTS = XInternAtom(data->display, "_WIN_HINTS", True);
        if (WM_HINTS != None) {
            long GNOMEHints = 0;

            XChangeProperty(data->display, w,
                            WM_HINTS, WM_HINTS, 32,
                            PropModeReplace,
                            (unsigned char *) &GNOMEHints,
                            sizeof(GNOMEHints) / sizeof(long));
            set = SDL_TRUE;
        }
        /* Finally set the transient hints if necessary */
        if (!set) {
            XSetTransientForHint(data->display, w,
                                 RootWindow(data->display,
                                            displaydata->screen));
        }
    } else {
        SDL_bool set;
        Atom WM_HINTS;

        /* We haven't modified the window manager hints yet */
        set = SDL_FALSE;

        /* First try to unset MWM hints */
        WM_HINTS = XInternAtom(data->display, "_MOTIF_WM_HINTS", True);
        if (WM_HINTS != None) {
            XDeleteProperty(data->display, w, WM_HINTS);
            set = SDL_TRUE;
        }
        /* Now try to unset KWM hints */
        WM_HINTS = XInternAtom(data->display, "KWM_WIN_DECORATION", True);
        if (WM_HINTS != None) {
            XDeleteProperty(data->display, w, WM_HINTS);
            set = SDL_TRUE;
        }
        /* Now try to unset GNOME hints */
        WM_HINTS = XInternAtom(data->display, "_WIN_HINTS", True);
        if (WM_HINTS != None) {
            XDeleteProperty(data->display, w, WM_HINTS);
            set = SDL_TRUE;
        }
        /* Finally unset the transient hints if necessary */
        if (!set) {
            /* NOTE: Does this work? */
            XSetTransientForHint(data->display, w, None);
        }
    }

    /* Tell KDE to keep fullscreen windows on top */
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        XEvent ev;
        long mask;

        SDL_zero(ev);
        ev.xclient.type = ClientMessage;
        ev.xclient.window = RootWindow(data->display, displaydata->screen);
        ev.xclient.message_type =
            XInternAtom(data->display, "KWM_KEEP_ON_TOP", False);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = w;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(data->display,
                   RootWindow(data->display, displaydata->screen), False,
                   SubstructureRedirectMask, &ev);
    }

    /* Set the input hints so we get keyboard input */
    wmhints = XAllocWMHints();
    if (wmhints) {
        wmhints->input = True;
        wmhints->flags = InputHint;
        XSetWMHints(data->display, w, wmhints);
        XFree(wmhints);
    }

    /* Set the class hints so we can get an icon (AfterStep) */
    classhints = XAllocClassHint();
    if (classhints != NULL) {
        classhints->res_name = data->classname;
        classhints->res_class = data->classname;
        XSetClassHint(data->display, w, classhints);
        XFree(classhints);
    }

    /* Allow the window to be deleted by the window manager */
    XSetWMProtocols(data->display, w, &data->WM_DELETE_WINDOW, 1);

    if (SetupWindowData(_this, window, w, SDL_TRUE) < 0) {
#ifdef SDL_VIDEO_OPENGL_GLX
        if (window->flags & SDL_WINDOW_OPENGL) {
            X11_GL_Shutdown(_this);
        }
#endif
        XDestroyWindow(data->display, w);
        return -1;
    }
#ifdef X_HAVE_UTF8_STRING
    {
        Uint32 fevent = 0;
        pXGetICValues(((SDL_WindowData *) window->driverdata)->ic,
                      XNFilterEvents, &fevent, NULL);
        XSelectInput(data->display, w,
                     (FocusChangeMask | EnterWindowMask | LeaveWindowMask |
                      ExposureMask | ButtonPressMask | ButtonReleaseMask |
                      PointerMotionMask | KeyPressMask | KeyReleaseMask |
                      PropertyChangeMask | StructureNotifyMask |
                      KeymapStateMask | fevent));
    }
#else
    XSelectInput(data->display, w,
                 (FocusChangeMask | EnterWindowMask | LeaveWindowMask |
                  ExposureMask | ButtonPressMask | ButtonReleaseMask |
                  PointerMotionMask | KeyPressMask | KeyReleaseMask |
                  PropertyChangeMask | StructureNotifyMask |
                  KeymapStateMask));
#endif

    return 0;
}

int
X11_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
    Window w = (Window) data;

    /* FIXME: Query the title from the existing window */

    if (SetupWindowData(_this, window, w, SDL_FALSE) < 0) {
        return -1;
    }
    return 0;
}

void
X11_SetWindowTitle(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;
    XTextProperty titleprop, iconprop;
    Status status;
    const char *title = window->title;
    const char *icon = NULL;

#ifdef X_HAVE_UTF8_STRING
    Atom _NET_WM_NAME = 0;
    Atom _NET_WM_ICON_NAME = 0;

    /* Look up some useful Atoms */
    if (SDL_X11_HAVE_UTF8) {
        _NET_WM_NAME = XInternAtom(display, "_NET_WM_NAME", False);
        _NET_WM_ICON_NAME = XInternAtom(display, "_NET_WM_ICON_NAME", False);
    }
#endif

    if (title != NULL) {
        char *title_locale = SDL_iconv_utf8_locale(title);
        if (!title_locale) {
            SDL_OutOfMemory();
            return;
        }
        status = XStringListToTextProperty(&title_locale, 1, &titleprop);
        SDL_free(title_locale);
        if (status) {
            XSetTextProperty(display, data->window, &titleprop, XA_WM_NAME);
            XFree(titleprop.value);
        }
#ifdef X_HAVE_UTF8_STRING
        if (SDL_X11_HAVE_UTF8) {
            status =
                Xutf8TextListToTextProperty(display, (char **) &title, 1,
                                            XUTF8StringStyle, &titleprop);
            if (status == Success) {
                XSetTextProperty(display, data->window, &titleprop,
                                 _NET_WM_NAME);
                XFree(titleprop.value);
            }
        }
#endif
    }
    if (icon != NULL) {
        char *icon_locale = SDL_iconv_utf8_locale(icon);
        if (!icon_locale) {
            SDL_OutOfMemory();
            return;
        }
        status = XStringListToTextProperty(&icon_locale, 1, &iconprop);
        SDL_free(icon_locale);
        if (status) {
            XSetTextProperty(display, data->window, &iconprop,
                             XA_WM_ICON_NAME);
            XFree(iconprop.value);
        }
#ifdef X_HAVE_UTF8_STRING
        if (SDL_X11_HAVE_UTF8) {
            status =
                Xutf8TextListToTextProperty(display, (char **) &icon, 1,
                                            XUTF8StringStyle, &iconprop);
            if (status == Success) {
                XSetTextProperty(display, data->window, &iconprop,
                                 _NET_WM_ICON_NAME);
                XFree(iconprop.value);
            }
        }
#endif
    }
}

void
X11_SetWindowPosition(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    SDL_DisplayData *displaydata =
        (SDL_DisplayData *) SDL_GetDisplayFromWindow(window)->driverdata;
    Display *display = data->videodata->display;

    XMoveWindow(display, data->window, window->x, window->y);
}

void
X11_SetWindowSize(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    XMoveWindow(display, data->window, window->w, window->h);
}

void
X11_ShowWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    XMapRaised(display, data->window);
}

void
X11_HideWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    XUnmapWindow(display, data->window);
}

void
X11_RaiseWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    XRaiseWindow(display, data->window);
}

void
X11_MaximizeWindow(_THIS, SDL_Window * window)
{
    /* FIXME: is this even possible? */
}

void
X11_MinimizeWindow(_THIS, SDL_Window * window)
{
    X11_HideWindow(_this, window);
}

void
X11_RestoreWindow(_THIS, SDL_Window * window)
{
    X11_ShowWindow(_this, window);
}

void
X11_SetWindowGrab(_THIS, SDL_Window * window)
{
    /* FIXME */
}

void
X11_DestroyWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    window->driverdata = NULL;

    if (data) {
        SDL_VideoData *videodata = (SDL_VideoData *) data->videodata;
        Display *display = videodata->display;
        int numwindows = videodata->numwindows;
        SDL_WindowData **windowlist = videodata->windowlist;
        int i;

        if (windowlist) {
            for (i = 0; i < numwindows; ++i) {
                if (windowlist[i] && (windowlist[i]->windowID == window->id)) {
                    windowlist[i] = windowlist[numwindows - 1];
                    windowlist[numwindows - 1] = NULL;
                    videodata->numwindows--;
                    break;
                }
            }
        }
#ifdef SDL_VIDEO_OPENGL_GLX
        if (window->flags & SDL_WINDOW_OPENGL) {
            X11_GL_Shutdown(_this);
        }
#endif
#ifdef X_HAVE_UTF8_STRING
        if (data->ic) {
            XDestroyIC(data->ic);
        }
#endif
        if (data->created) {
            XDestroyWindow(display, data->window);
        }
        SDL_free(data);
    }
}

SDL_bool
X11_GetWindowWMInfo(_THIS, SDL_Window * window, SDL_SysWMinfo * info)
{
    if (info->version.major <= SDL_MAJOR_VERSION) {
        /* FIXME! */
        return SDL_TRUE;
    } else {
        SDL_SetError("Application not compiled with SDL %d.%d\n",
                     SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
        return SDL_FALSE;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
