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

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Utilities for getting and setting the X display mode */

#include <stdlib.h>
#include <string.h>

#include "SDL_timer.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_events_c.h"
#include "SDL_x11video.h"
#include "SDL_x11wm_c.h"
#include "SDL_x11modes_c.h"
#include "SDL_x11image_c.h"

#ifdef HAVE_XINERAMA
#include <XFree86/extensions/Xinerama.h>
#endif 

#define MAX(a, b)        (a > b ? a : b)

#ifdef XFREE86_VM
Bool SDL_NAME(XF86VidModeGetModeInfo)(Display *dpy, int scr, SDL_NAME(XF86VidModeModeInfo) *info)
{
    SDL_NAME(XF86VidModeModeLine) *l = (SDL_NAME(XF86VidModeModeLine)*)((char*)info + sizeof info->dotclock);
    return SDL_NAME(XF86VidModeGetModeLine)(dpy, scr, &info->dotclock, l);
}
#endif /* XFREE86_VM */

#ifdef XFREE86_VM
static void save_mode(_THIS)
{
    memset(&saved_mode, 0, sizeof(saved_mode));
    SDL_NAME(XF86VidModeGetModeInfo)(SDL_Display,SDL_Screen,&saved_mode);
    SDL_NAME(XF86VidModeGetViewPort)(SDL_Display,SDL_Screen,&saved_view.x,&saved_view.y);
}
#endif

#ifdef XFREE86_VM
static void restore_mode(_THIS)
{
    SDL_NAME(XF86VidModeModeLine) mode;
    int unused;

    if ( SDL_NAME(XF86VidModeGetModeLine)(SDL_Display, SDL_Screen, &unused, &mode) ) {
        if ( (saved_mode.hdisplay != mode.hdisplay) ||
             (saved_mode.vdisplay != mode.vdisplay) ) {
            SDL_NAME(XF86VidModeSwitchToMode)(SDL_Display, SDL_Screen, &saved_mode);
        }
    }
    if ( (saved_view.x != 0) || (saved_view.y != 0) ) {
        SDL_NAME(XF86VidModeSetViewPort)(SDL_Display, SDL_Screen, saved_view.x, saved_view.y);
    }
}
#endif

#ifdef XFREE86_VM
static int cmpmodes(const void *va, const void *vb)
{
    const SDL_NAME(XF86VidModeModeInfo) *a = *(const SDL_NAME(XF86VidModeModeInfo)**)va;
    const SDL_NAME(XF86VidModeModeInfo) *b = *(const SDL_NAME(XF86VidModeModeInfo)**)vb;
    if( (a->vdisplay > b->vdisplay) && (a->hdisplay >= b->hdisplay) )
        return -1;
    return b->hdisplay - a->hdisplay;
}
#endif

static void get_real_resolution(_THIS, int* w, int* h);

