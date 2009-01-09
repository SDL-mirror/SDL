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

#include "SDL_syswm.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"

#include "SDL_DirectFB_video.h"

int
DirectFB_CreateWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_DISPLAYDATA(_this, window);
    DFB_WindowData *windata;
    DFBWindowOptions wopts;
    DFBWindowDescription desc;
    int ret, x, y;

    SDL_DFB_DEBUG("Trace x %d y %d w %d h %d\n", window->x, window->y,
                  window->w, window->h);
    window->driverdata = NULL;
    SDL_DFB_CALLOC(window->driverdata, 1, sizeof(DFB_WindowData));
    windata = (DFB_WindowData *) window->driverdata;

    SDL_DFB_CHECKERR(devdata->
                     dfb->SetCooperativeLevel(devdata->dfb, DFSCL_NORMAL));
    SDL_DFB_CHECKERR(dispdata->
                     layer->SetCooperativeLevel(dispdata->layer,
                                                DLSCL_ADMINISTRATIVE));

    /* Fill the window description. */
    if (window->x == SDL_WINDOWPOS_CENTERED) {
        x = (dispdata->cw - window->w) / 2;
    } else if (window->x == SDL_WINDOWPOS_UNDEFINED) {
        x = 0;
    } else {
        x = window->x;
    }
    if (window->y == SDL_WINDOWPOS_CENTERED) {
        y = (dispdata->ch - window->h) / 2;
    } else if (window->y == SDL_WINDOWPOS_UNDEFINED) {
        y = 0;
    } else {
        y = window->y;
    }
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        x = 0;
        y = 0;
    }

    desc.flags = DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_PIXELFORMAT;
    /*| DWDESC_CAPS | DWDESC_SURFACE_CAPS */

#if (DIRECTFB_MAJOR_VERSION == 1) && (DIRECTFB_MINOR_VERSION >= 0)
    /* Needed for 1.2 */
    desc.flags |= DWDESC_POSX | DWDESC_POSY | DWDESC_SURFACE_CAPS;
    desc.posx = x;
    desc.posy = y;
#else
    if (!(window->flags & SDL_WINDOW_FULLSCREEN)
        && window->x != SDL_WINDOWPOS_UNDEFINED
        && window->y != SDL_WINDOWPOS_UNDEFINED) {
        desc.flags |= DWDESC_POSX | DWDESC_POSY;
        desc.posx = x;
        desc.posy = y;
    }
#endif

    desc.width = window->w;
    desc.height = window->h;
    desc.pixelformat = dispdata->pixelformat;
#if 0
    desc.caps = 0;
    desc.surface_caps =
        DSCAPS_DOUBLE | DSCAPS_TRIPLE | DSCAPS_PREMULTIPLIED |
        DSCAPS_VIDEOONLY;
#endif
    desc.surface_caps = DSCAPS_PREMULTIPLIED;
    /* DSCAPS_VIDEOONLY has negative impact on performance */

    /* Create the window. */
    SDL_DFB_CHECKERR(dispdata->
                     layer->CreateWindow(dispdata->layer, &desc,
                                         &windata->window));

    windata->window->GetOptions(windata->window, &wopts);
#if (DIRECTFB_MAJOR_VERSION == 1) && (DIRECTFB_MINOR_VERSION >= 0)

    if (window->flags & SDL_WINDOW_RESIZABLE)
        wopts |= DWOP_SCALE;
    else
        wopts |= DWOP_KEEP_SIZE;
#else
    wopts |= DWOP_KEEP_SIZE;    /* if not we will crash ... */
#endif

    if (window->flags & SDL_WINDOW_FULLSCREEN)
        wopts |= DWOP_KEEP_POSITION | DWOP_KEEP_STACKING | DWOP_KEEP_SIZE;

    windata->window->SetOptions(windata->window, wopts);
    /* Get the window's surface. */
    SDL_DFB_CHECKERR(windata->
                     window->GetSurface(windata->window, &windata->surface));
    windata->window->SetOpacity(windata->window, 0xFF);
    SDL_DFB_CHECKERR(windata->window->CreateEventBuffer(windata->window,
                                                        &(windata->
                                                          eventbuffer)));
    SDL_DFB_CHECKERR(windata->
                     window->EnableEvents(windata->window, DWET_ALL));

    if (window->flags & SDL_WINDOW_FULLSCREEN)
        windata->window->SetStackingClass(windata->window, DWSC_UPPER);
    /* Make it the top most window. */
    windata->window->RaiseToTop(windata->window);

    windata->window->GetID(windata->window, &windata->windowID);

    windata->window->GetSize(windata->window, &window->w, &window->h);

    /* remember parent */
    windata->id = window->id;

    /* Add to list ... */

    windata->next = devdata->firstwin;
    windata->opacity = 0xFF;
    devdata->firstwin = windata;

    return 0;
  error:
    SDL_DFB_RELEASE(windata->window);
    SDL_DFB_RELEASE(windata->surface);
    return -1;
}

