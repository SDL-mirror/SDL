/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

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
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_assert.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_uikitvideo.h"
#include "SDL_uikitevents.h"
#include "SDL_uikitwindow.h"
#import "SDL_uikitappdelegate.h"

#import "SDL_uikitopenglview.h"

#include <Foundation/Foundation.h>

@implementation SDL_uikitviewcontroller

- (id)initWithSDLWindow:(SDL_Window *)_window {
    [self init];
    self->window = _window;
    return self;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orient {
    return YES;
}

- (void)loadView  {
    // do nothing.
}

// Send a resized event when the orientation changes.
- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
    const UIInterfaceOrientation toInterfaceOrientation = [self interfaceOrientation];
    SDL_WindowData *data = self->window->driverdata;
    UIWindow *uiwindow = data->uiwindow;
    CGRect frame = [uiwindow frame];
    const CGSize size = frame.size;
    int w, h;

    switch (toInterfaceOrientation) {
        case UIInterfaceOrientationPortrait:
        case UIInterfaceOrientationPortraitUpsideDown:
            w = (size.width < size.height) ? size.width : size.height;
            h = (size.width > size.height) ? size.width : size.height;
            break;

        case UIInterfaceOrientationLandscapeLeft:
        case UIInterfaceOrientationLandscapeRight:
            w = (size.width > size.height) ? size.width : size.height;
            h = (size.width < size.height) ? size.width : size.height;
            break;

        default:
            SDL_assert(0 && "Unexpected interface orientation!");
            return;
    }

    frame.size.width = w;
    frame.size.height = h;
    [uiwindow setFrame:frame];
    [data->view updateFrame];
    SDL_SendWindowEvent(self->window, SDL_WINDOWEVENT_RESIZED, w, h);
}

@end



static int SetupWindowData(_THIS, SDL_Window *window, UIWindow *uiwindow, SDL_bool created)
{
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    UIScreen *uiscreen = (UIScreen *) display->driverdata;
    SDL_WindowData *data;
        
    /* Allocate the window data */
    data = (SDL_WindowData *)SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    data->uiwindow = uiwindow;
    data->viewcontroller = nil;
    data->view = nil;

    /* Fill in the SDL window with the window data */
    {
        window->x = 0;
        window->y = 0;
        window->w = (int)uiwindow.frame.size.width;
        window->h = (int)uiwindow.frame.size.height;
    }
    
    window->driverdata = data;

    // !!! FIXME: should we force this? Shouldn't specifying FULLSCREEN
    // !!! FIXME:  imply BORDERLESS?
    window->flags |= SDL_WINDOW_FULLSCREEN;        /* window is always fullscreen */
    window->flags |= SDL_WINDOW_SHOWN;            /* only one window on iOS, always shown */

    // SDL_WINDOW_BORDERLESS controls whether status bar is hidden.
    // This is only set if the window is on the main screen. Other screens
    //  just force the window to have the borderless flag.
    if ([UIScreen mainScreen] != uiscreen) {
        window->flags &= ~SDL_WINDOW_RESIZABLE;  // window is NEVER resizeable
        window->flags &= ~SDL_WINDOW_INPUT_FOCUS;  // never has input focus
        window->flags |= SDL_WINDOW_BORDERLESS;  // never has a status bar.
    } else {
        window->flags |= SDL_WINDOW_INPUT_FOCUS;  // always has input focus

        if (window->flags & SDL_WINDOW_BORDERLESS) {
            [UIApplication sharedApplication].statusBarHidden = YES;
        } else {
            [UIApplication sharedApplication].statusBarHidden = NO;
        }

        const CGSize uisize = [[uiscreen currentMode] size];
        const UIDeviceOrientation o = [[UIDevice currentDevice] orientation];
        const BOOL landscape = (o == UIDeviceOrientationLandscapeLeft) ||
                                   (o == UIDeviceOrientationLandscapeRight);
        const BOOL rotate = ( ((window->w > window->h) && (!landscape)) ||
                              ((window->w < window->h) && (landscape)) );

        if (window->flags & SDL_WINDOW_RESIZABLE) {
            // The View Controller will handle rotating the view when the
            //  device orientation changes. We expose these as resize events.
            SDL_uikitviewcontroller *controller;
            controller = [SDL_uikitviewcontroller alloc];
            data->viewcontroller = [controller initWithSDLWindow:window];
            [data->viewcontroller setTitle:@"SDL App"];  // !!! FIXME: hook up SDL_SetWindowTitle()
            // !!! FIXME: if (rotate), force a "resize" right at the start
        } else {
            // Rotate the view if we have to, but only on the main screen
            //  (presumably, an external display doesn't report orientation).
            if (rotate) {
                #define D2R(x) (M_PI * (x) / 180.0)   // degrees to radians.
                [uiwindow setTransform:CGAffineTransformIdentity];
                [uiwindow setTransform:CGAffineTransformMakeRotation(D2R(90))];
                #undef D2R
            }
        }
    }

    return 0;
}