static void set_best_resolution(_THIS, int width, int height)
{
#ifdef XFREE86_VM
    if ( use_vidmode ) {
        SDL_NAME(XF86VidModeModeLine) mode;
        SDL_NAME(XF86VidModeModeInfo) **modes;
        int i;
        int best_width = 0, best_height = 0;
        int nmodes;

        if ( SDL_NAME(XF86VidModeGetModeLine)(SDL_Display, SDL_Screen, &i, &mode) &&
             SDL_NAME(XF86VidModeGetAllModeLines)(SDL_Display,SDL_Screen,&nmodes,&modes)){
#ifdef XFREE86_DEBUG
            printf("Available modes (unsorted):\n");
            for ( i = 0; i < nmodes; ++i ) {
                printf("Mode %d: %d x %d @ %d\n", i,
                        modes[i]->hdisplay, modes[i]->vdisplay,
                        1000 * modes[i]->dotclock / (modes[i]->htotal *
                        modes[i]->vtotal) );
            }
#endif
            for ( i = 0; i < nmodes ; i++ ) {
                if ( (modes[i]->hdisplay == width) &&
                     (modes[i]->vdisplay == height) )
                    goto match;
            }
            qsort(modes, nmodes, sizeof *modes, cmpmodes);
            for ( i = nmodes-1; i > 0 ; i-- ) {
		if ( ! best_width ) {
                    if ( (modes[i]->hdisplay >= width) &&
                         (modes[i]->vdisplay >= height) ) {
                        best_width = modes[i]->hdisplay;
                        best_height = modes[i]->vdisplay;
                    }
                } else {
                    if ( (modes[i]->hdisplay != best_width) ||
                         (modes[i]->vdisplay != best_height) ) {
                        i++;
                        break;
                    }
                }
            }
       match:
            if ( (modes[i]->hdisplay != mode.hdisplay) ||
                 (modes[i]->vdisplay != mode.vdisplay) ) {
                SDL_NAME(XF86VidModeSwitchToMode)(SDL_Display, SDL_Screen, modes[i]);
            }
            XFree(modes);
        }
    }
#endif /* XFREE86_VM */

                                /* XiG */
#ifdef HAVE_XIGXME
#ifdef XIG_DEBUG
    fprintf(stderr, "XME: set_best_resolution(): w = %d, h = %d\n",
            width, height);
#endif
    if ( SDL_modelist ) {
        int i;

        for ( i=0; SDL_modelist[i]; ++i ) {
            if ( (SDL_modelist[i]->w >= width) &&
                 (SDL_modelist[i]->h >= height) ) {
                break;
            }
        }
        
        if ( SDL_modelist[i] ) { /* found one, lets try it */
            int w, h;        

            /* check current mode so we can avoid uneccessary mode changes */
            get_real_resolution(this, &w, &h);

            if ( (SDL_modelist[i]->w != w) || (SDL_modelist[i]->h != h) ) {
# ifdef XIG_DEBUG
                fprintf(stderr, "XME: set_best_resolution: "
                        "XiGMiscChangeResolution: %d %d\n",
                        SDL_modelist[s]->w, SDL_modelist[s]->h);
# endif
                XiGMiscChangeResolution(SDL_Display, 
                                        SDL_Screen,
                                        0, /* view */
                                        SDL_modelist[i]->w, 
                                        SDL_modelist[i]->h, 
                                        0);
                XSync(SDL_Display, False);
            }
        }
    }
#endif /* HAVE_XIGXME */

}

static void get_real_resolution(_THIS, int* w, int* h)
{
#ifdef XFREE86_VM
    if ( use_vidmode ) {
        SDL_NAME(XF86VidModeModeLine) mode;
        int unused;

        if ( SDL_NAME(XF86VidModeGetModeLine)(SDL_Display, SDL_Screen, &unused, &mode) ) {
            *w = mode.hdisplay;
            *h = mode.vdisplay;
            return;
        }
    }
#endif

#ifdef HAVE_XIGXME
    if ( use_xme ) {
        int ractive;
        XiGMiscResolutionInfo *modelist;

        XiGMiscQueryResolutions(SDL_Display, SDL_Screen,
                                0, /* view */
                                &ractive, &modelist);
        *w = modelist[ractive].width;
        *h = modelist[ractive].height;
#ifdef XIG_DEBUG
        fprintf(stderr, "XME: get_real_resolution: w = %d h = %d\n", *w, *h);
#endif
        XFree(modelist);
        return;
    }
#endif /* XIG_XME */

    *w = DisplayWidth(SDL_Display, SDL_Screen);
    *h = DisplayHeight(SDL_Display, SDL_Screen);
}

/* Called after mapping a window - waits until the window is mapped */
void X11_WaitMapped(_THIS, Window win)
{
    XEvent event;
    do {
        XMaskEvent(SDL_Display, StructureNotifyMask, &event);
    } while ( (event.type != MapNotify) || (event.xmap.event != win) );
}

/* Called after unmapping a window - waits until the window is unmapped */
void X11_WaitUnmapped(_THIS, Window win)
{
    XEvent event;
    do {
        XMaskEvent(SDL_Display, StructureNotifyMask, &event);
    } while ( (event.type != UnmapNotify) || (event.xunmap.event != win) );
}

static void move_cursor_to(_THIS, int x, int y)
{
    XWarpPointer(SDL_Display, None, SDL_Root, 0, 0, 0, 0, x, y);
}

