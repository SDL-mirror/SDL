/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

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

#include <proto/wb.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>

#include "SDL_os4video.h"
#include "SDL_os4shape.h"
#include "SDL_os4window.h"
#include "SDL_os4modes.h"
#include "SDL_os4opengl.h"
#include "SDL_os4mouse.h"
#include "SDL_os4events.h"

#include "SDL_syswm.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_events_c.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

extern SDL_bool (*OS4_ResizeGlContext)(_THIS, SDL_Window * window);
extern void (*OS4_UpdateGlWindowPointer)(_THIS, SDL_Window * window);

static void OS4_CloseSystemWindow(_THIS, struct Window * window);
static void OS4_CloseWindow(_THIS, SDL_Window * sdlwin);

void
OS4_GetWindowSize(_THIS, struct Window * window, int * width, int * height)
{
    LONG ret = IIntuition->GetWindowAttrs(
        window,
        WA_InnerWidth, width,
        WA_InnerHeight, height,
        TAG_DONE);

    if (ret) {
        dprintf("GetWindowAttrs() returned %d\n", ret);
    }
}

void
OS4_GetWindowActiveSize(SDL_Window * window, int * width, int * height)
{
    if (window->flags & SDL_WINDOW_MAXIMIZED) {
        *width = window->max_w;
        *height = window->max_h;
    } else {
        *width = window->w;
        *height = window->h;
    }
}

static SDL_bool
OS4_IsFullscreen(SDL_Window * window)
{
    return (window->flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP));
}

static void
OS4_RemoveAppWindow(_THIS, SDL_WindowData *data)
{
    if (data->appWin) {
        dprintf("Removing AppWindow\n");

        if (IWorkbench->RemoveAppWindow(data->appWin) == FALSE) {
            dprintf("Failed to remove AppWindow\n");
        }
        data->appWin = NULL;
    }
}

static void
OS4_RemoveAppIcon(_THIS, SDL_WindowData *data)
{
    if (data->appIcon) {
        dprintf("Removing AppIcon\n");

        if (IWorkbench->RemoveAppIcon(data->appIcon) == FALSE) {
            dprintf("Failed to remove AppIcon\n");
        }
        data->appIcon = NULL;
    }
}

static int
OS4_SetupWindowData(_THIS, SDL_Window * sdlwin, struct Window * syswin)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;

    SDL_WindowData *data;

    if (sdlwin->driverdata) {
        data = sdlwin->driverdata;
        dprintf("Old window data %p exists\n", data);
    } else {
        data = (SDL_WindowData *) SDL_calloc(1, sizeof(*data));

        if (!data) {
            return SDL_OutOfMemory();
        }

        sdlwin->driverdata = data;
    }

    data->sdlwin = sdlwin;
    data->syswin = syswin;
    data->pointerGrabTicks = 0;

    if (data->syswin) {
        int width = 0;
        int height = 0;

        OS4_GetWindowSize(_this, data->syswin, &width, &height);

        dprintf("'%s' dimensions %d*%d\n", sdlwin->title, width, height);

        sdlwin->w = width;
        sdlwin->h = height;
    }

    // Pass SDL window as user data
    data->appWin = IWorkbench->AddAppWindow(0, (ULONG)sdlwin, syswin,
        videodata->appMsgPort, TAG_DONE);

    if (!data->appWin) {
        dprintf("Couldn't create AppWindow\n");
        // It's sad but don't fail because of this
    }

    return 0;
}

static uint32
OS4_GetIDCMPFlags(SDL_Window * window, SDL_bool fullscreen)
{
    uint32 IDCMPFlags = IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE
                      | IDCMP_DELTAMOVE | IDCMP_RAWKEY | IDCMP_ACTIVEWINDOW
                      | IDCMP_INACTIVEWINDOW | IDCMP_INTUITICKS
                      | IDCMP_EXTENDEDMOUSE;

    dprintf("Called\n");

    if (!fullscreen) {
        if (!(window->flags & SDL_WINDOW_BORDERLESS)) {
            IDCMPFlags  |= IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_CHANGEWINDOW;
        }

        if (window->flags & SDL_WINDOW_RESIZABLE) {
            //IDCMPFlags  |= IDCMP_SIZEVERIFY; no handling so far
            IDCMPFlags |= IDCMP_NEWSIZE;
        }
    }

    return IDCMPFlags;
}