int
UIKit_CreateWindow(_THIS, SDL_Window *window)
{
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    UIScreen *uiscreen = (UIScreen *) display->driverdata;

    // SDL currently puts this window at the start of display's linked list. We rely on this.
    SDL_assert(_this->windows == window);

    /* We currently only handle a single window per display on iOS */
    if (window->next != NULL) {
        SDL_SetError("Only one window allowed per display.");
        return -1;
    }

    // Non-mainscreen windows must be force to borderless, as there's no
    //  status bar there, and we want to get the right dimensions later in
    //  this function.
    if ([UIScreen mainScreen] != uiscreen) {
        window->flags |= SDL_WINDOW_BORDERLESS;
    }

    // If monitor has a resolution of 0x0 (hasn't been explicitly set by the
    //  user, so it's in standby), try to force the display to a resolution
    //  that most closely matches the desired window size.
    if (SDL_UIKit_supports_multiple_displays) {
        const CGSize origsize = [[uiscreen currentMode] size];
        if ((origsize.width == 0.0f) && (origsize.height == 0.0f)) {
            if (display->num_display_modes == 0) {
                _this->GetDisplayModes(_this, display);
            }

            int i;
            const SDL_DisplayMode *bestmode = NULL;
            for (i = display->num_display_modes; i >= 0; i--) {
                const SDL_DisplayMode *mode = &display->display_modes[i];
                if ((mode->w >= window->w) && (mode->h >= window->h))
                    bestmode = mode;
            }

            if (bestmode) {
                UIScreenMode *uimode = (UIScreenMode *) bestmode->driverdata;
                [uiscreen setCurrentMode:uimode];
                display->desktop_mode = *bestmode;
                display->current_mode = *bestmode;
            }
        }
    }

    /* ignore the size user requested, and make a fullscreen window */
    // !!! FIXME: can we have a smaller view?
    UIWindow *uiwindow = [UIWindow alloc];
    if (window->flags & SDL_WINDOW_BORDERLESS)
        uiwindow = [uiwindow initWithFrame:[uiscreen bounds]];
    else
        uiwindow = [uiwindow initWithFrame:[uiscreen applicationFrame]];

    if (SDL_UIKit_supports_multiple_displays) {
        [uiwindow setScreen:uiscreen];
    }

    if (SetupWindowData(_this, window, uiwindow, SDL_TRUE) < 0) {
        [uiwindow release];
        return -1;
    }    
    
    return 1;
    
}

void
UIKit_DestroyWindow(_THIS, SDL_Window * window) {
    SDL_WindowData *data = (SDL_WindowData *)window->driverdata;
    if (data) {
        [data->viewcontroller release];
        [data->uiwindow release];
        SDL_free(data);
        window->driverdata = NULL;
    }
}

SDL_bool
UIKit_GetWindowWMInfo(_THIS, SDL_Window * window, SDL_SysWMinfo * info)
{
    UIWindow *uiwindow = ((SDL_WindowData *) window->driverdata)->uiwindow;

    if (info->version.major <= SDL_MAJOR_VERSION) {
        info->subsystem = SDL_SYSWM_UIKIT;
        info->info.uikit.window = uiwindow;
        return SDL_TRUE;
    } else {
        SDL_SetError("Application not compiled with SDL %d.%d\n",
                     SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
        return SDL_FALSE;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