static int add_visual(_THIS, int depth, int class)
{
    XVisualInfo vi;
    if(XMatchVisualInfo(SDL_Display, SDL_Screen, depth, class, &vi)) {
        int n = this->hidden->nvisuals;
        this->hidden->visuals[n].depth = vi.depth;
        this->hidden->visuals[n].visual = vi.visual;
        this->hidden->nvisuals++;
    }
    return(this->hidden->nvisuals);
}
static int add_visual_byid(_THIS, const char *visual_id)
{
    XVisualInfo *vi, template;
    int nvis;

    if ( visual_id ) {
        memset(&template, 0, (sizeof template));
        template.visualid = strtol(visual_id, NULL, 0);
        vi = XGetVisualInfo(SDL_Display, VisualIDMask, &template, &nvis);
        if ( vi ) {
            int n = this->hidden->nvisuals;
            this->hidden->visuals[n].depth = vi->depth;
            this->hidden->visuals[n].visual = vi->visual;
            this->hidden->nvisuals++;
            XFree(vi);
        }
    }
    return(this->hidden->nvisuals);
}

/* Global for the error handler */
int vm_event, vm_error = -1;

int X11_GetVideoModes(_THIS)
{
#ifdef XFREE86_VM
    int buggy_X11;
    int vm_major, vm_minor;
    int nmodes;
    SDL_NAME(XF86VidModeModeInfo) **modes;
#endif
#ifdef HAVE_XIGXME
    int xme_major, xme_minor;
    int ractive, nummodes;
    XiGMiscResolutionInfo *modelist;
#endif
    int i, n;
    int screen_w;
    int screen_h;

    vm_error = -1;
    use_vidmode = 0;
    screen_w = DisplayWidth(SDL_Display, SDL_Screen);
    screen_h = DisplayHeight(SDL_Display, SDL_Screen);

#ifdef XFREE86_VM
    /* Metro-X 4.3.0 and earlier has a broken implementation of
       XF86VidModeGetAllModeLines() - it hangs the client.
     */
    buggy_X11 = 0;
    if ( strcmp(ServerVendor(SDL_Display), "Metro Link Incorporated") == 0 ) {
        FILE *metro_fp;

        metro_fp = fopen("/usr/X11R6/lib/X11/Metro/.version", "r");
        if ( metro_fp != NULL ) {
            int major, minor, patch, version;
            major = 0; minor = 0; patch = 0;
            fscanf(metro_fp, "%d.%d.%d", &major, &minor, &patch);
            version = major*100+minor*10+patch;
            if ( version < 431 ) {
                buggy_X11 = 1;
            }
            fclose(metro_fp);
        }
    }
#if defined(__alpha__) || defined(__sparc64__) || defined(__powerpc__)
    /* The alpha, sparc64 and PPC XFree86 servers are also buggy */
    buggy_X11 = 1;
#endif
    /* Enumerate the available fullscreen modes */
    if ( ! buggy_X11 ) {
        if ( SDL_NAME(XF86VidModeQueryExtension)(SDL_Display, &vm_event, &vm_error) &&
              SDL_NAME(XF86VidModeQueryVersion)(SDL_Display, &vm_major, &vm_minor) ) {
#ifdef BROKEN_XFREE86_4001
#ifdef X_XF86VidModeGetDotClocks  /* Compiled under XFree86 4.0 */
                /* Earlier X servers hang when doing vidmode */
                if ( vm_major < 2 ) {
#ifdef XFREE86_DEBUG
                    printf("Compiled under XFree86 4.0, server is XFree86 3.X\n");
#endif
                    buggy_X11 = 1;
                }
#else
                /* XFree86 3.X code works with XFree86 4.0 servers */;
#endif /* XFree86 4.0 */
#endif /* XFree86 4.02 and newer are fixed wrt backwards compatibility */
        } else {
            buggy_X11 = 1;
        }
    }
    if ( ! buggy_X11 &&
         SDL_NAME(XF86VidModeGetAllModeLines)(SDL_Display, SDL_Screen,&nmodes,&modes) ) {

#ifdef XFREE86_DEBUG
        printf("Available modes: (sorted)\n");
        for ( i = 0; i < nmodes; ++i ) {
            printf("Mode %d: %d x %d @ %d\n", i,
                    modes[i]->hdisplay, modes[i]->vdisplay,
                    1000 * modes[i]->dotclock / (modes[i]->htotal *
                    modes[i]->vtotal) );
        }
#endif

        qsort(modes, nmodes, sizeof *modes, cmpmodes);
        SDL_modelist = (SDL_Rect **)malloc((nmodes+2)*sizeof(SDL_Rect *));
        if ( SDL_modelist ) {
            n = 0;
            for ( i=0; i<nmodes; ++i ) {
                int w, h;

                /* Check to see if we should add the screen size (Xinerama) */
                w = modes[i]->hdisplay;
                h = modes[i]->vdisplay;
                if ( (screen_w * screen_h) >= (w * h) ) {
                    if ( (screen_w != w) || (screen_h != h) ) {
                        SDL_modelist[n] = (SDL_Rect *)malloc(sizeof(SDL_Rect));
                        if ( SDL_modelist[n] ) {
                            SDL_modelist[n]->x = 0;
                            SDL_modelist[n]->y = 0;
                            SDL_modelist[n]->w = screen_w;
                            SDL_modelist[n]->h = screen_h;
                            ++n;
                        }
                    }
                    screen_w = 0;
                    screen_h = 0;
                }

                /* Add the size from the video mode list */
                SDL_modelist[n] = (SDL_Rect *)malloc(sizeof(SDL_Rect));
                if ( SDL_modelist[n] == NULL ) {
                    break;
                }
                SDL_modelist[n]->x = 0;
                SDL_modelist[n]->y = 0;
                SDL_modelist[n]->w = w;
                SDL_modelist[n]->h = h;
                ++n;
            }
            SDL_modelist[n] = NULL;
        }
        XFree(modes);

        use_vidmode = vm_major * 100 + vm_minor;
        save_mode(this);
    }
#endif /* XFREE86_VM */

                                /* XiG */
#ifdef HAVE_XIGXME
    /* first lets make sure we have the extension, and it's at least v2.0 */
    if (XiGMiscQueryVersion(SDL_Display, &xme_major, &xme_minor)) {
#ifdef XIG_DEBUG
        fprintf(stderr, "XME: XiGMiscQueryVersion: V%d.%d\n",
                xme_major, xme_minor);
#endif
        /* work around a XiGMisc bogosity in our version of libXext */
        if (xme_major == 0 && xme_major == 0) {
            /* Ideally libxme would spit this out, but the problem is that
               the right Query func will never be called if using the bogus
               libXext version.
             */
            fprintf(stderr, 
"XME: If you are using Xi Graphics CDE and a Summit server, you need to\n"
"XME: get the libXext update from our ftp site before fullscreen switching\n"
"XME: will work.  Fullscreen switching is only supported on Summit Servers\n");
          }
    } else {
        /* not there. Bummer. */
        xme_major = xme_minor = 0;
    }

    modelist = NULL;
    if (xme_major >= 2 && (nummodes = XiGMiscQueryResolutions(SDL_Display, 
                                            SDL_Screen,
                                            0, /* view */
                                            &ractive, 
                                            &modelist)) > 1)
    {                                /* then we actually have some */
        int j;

#ifdef XIG_DEBUG
        fprintf(stderr, "XME: nummodes = %d, active mode = %d\n",
                nummodes, ractive);
#endif

        SDL_modelist = (SDL_Rect **)malloc((nummodes+1)*sizeof(SDL_Rect *));

                                /* we get the list already sorted in */
                                /* descending order.  We'll copy it in */
                                /* reverse order so SDL is happy */
        if (SDL_modelist) {
            for ( i=0, j=nummodes-1; j>=0; i++, j-- ) {
                if ((SDL_modelist[i] = 
                     (SDL_Rect *)malloc(sizeof(SDL_Rect))) == NULL)
                  break;
#ifdef XIG_DEBUG
                fprintf(stderr, "XME: mode = %4d, w = %4d, h = %4d\n",
                       i, modelist[i].width, modelist[i].height);
#endif
                
                SDL_modelist[i]->x = 0;
                SDL_modelist[i]->y = 0;
                SDL_modelist[i]->w = modelist[j].width;
                SDL_modelist[i]->h = modelist[j].height;
                
            }
            SDL_modelist[i] = NULL; /* terminator */
        }
        use_xme = 1;
        saved_res = modelist[ractive]; /* save the current resolution */
    } else {
        use_xme = 0;
    }
    if ( modelist ) {
        XFree(modelist);
    }
#endif /* HAVE_XIGXME */

    {
        static int depth_list[] = { 32, 24, 16, 15, 8 };
        int j, np;
        int use_directcolor = 1;
        XPixmapFormatValues *pf;

        /* Search for the visuals in deepest-first order, so that the first
           will be the richest one */
        if ( getenv("SDL_VIDEO_X11_NODIRECTCOLOR") ) {
                use_directcolor = 0;
        }
        this->hidden->nvisuals = 0;
        if ( ! add_visual_byid(this, getenv("SDL_VIDEO_X11_VISUALID")) ) {
                for ( i=0; i<SDL_TABLESIZE(depth_list); ++i ) {
                        if ( depth_list[i] > 8 ) {
                                if ( use_directcolor ) {
                                        add_visual(this, depth_list[i], DirectColor);
                                }
                                add_visual(this, depth_list[i], TrueColor);
                        } else {
                                add_visual(this, depth_list[i], PseudoColor);
                                add_visual(this, depth_list[i], StaticColor);
                        }
                }
        }
        if ( this->hidden->nvisuals == 0 ) {
            SDL_SetError("Found no sufficiently capable X11 visuals");
            return -1;
        }
            
        /* look up the pixel quantum for each depth */
        pf = XListPixmapFormats(SDL_Display, &np);
        for(i = 0; i < this->hidden->nvisuals; i++) {
            int d = this->hidden->visuals[i].depth;
            for(j = 0; j < np; j++)
                if(pf[j].depth == d)
                    break;
            this->hidden->visuals[i].bpp = j < np ? pf[j].bits_per_pixel : d;
        }

        XFree(pf);
    }

    if ( SDL_modelist == NULL ) {
        SDL_modelist = (SDL_Rect **)malloc((1+1)*sizeof(SDL_Rect *));
        if ( SDL_modelist ) {
            n = 0;
            SDL_modelist[n] = (SDL_Rect *)malloc(sizeof(SDL_Rect));
            if ( SDL_modelist[n] ) {
                SDL_modelist[n]->x = 0;
                SDL_modelist[n]->y = 0;
                SDL_modelist[n]->w = screen_w;
                SDL_modelist[n]->h = screen_h;
                ++n;
            }
            SDL_modelist[n] = NULL;
        }
    }

#if defined(XFREE86_DEBUG) || defined(XIG_DEBUG)
    if ( use_vidmode ) {
        printf("XFree86 VidMode is enabled\n");
    }

#ifdef HAVE_XIGXME
    if ( use_xme )
      printf("Xi Graphics XME fullscreen is enabled\n");
    else
      printf("Xi Graphics XME fullscreen is not available\n");
#endif 

    if ( SDL_modelist ) {
        printf("X11 video mode list:\n");
        for ( i=0; SDL_modelist[i]; ++i ) {
            printf("\t%dx%d\n", SDL_modelist[i]->w, SDL_modelist[i]->h);
        }
    }
#endif /* XFREE86_DEBUG || XIG_DEBUG */

    /* The default X/Y fullscreen offset is 0/0 */
    xinerama_x = 0;
    xinerama_y = 0;

#ifdef HAVE_XINERAMA
    /* Query Xinerama extention */
    if ( SDL_NAME(XineramaQueryExtension)(SDL_Display, &i, &i) &&
         SDL_NAME(XineramaIsActive)(SDL_Display) ) {
        /* Find out which screen is the desired one */
        int desired = 0;
        int screens;
        SDL_NAME(XineramaScreenInfo) *xinerama;

#ifdef XINERAMA_DEBUG
        printf("X11 detected Xinerama:\n");
#endif
#if 0 /* Apparently the vidmode extension doesn't work with Xinerama */
        const char *variable = getenv("SDL_VIDEO_X11_XINERAMA_SCREEN");
        if ( variable ) {
                desired = atoi(variable);
        }
#endif
        xinerama = SDL_NAME(XineramaQueryScreens)(SDL_Display, &screens);
        for ( i = 0; i < screens; i++ ) {
#ifdef XINERAMA_DEBUG
            printf("xinerama %d: %dx%d+%d+%d\n",
                xinerama[i].screen_number,
                xinerama[i].width, xinerama[i].height,
                xinerama[i].x_org, xinerama[i].y_org);
#endif
            if ( xinerama[i].screen_number == desired ) {
                xinerama_x = xinerama[i].x_org;
                xinerama_y = xinerama[i].y_org;
            }
        }
        XFree(xinerama);
    }
#endif /* HAVE_XINERAMA */

    return 0;
}