static uint32
OS4_GetWindowFlags(SDL_Window * window, SDL_bool fullscreen)
{
    uint32 windowFlags = WFLG_REPORTMOUSE | WFLG_RMBTRAP;

    dprintf("Called\n");

    if (fullscreen) {
        windowFlags |= WFLG_BORDERLESS | WFLG_SIMPLE_REFRESH | WFLG_BACKDROP;
    } else {
        windowFlags |= WFLG_SMART_REFRESH | WFLG_NOCAREREFRESH | WFLG_NEWLOOKMENUS;

        if (window->flags & SDL_WINDOW_BORDERLESS) {
            windowFlags |= WFLG_BORDERLESS;
        } else {
            windowFlags |= WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET;

            if (window->flags & SDL_WINDOW_RESIZABLE) {
                windowFlags |= WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM;
            }
        }
    }

    return windowFlags;
}

static struct Screen *
OS4_GetScreenForWindow(_THIS, SDL_VideoDisplay * display)
{
    if (display) {
        SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;

        dprintf("Fullscreen\n");
        return displaydata->screen;
    } else {
        SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;

        dprintf("Window mode (public screen)\n");
        return videodata->publicScreen;
    }
}

static ULONG
OS4_BackFill(const struct Hook *hook, struct RastPort *rastport, struct BackFillMessage *message)
{
    struct Rectangle *rect = &message->Bounds;
    struct GraphicsIFace *igfx = hook->h_Data;

    struct RastPort bfRastport;

    igfx->InitRastPort(&bfRastport);
    bfRastport.BitMap = rastport->BitMap;

    igfx->RectFillColor(&bfRastport, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY, 0xFF000000);

    return 0;
}

static struct Hook OS4_BackFillHook = {
    {0, 0},       /* h_MinNode */
    OS4_BackFill, /* h_Entry */
    0,            /* h_SubEntry */
    0             /* h_Data */
};

static void
OS4_CenterWindow(struct Screen * screen, SDL_Window * window)
{
    if (SDL_WINDOWPOS_ISCENTERED(window->windowed.x) ||
        SDL_WINDOWPOS_ISUNDEFINED(window->windowed.x)) {

        window->windowed.x = (screen->Width - window->windowed.w) / 2;
        dprintf("X centered\n");
    }

    if (SDL_WINDOWPOS_ISCENTERED(window->windowed.y) ||
        SDL_WINDOWPOS_ISUNDEFINED(window->windowed.y)) {

        window->windowed.y = (screen->Height - window->windowed.h) / 2;
        dprintf("Y centered\n");
    }
}

static void
OS4_DefineWindowBox(SDL_Window * window, struct Screen * screen, SDL_bool fullscreen, SDL_Rect * box)
{
    if (fullscreen && screen) {
        box->x = 0; // window->x;
        box->y = 0; // window->y;
        box->w = screen->Width; // window->w;
        box->h = screen->Height; // window->h;
    } else {
        OS4_CenterWindow(screen, window);

        box->x = window->windowed.x;
        box->y = window->windowed.y;
        box->w = window->windowed.w;
        box->h = window->windowed.h;
    }
}

static void
OS4_CreateIconifyGadget(_THIS, SDL_Window * window)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    SDL_WindowData *data = window->driverdata;

    dprintf("Called\n");

    struct DrawInfo *di = IIntuition->GetScreenDrawInfo(videodata->publicScreen);

    if (di) {
        data->image = (struct Image *)IIntuition->NewObject(NULL, SYSICLASS,
            SYSIA_Which, ICONIFYIMAGE,
            SYSIA_DrawInfo, di,
            TAG_DONE);

        if (data->image) {

            dprintf("Image %p for gadget created\n", data->image);

            data->gadget = (struct Gadget *)IIntuition->NewObject(NULL, BUTTONGCLASS,
                GA_Image, data->image,
                GA_ID, GID_ICONIFY,
                GA_TopBorder, TRUE,
                GA_RelRight, TRUE,
                GA_Titlebar, TRUE,
                GA_RelVerify, TRUE,
                TAG_DONE);

            if (data->gadget) {
                struct Window *syswin = data->syswin;

                IIntuition->AddGadget(syswin, data->gadget, -1);

                dprintf("Gadget %p created and added\n", data->gadget);
            } else {
                dprintf("Failed to create button class\n");
            }
        } else {
           dprintf("Failed to create image class\n");
        }

        IIntuition->FreeScreenDrawInfo(videodata->publicScreen, di);

    } else {
        dprintf("Failed to get screen draw info\n");
    }
}