int
DirectFB_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
    SDL_Unsupported();
    return -1;
}

void
DirectFB_SetWindowTitle(_THIS, SDL_Window * window)
{
    SDL_Unsupported();
}

void
DirectFB_SetWindowPosition(_THIS, SDL_Window * window)
{
    SDL_DFB_WINDOWDATA(window);
    int x, y;

    if (window->y == SDL_WINDOWPOS_UNDEFINED)
        y = 0;
    else
        y = window->y;

    if (window->x == SDL_WINDOWPOS_UNDEFINED)
        x = 0;
    else
        x = window->x;

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        x = 0;
        y = 0;
    }

    windata->window->MoveTo(windata->window, x, y);
}

void
DirectFB_SetWindowSize(_THIS, SDL_Window * window)
{
    int ret;
    SDL_DFB_WINDOWDATA(window);

    if (!(window->flags & SDL_WINDOW_FULLSCREEN)) {
#if (DIRECTFB_MAJOR_VERSION == 1) && (DIRECTFB_MINOR_VERSION >= 0)
        int cw;
        int ch;

        /* Make sure all events are disabled for this operation ! */
        SDL_DFB_CHECKERR(windata->
                         window->DisableEvents(windata->window, DWET_ALL));

        SDL_DFB_CHECKERR(windata->window->GetSize(windata->window, &cw, &ch));
        if (cw != window->w || ch != window->h)
            SDL_DFB_CHECKERR(windata->
                             window->Resize(windata->window, window->w,
                                            window->h));
        SDL_DFB_CHECKERR(windata->
                         window->EnableEvents(windata->window, DWET_ALL));

#else
        SDL_DFB_CHECKERR(windata->
                         window->Resize(windata->window, window->w,
                                        window->h));
#endif
        SDL_DFB_CHECKERR(windata->window->GetSize(windata->window, &window->w, &window->h));    /* if a window manager should have decided otherwise */

        SDL_OnWindowResized(window);
    }
    return;
  error:
    windata->window->EnableEvents(windata->window, DWET_ALL);
    return;
}

void
DirectFB_ShowWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_WINDOWDATA(window);

    windata->window->SetOpacity(windata->window, windata->opacity);

}

void
DirectFB_HideWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_WINDOWDATA(window);

    windata->window->GetOpacity(windata->window, &windata->opacity);
    windata->window->SetOpacity(windata->window, 0);
}

void
DirectFB_RaiseWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_WINDOWDATA(window);

    windata->window->RaiseToTop(windata->window);
    windata->window->RequestFocus(windata->window);
}

void
DirectFB_MaximizeWindow(_THIS, SDL_Window * window)
{
    /* FIXME: Size to Desktop ? */

    SDL_Unsupported();
}

void
DirectFB_MinimizeWindow(_THIS, SDL_Window * window)
{
    /* FIXME: Size to 32x32 ? */

    SDL_Unsupported();
}

void
DirectFB_RestoreWindow(_THIS, SDL_Window * window)
{
    SDL_Unsupported();
}

void
DirectFB_SetWindowGrab(_THIS, SDL_Window * window)
{
    SDL_DFB_WINDOWDATA(window);

    if ((window->flags & SDL_WINDOW_INPUT_GRABBED) &&
        (window->flags & SDL_WINDOW_INPUT_FOCUS)) {
        windata->window->GrabPointer(windata->window);
        windata->window->GrabKeyboard(windata->window);
    } else {
        windata->window->UngrabPointer(windata->window);
        windata->window->UngrabKeyboard(windata->window);
    }
}

void
DirectFB_DestroyWindow(_THIS, SDL_Window * window)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    DFB_WindowData *p;

    SDL_DFB_DEBUG("Trace\n");

    SDL_DFB_RELEASE(windata->eventbuffer);
    SDL_DFB_RELEASE(windata->surface);
    SDL_DFB_RELEASE(windata->window);

    /* Remove from list ... */

    p = devdata->firstwin;
    while (p && p->next != windata)
        p = p->next;
    if (p)
        p->next = windata->next;
    else
        devdata->firstwin = windata->next;
    SDL_free(windata);
    return;
}

SDL_bool
DirectFB_GetWindowWMInfo(_THIS, SDL_Window * window,
                         struct SDL_SysWMinfo * info)
{
    SDL_Unsupported();
    return SDL_FALSE;
}