int X11_SupportedVisual(_THIS, SDL_PixelFormat *format)
{
    int i;
    for(i = 0; i < this->hidden->nvisuals; i++)
        if(this->hidden->visuals[i].bpp == format->BitsPerPixel)
            return 1;
    return 0;
}

SDL_Rect **X11_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    if ( X11_SupportedVisual(this, format) ) {
        if ( flags & SDL_FULLSCREEN ) {
            return(SDL_modelist);
        } else {
            return((SDL_Rect **)-1);
        }
    } else {
        return((SDL_Rect **)0);
    }
}

void X11_FreeVideoModes(_THIS)
{
    int i;

    if ( SDL_modelist ) {
        for ( i=0; SDL_modelist[i]; ++i ) {
            free(SDL_modelist[i]);
        }
        free(SDL_modelist);
        SDL_modelist = NULL;
    }
}

int X11_ResizeFullScreen(_THIS)
{
    int x, y;
    int real_w, real_h;
    int screen_w;
    int screen_h;

    screen_w = DisplayWidth(SDL_Display, SDL_Screen);
    screen_h = DisplayHeight(SDL_Display, SDL_Screen);

    x = xinerama_x;
    y = xinerama_y;
    if ( currently_fullscreen ) {
        /* Switch resolution and cover it with the FSwindow */
        move_cursor_to(this, x, y);
        set_best_resolution(this, current_w, current_h);
        move_cursor_to(this, x, y);
        get_real_resolution(this, &real_w, &real_h);
        if ( current_w > real_w ) {
            real_w = MAX(real_w, screen_w);
        }
        if ( current_h > real_h ) {
            real_h = MAX(real_h, screen_h);
        }
        XMoveResizeWindow(SDL_Display, FSwindow, x, y, real_w, real_h);
        move_cursor_to(this, real_w/2, real_h/2);

        /* Center and reparent the drawing window */
        x = (real_w - current_w)/2;
        y = (real_h - current_h)/2;
        XReparentWindow(SDL_Display, SDL_Window, FSwindow, x, y);
        /* FIXME: move the mouse to the old relative location */
        XSync(SDL_Display, True);   /* Flush spurious mode change events */
    }
    return(1);
}