static void
OS4_CreateIconifyGadgetForWindow(_THIS, SDL_Window * window)
{
    if (!OS4_IsFullscreen(window) && !(window->flags & SDL_WINDOW_BORDERLESS)) {
        if (window->w > 99 && window->h > 99) {
            OS4_CreateIconifyGadget(_this, window);
        } else {
            dprintf("Don't add gadget for too small window %d*%d (OS4 bug)\n",
                window->w, window->h);
        }
    }
}

static struct Window *
OS4_CreateSystemWindow(_THIS, SDL_Window * window, SDL_VideoDisplay * display)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;

    struct Window *syswin;

    SDL_bool fullscreen = display ? SDL_TRUE : SDL_FALSE;

    uint32 IDCMPFlags = OS4_GetIDCMPFlags(window, fullscreen);
    uint32 windowFlags = OS4_GetWindowFlags(window, fullscreen);

    struct Screen *screen = OS4_GetScreenForWindow(_this, display);

    SDL_Rect box;

    OS4_BackFillHook.h_Data = IGraphics; // Smuggle interface ptr for the hook

    OS4_DefineWindowBox(window, screen, fullscreen, &box);

    dprintf("Opening window '%s' at (%d,%d) of size (%dx%d) on screen %p\n",
        window->title, box.x, box.y, box.w, box.h, screen);

    syswin = IIntuition->OpenWindowTags(
        NULL,
        WA_PubScreen, screen,
        WA_Title, fullscreen ? NULL : window->title,
        WA_ScreenTitle, window->title,
        WA_Left, box.x,
        WA_Top, box.y,
        WA_InnerWidth, box.w,
        WA_InnerHeight, box.h,
        WA_Flags, windowFlags,
        WA_IDCMP, IDCMPFlags,
        WA_Hidden, (window->flags & SDL_WINDOW_HIDDEN) ? TRUE : FALSE,
        WA_GrabFocus, (window->flags & SDL_WINDOW_INPUT_GRABBED) ? POINTER_GRAB_TIMEOUT : 0,
        WA_UserPort, videodata->userPort,
        WA_BackFill, &OS4_BackFillHook,
        TAG_DONE);

    if (syswin) {
        dprintf("Window address %p\n", syswin);
    } else {
        dprintf("Couldn't create window\n");
        return NULL;
    }

    if (window->flags & SDL_WINDOW_RESIZABLE) {

        // If this window is resizable, reset window size limits
        // so that the user can actually resize it.
        BOOL ret = IIntuition->WindowLimits(syswin,
            syswin->BorderLeft + syswin->BorderRight + 100,
            syswin->BorderTop + syswin->BorderBottom + 100,
            -1,
            -1);

        if (!ret) {
            dprintf("Failed to set window limits\n");
        }
    }

    return syswin;
}

int
OS4_CreateWindow(_THIS, SDL_Window * window)
{
    struct Window *syswin = NULL;

    if (OS4_IsFullscreen(window)) {
        // We may not have the screen opened yet, so let's wait that SDL calls us back with
        // SDL_SetWindowFullscreen() and open the window then.
        dprintf("Open fullscreen window with delay\n");
    } else {
        if (!(syswin = OS4_CreateSystemWindow(_this, window, NULL))) {
            return SDL_SetError("Failed to create system window");
        }
    }

    if (OS4_SetupWindowData(_this, window, syswin) < 0) {

        // There is no AppWindow in this scenario
        OS4_CloseSystemWindow(_this, syswin);

        return SDL_SetError("Failed to setup window data");
    }

    OS4_CreateIconifyGadgetForWindow(_this, window);

    return 0;
}

int
OS4_CreateWindowFrom(_THIS, SDL_Window * window, const void * data)
{
    struct Window *syswin = (struct Window *) data;

    dprintf("Called for native window %p (flags 0x%X)\n", data, window->flags);

    if (syswin->Title && SDL_strlen(syswin->Title)) {
        window->title = SDL_strdup(syswin->Title);
    }

    if (OS4_SetupWindowData(_this, window, syswin) < 0) {
        return -1;
    }

    // TODO: OpenGL, (fullscreen may not be applicable here?)

    return 0;
}

void
OS4_SetWindowTitle(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = window->driverdata;

    //dprintf("Called\n");

    if (data && data->syswin) {
        STRPTR title = window->title ? window->title : "";

        IIntuition->SetWindowTitles(data->syswin, title, title);
    }
}