void X11_QueueEnterFullScreen(_THIS)
{
    switch_waiting = 0x01 | SDL_FULLSCREEN;
    switch_time = SDL_GetTicks() + 1500;
#if 0 /* This causes a BadMatch error if the window is iconified (not needed) */
    XSetInputFocus(SDL_Display, WMwindow, RevertToNone, CurrentTime);
#endif
}

int X11_EnterFullScreen(_THIS)
{
    int okay;
#if 0
    Window tmpwin, *windows;
    int i, nwindows;
#endif
    int real_w, real_h;
    int screen_w;
    int screen_h;

    okay = 1;
    if ( currently_fullscreen ) {
        return(okay);
    }

    /* Ungrab the input so that we can move the mouse around */
    X11_GrabInputNoLock(this, SDL_GRAB_OFF);

    /* Map the fullscreen window to blank the screen */
    screen_w = DisplayWidth(SDL_Display, SDL_Screen);
    screen_h = DisplayHeight(SDL_Display, SDL_Screen);
    get_real_resolution(this, &real_w, &real_h);
    if ( current_w > real_w ) {
        real_w = MAX(real_w, screen_w);
    }
    if ( current_h > real_h ) {
        real_h = MAX(real_h, screen_h);
    }
    XMoveResizeWindow(SDL_Display, FSwindow,
                      xinerama_x, xinerama_y, real_w, real_h);
    XMapRaised(SDL_Display, FSwindow);
    X11_WaitMapped(this, FSwindow);

#if 0 /* This seems to break WindowMaker in focus-follows-mouse mode */
    /* Make sure we got to the top of the window stack */
    if ( XQueryTree(SDL_Display, SDL_Root, &tmpwin, &tmpwin,
                            &windows, &nwindows) && windows ) {
        /* If not, try to put us there - if fail... oh well */
        if ( windows[nwindows-1] != FSwindow ) {
            tmpwin = windows[nwindows-1];
            for ( i=0; i<nwindows; ++i ) {
                if ( windows[i] == FSwindow ) {
                    memcpy(&windows[i], &windows[i+1],
                           (nwindows-i-1)*sizeof(windows[i]));
                    break;
                }
            }
            windows[nwindows-1] = FSwindow;
            XRestackWindows(SDL_Display, windows, nwindows);
            XSync(SDL_Display, False);
        }
        XFree(windows);
    }
#else
    XRaiseWindow(SDL_Display, FSwindow);
#endif

#ifdef XFREE86_VM
    /* Save the current video mode */
    if ( use_vidmode ) {
        SDL_NAME(XF86VidModeLockModeSwitch)(SDL_Display, SDL_Screen, True);
    }
#endif
    currently_fullscreen = 1;

    /* Set the new resolution */
    okay = X11_ResizeFullScreen(this);
    if ( ! okay ) {
        X11_LeaveFullScreen(this);
    }
    /* Set the colormap */
    if ( SDL_XColorMap ) {
        XInstallColormap(SDL_Display, SDL_XColorMap);
    }
    if ( okay )
        X11_GrabInputNoLock(this, this->input_grab | SDL_GRAB_FULLSCREEN);

    /* We may need to refresh the screen at this point (no backing store)
       We also don't get an event, which is why we explicitly refresh. */
    if ( this->screen ) {
        if ( this->screen->flags & SDL_OPENGL ) {
            SDL_PrivateExpose();
        } else {
            X11_RefreshDisplay(this);
        }
    }

    return(okay);
}

int X11_LeaveFullScreen(_THIS)
{
    if ( currently_fullscreen ) {
        XReparentWindow(SDL_Display, SDL_Window, WMwindow, 0, 0);
#ifdef XFREE86_VM
        if ( use_vidmode ) {
            restore_mode(this);
            SDL_NAME(XF86VidModeLockModeSwitch)(SDL_Display, SDL_Screen, False);
        }
#endif

#ifdef HAVE_XIGXME
        if ( use_xme ) {
            int rw, rh;        
            
            /* check current mode so we can avoid uneccessary mode changes */
            get_real_resolution(this, &rw, &rh);

            if (rw != saved_res.width || rh != saved_res.height) {
                XiGMiscChangeResolution(SDL_Display, 
                                        SDL_Screen,
                                        0, /* view */
                                        saved_res.width, 
                                        saved_res.height,
                                        0);
                XSync(SDL_Display, False);
            }
        }
#endif

        XUnmapWindow(SDL_Display, FSwindow);
        X11_WaitUnmapped(this, FSwindow);
        XSync(SDL_Display, True);   /* Flush spurious mode change events */
        currently_fullscreen = 0;
    }
    /* If we get popped out of fullscreen mode for some reason, input_grab
       will still have the SDL_GRAB_FULLSCREEN flag set, since this is only
       temporary.  In this case, release the grab unless the input has been
       explicitly grabbed.
     */
    X11_GrabInputNoLock(this, this->input_grab & ~SDL_GRAB_FULLSCREEN);

    /* We may need to refresh the screen at this point (no backing store)
       We also don't get an event, which is why we explicitly refresh. */
    if ( this->screen ) {
        if ( this->screen->flags & SDL_OPENGL ) {
            SDL_PrivateExpose();
        } else {
            X11_RefreshDisplay(this);
        }
    }

    return(0);
}