void
OS4_SetWindowBox(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = window->driverdata;

    if (data && data->syswin) {
        int width, height;
        LONG ret;

        OS4_GetWindowActiveSize(window, &width, &height);

        ret = IIntuition->SetWindowAttrs(data->syswin,
            WA_Left, window->x,
            WA_Top, window->y,
            WA_InnerWidth, width,
            WA_InnerHeight, height,
            TAG_DONE);

        if (ret) {
            dprintf("SetWindowAttrs() returned %d\n", ret);
        }

        if (SDL_IsShapedWindow(window)) {
            OS4_ResizeWindowShape(window);
        }

        if (data->glContext) {
            OS4_ResizeGlContext(_this, window);
        }
    }
}

void
OS4_SetWindowPosition(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = window->driverdata;

    dprintf("New window position %d, %d\n", window->x, window->y);

    if (data && data->syswin) {

        LONG ret = IIntuition->SetWindowAttrs(data->syswin,
            WA_Left, window->x,
            WA_Top, window->y,
            TAG_DONE);

        if (ret) {
            dprintf("SetWindowAttrs() returned %d\n", ret);
        }
    }
}

static void
OS4_ResizeWindow(_THIS, SDL_Window * window, int width, int height)
{
    if (width > 0 && height > 0) {
        SDL_WindowData *data = window->driverdata;

        if (data) {
            LONG ret = IIntuition->SetWindowAttrs(data->syswin,
                WA_InnerWidth, width,
                WA_InnerHeight, height,
                TAG_DONE);

            if (ret) {
                dprintf("SetWindowAttrs() returned %d\n", ret);
            }

            if (SDL_IsShapedWindow(window)) {
                OS4_ResizeWindowShape(window);
            }

            if (data->glContext) {
                OS4_ResizeGlContext(_this, window);
            }
        }
    } else {
        dprintf("Invalid width %d or height %d\n", width, height);
    }
}

void
OS4_SetWindowSize(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = window->driverdata;

    if (data && data->syswin) {

        int width = 0;
        int height = 0;

        OS4_GetWindowSize(_this, data->syswin, &width, &height);

        if (width != window->w || height != window->h) {

            dprintf("New window size %d*%d\n", window->w, window->h);

            OS4_ResizeWindow(_this, window, window->w, window->h);
        } else {
            dprintf("Ignored size request %d*%d\n", width, height);
        }
    }
}

void
OS4_ShowWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = window->driverdata;

    if (data && data->syswin) {

        LONG ret;

        dprintf("Showing window '%s'\n", window->title);

        // TODO: could use ShowWindow but what we pass for the Other?
        ret = IIntuition->SetWindowAttrs(data->syswin,
            WA_Hidden, FALSE,
            TAG_DONE);

        if (ret) {
            dprintf("SetWindowAttrs() returned %d\n", ret);
        }

        if (OS4_IsFullscreen(window)) {
            IIntuition->ScreenToFront(data->syswin->WScreen);
        }

        IIntuition->ActivateWindow(data->syswin);

        window->flags |= SDL_WINDOW_INPUT_FOCUS;

        SDL_SetKeyboardFocus(window);

        // If cursor was disabled earlier, make sure also this window gets the news
        OS4_RefreshCursorState();
    }
}

void
OS4_HideWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = window->driverdata;

    if (window->is_destroying) {
        dprintf("Ignore hide request, window '%s' is destroying\n", window->title);
        return;
    }

    if (data && data->syswin) {
        BOOL result;

        dprintf("Hiding window '%s'\n", window->title);

        // TODO: how to hide a fullscreen window? Close the screen?
        result = IIntuition->HideWindow(data->syswin);

        if (!result) {
            dprintf("HideWindow() failed\n");
        }
    }
}

void
OS4_RaiseWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = window->driverdata;

    if (data && data->syswin) {
        dprintf("Raising window '%s'\n", window->title);

        IIntuition->WindowToFront(data->syswin);
        IIntuition->ActivateWindow(data->syswin);
    }
}

static void
OS4_CloseSystemWindow(_THIS, struct Window * window)
{
    if (window) {
        dprintf("Closing window '%s' (address %p)\n", window->Title, window);

        struct Screen *screen = window->WScreen;

        IIntuition->CloseWindow(window);

        OS4_CloseScreen(_this, screen);
    } else {
        dprintf("NULL pointer\n");
    }
}

static void
OS4_CloseWindow(_THIS, SDL_Window * sdlwin)
{
    SDL_WindowData *data = sdlwin->driverdata;

    if (data) {
        OS4_RemoveAppWindow(_this, data);
        OS4_RemoveAppIcon(_this, data);

        if (data->syswin) {

            OS4_CloseSystemWindow(_this, data->syswin);
            data->syswin = NULL;

            if (data->gadget) {
                dprintf("Disposing gadget %p\n", data->gadget);
                //IIntuition->RemoveGadget(data->syswin, data->gadget);
                IIntuition->DisposeObject((Object *)data->gadget);
                data->gadget = NULL;
            }

            if (data->image) {
                dprintf("Disposing gadget image %p\n", data->image);
                IIntuition->DisposeObject((Object *)data->image);
                data->image = NULL;
            }
        }
    } else {
        dprintf("NULL pointer\n");
    }
}

void
OS4_SetWindowFullscreen(_THIS, SDL_Window * window, SDL_VideoDisplay * display, SDL_bool fullscreen)
{
    if (window->is_destroying) {
        // This function gets also called during window closing
        dprintf("Window '%s' is being destroyed, mode change ignored\n", window->title);
    } else {
        SDL_WindowData *data = window->driverdata;

        dprintf("Trying to set '%s' into %s mode\n", window->title,
            fullscreen ? "fullscreen" : "window");

        if (window->flags & SDL_WINDOW_FOREIGN) {
            dprintf("Native window '%s' (%p), mode change ignored\n", window->title, data->syswin);
        } else {

            int oldWidth = 0;
            int oldHeight = 0;

            if (fullscreen) {
                // Detect dummy transition and keep calm
                SDL_DisplayData *displayData = display->driverdata;

                if (displayData->screen && data->syswin) {
                    if (data->syswin->WScreen == displayData->screen) {
                        dprintf("Same screen, useless mode change ignored\n");
                        return;
                    }
                }
            }

            if (data->syswin) {
                dprintf("Reopening window '%s' (%p) due to mode change\n",
                    window->title, data->syswin);

                OS4_GetWindowSize(_this, data->syswin, &oldWidth, &oldHeight);
                OS4_CloseWindow(_this, window);

            } else {
                dprintf("System window doesn't exist yet, let's open it\n");
            }

            data->syswin = OS4_CreateSystemWindow(_this, window, fullscreen ? display : NULL);

            if (data->syswin) {

                OS4_CreateIconifyGadgetForWindow(_this, window);

                // Make sure the new window is active
                OS4_ShowWindow(_this, window);

                if ((window->flags & SDL_WINDOW_OPENGL) && data->glContext) {
                    OS4_UpdateGlWindowPointer(_this, window);
                }

                if (oldWidth && oldHeight) {
                    int width, height;

                    OS4_GetWindowSize(_this, data->syswin, &width, &height);

                    if (oldWidth != width || oldHeight != height) {

                        dprintf("Inform SDL about window resize\n");
                        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_RESIZED,
                            width, height);
                    }
                }

                dprintf("Resetting keyboard\n");
                SDL_ResetKeyboard();
            }
        }
    }
}

// This may be called from os4events.c
void
OS4_SetWindowGrabPrivate(_THIS, struct Window * w, SDL_bool activate)
{
    if (w) {
        struct IBox grabBox = {
            w->BorderLeft,
            w->BorderTop,
            w->Width  - w->BorderLeft - w->BorderRight,
            w->Height - w->BorderTop  - w->BorderBottom
        };

        LONG ret;

        if (activate) {
            // It seems to be that grabbed window should be active, otherwise some other
            // window (like shell) may be grabbed?
            IIntuition->ActivateWindow(w);

            ret = IIntuition->SetWindowAttrs(w,
                WA_MouseLimits, &grabBox,
                WA_GrabFocus, POINTER_GRAB_TIMEOUT,
                TAG_DONE);
        } else {
            ret = IIntuition->SetWindowAttrs(w,
                WA_MouseLimits, NULL,
                WA_GrabFocus, 0,
                TAG_DONE);
        }

        if (ret) {
            dprintf("SetWindowAttrs() returned %d\n", ret);
        } else {
            dprintf("Window %p ('%s') input was %s\n",
                w, w->Title, activate ? "grabbed" : "released");
        }
    }
}

void
OS4_SetWindowGrab(_THIS, SDL_Window * window, SDL_bool grabbed)
{
    SDL_WindowData *data = window->driverdata;

    if (data) {
        OS4_SetWindowGrabPrivate(_this, data->syswin, grabbed);
        data->pointerGrabTicks = 0;
    }
}

void
OS4_DestroyWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = window->driverdata;

    dprintf("Called for '%s' (flags 0x%X)\n", window->title, window->flags);

    if (data) {

        if (data->syswin) {

            if (!(window->flags & SDL_WINDOW_FOREIGN)) {

                if (SDL_IsShapedWindow(window)) {
                    OS4_DestroyShape(_this, window);
                }

                OS4_CloseWindow(_this, window);
            } else {
                dprintf("Ignored for native window\n");
            }
        }

        if (window->flags & SDL_WINDOW_OPENGL) {
            OS4_GL_FreeBuffers(_this, data);
        }

        SDL_free(data);
        window->driverdata = NULL;
    }
}

SDL_bool
OS4_GetWindowWMInfo(_THIS, SDL_Window * window, struct SDL_SysWMinfo * info)
{
    if (info->version.major <= SDL_MAJOR_VERSION) {
        struct Window *syswin = ((SDL_WindowData *) window->driverdata)->syswin;

        info->subsystem = SDL_SYSWM_OS4;
        info->info.os4.window = syswin;

        dprintf("Window pointer %p\n", syswin);

        return SDL_TRUE;
    } else {
        dprintf("Application not compiled with SDL %d.%d\n",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION);

        SDL_SetError("Application not compiled with SDL %d.%d",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION);

        return SDL_FALSE;
    }
}

int
OS4_SetWindowHitTest(SDL_Window * window, SDL_bool enabled)
{
    return 0; // just succeed, the real work is done elsewhere
}

int
OS4_SetWindowOpacity(_THIS, SDL_Window * window, float opacity)
{
    struct Window *syswin = ((SDL_WindowData *) window->driverdata)->syswin;
    LONG ret;

    UBYTE value = opacity * 255;

    dprintf("Setting window '%s' opaqueness to %d\n", window->title, value);

    ret = IIntuition->SetWindowAttrs(
        syswin,
        WA_Opaqueness, value,
        TAG_DONE);

    if (ret) {
        dprintf("Failed to set window opaqueness to %d\n", value);
        return -1;
    }

    return 0;
}

int
OS4_GetWindowBordersSize(_THIS, SDL_Window * window, int * top, int * left, int * bottom, int * right)
{
    struct Window *syswin = ((SDL_WindowData *) window->driverdata)->syswin;

    if (top) {
        *top = syswin->BorderTop;
    }

    if (left) {
        *left = syswin->BorderLeft;
    }

    if (bottom) {
        *bottom = syswin->BorderBottom;
    }

    if (right) {
        *right = syswin->BorderRight;
    }

    return 0;
}

void
OS4_SetWindowMinMaxSize(_THIS, SDL_Window * window)
{
    if (window->driverdata) {

        dprintf("Window min size %d*%d, max size %d*%d\n",
            window->min_w, window->min_h,
            window->max_w, window->max_h);

        if (window->flags & SDL_WINDOW_RESIZABLE) {

            SDL_WindowData *data = window->driverdata;

            if (data->syswin) {
                struct Window *syswin = data->syswin;

                int width = syswin->BorderLeft + syswin->BorderRight;
                int height = syswin->BorderTop + syswin->BorderBottom;

                BOOL ret =IIntuition->WindowLimits(data->syswin,
                    window->min_w + width, window->min_h + height,
                    window->max_w + width, window->max_h + height);

                if (!ret) {
                    dprintf("Setting window limits failed\n");
                }
            } else {
                dprintf("NULL window\n");
            }
        } else {
            dprintf("Window is not resizable\n");
        }
    }
}

void
OS4_MaximizeWindow(_THIS, SDL_Window * window)
{
    dprintf("Maximizing '%s' to %d*%d\n", window->title, window->max_w, window->max_h);

    if (window->flags & SDL_WINDOW_MINIMIZED) {
        OS4_UniconifyWindow(_this, window);
    }

    // HACK: set flag temporarily so that shaped window and OpenGL
    // context can be resized accordingly...
    window->flags |= SDL_WINDOW_MAXIMIZED;

    OS4_ResizeWindow(_this, window, window->max_w, window->max_h);

    // ...then remove the flag so that user event can be triggered
    window->flags &= ~SDL_WINDOW_MAXIMIZED;

    SDL_SendWindowEvent(window, SDL_WINDOWEVENT_MAXIMIZED, 0, 0);
}

void
OS4_MinimizeWindow(_THIS, SDL_Window * window)
{
    dprintf("Minimizing '%s'\n", window->title);

    OS4_IconifyWindow(_this, window);
}

void
OS4_RestoreWindow(_THIS, SDL_Window * window)
{
    dprintf("Restoring '%s' to %d*%d\n", window->title, window->w, window->h);

    if (window->flags & SDL_WINDOW_MINIMIZED) {
        OS4_UniconifyWindow(_this, window);
    } else if (window->flags & SDL_WINDOW_MAXIMIZED) {
        OS4_ResizeWindow(_this, window, window->w, window->h);

        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_RESTORED, 0, 0);
    } else {
        dprintf("Don't know what to do\n");
    }
}

static struct DiskObject*
OS4_GetDiskObject(_THIS)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    struct DiskObject *diskObject = NULL;

    if (videodata->appName) {
        BPTR oldDir = IDOS->SetCurrentDir(IDOS->GetProgramDir());
        diskObject = IIcon->GetDiskObject(videodata->appName);
        IDOS->SetCurrentDir(oldDir);
    }

    if (!diskObject) {
        CONST_STRPTR fallbackIconName = "ENVARC:Sys/def_window";

        dprintf("Falling back to '%s'\n", fallbackIconName);
        diskObject = IIcon->GetDiskObjectNew(fallbackIconName);
    }

    return diskObject;
}

void
OS4_IconifyWindow(_THIS, SDL_Window * window)
{
    if (window && window->driverdata) {
        SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;

        SDL_WindowData *data = window->driverdata;

        if (window->flags & SDL_WINDOW_MINIMIZED) {
            dprintf("Window '%s' is already iconified\n", window->title);
        } else {
            struct DiskObject *diskObject = OS4_GetDiskObject(_this);

            if (diskObject) {
                diskObject->do_CurrentX = NO_ICON_POSITION;
                diskObject->do_CurrentY = NO_ICON_POSITION;

                data->appIcon = IWorkbench->AddAppIcon(
                    0,
                    (ULONG)window,
                    videodata->appName,
                    videodata->appMsgPort,
                    0,
                    diskObject,
                    TAG_DONE);

                if (!data->appIcon) {
                    dprintf("Failed to add AppIcon\n");
                } else {
                    dprintf("Iconifying '%s'\n", window->title);

                    OS4_HideWindow(_this, window);

                    SDL_SendWindowEvent(window, SDL_WINDOWEVENT_MINIMIZED, 0, 0);
                }

                IIcon->FreeDiskObject(diskObject);
            } else {
                dprintf("Failed to load icon\n");
            }
        }
    }
}

void
OS4_UniconifyWindow(_THIS, SDL_Window * window)
{
    if (window && window->driverdata) {
        if (window->flags & SDL_WINDOW_MINIMIZED) {
            dprintf("Restoring '%s'\n", window->title);

            OS4_RemoveAppIcon(_this, window->driverdata);
            OS4_ShowWindow(_this, window);

            SDL_SendWindowEvent(window, SDL_WINDOWEVENT_RESTORED, 0, 0);
        } else {
            dprintf("Window '%s' isn't in iconified state\n", window->title);
        }
    }
}

void
OS4_SetWindowResizable (_THIS, SDL_Window * window, SDL_bool resizable)
{
    if (window->flags & SDL_WINDOW_FOREIGN) {
        dprintf("Cannot modify native window '%s'\n", window->title);
        return;
    }

    SDL_WindowData *data = window->driverdata;

    if (data->syswin) {
        dprintf("Closing system window '%s' before re-creation\n", window->title);
        OS4_CloseWindow(_this, window);
    }

    data->syswin = OS4_CreateSystemWindow(_this, window, NULL);

    if (data->syswin) {
        OS4_CreateIconifyGadgetForWindow(_this, window);

        // Make sure the new window is active
        OS4_ShowWindow(_this, window);

        if ((window->flags & SDL_WINDOW_OPENGL) && data->glContext) {
            OS4_UpdateGlWindowPointer(_this, window);
        }
    } else {
        dprintf("Failed to re-create window '%s'\n", window->title);
    }
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
