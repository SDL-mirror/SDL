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

/* we need to define it, so that raw input is included */

#if (_WIN32_WINNT < 0x0501)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include "SDL_config.h"

#include "../SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"

#include "SDL_win32video.h"

/* This is included after SDL_win32video.h, which includes windows.h */
#include "SDL_syswm.h"

extern HCTX *g_hCtx;            /* the table of tablet event contexts, each windows has to have it's own tablet context */
static Uint32 highestId = 0;    /* the highest id of the tablet context */

/* Fake window to help with DirectInput events. */
HWND SDL_HelperWindow = NULL;
static WCHAR *SDL_HelperWindowClassName = TEXT("SDLHelperWindowInputCatcher");
static WCHAR *SDL_HelperWindowName = TEXT("SDLHelperWindowInputMsgWindow");
static ATOM SDL_HelperWindowClass = 0;

static int
SetupWindowData(_THIS, SDL_Window * window, HWND hwnd, SDL_bool created)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    SDL_WindowData *data;

    /* Allocate the window data */
    data = (SDL_WindowData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    data->windowID = window->id;
    data->hwnd = hwnd;
    data->hdc = GetDC(hwnd);
    data->created = created;
    data->mouse_pressed = SDL_FALSE;
    data->videodata = videodata;

    /* Associate the data with the window */
    if (!SetProp(hwnd, TEXT("SDL_WindowData"), data)) {
        ReleaseDC(hwnd, data->hdc);
        SDL_free(data);
        WIN_SetError("SetProp() failed");
        return -1;
    }

    /* Set up the window proc function */
    data->wndproc = (WNDPROC) GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    if (data->wndproc == NULL) {
        data->wndproc = DefWindowProc;
    } else {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) WIN_WindowProc);
    }

    /* Fill in the SDL window with the window data */
    {
        POINT point;
        point.x = 0;
        point.y = 0;
        if (ClientToScreen(hwnd, &point)) {
            window->x = point.x;
            window->y = point.y;
        }
    }
    {
        RECT rect;
        if (GetClientRect(hwnd, &rect)) {
            window->w = rect.right;
            window->h = rect.bottom;
        }
    }
    {
        DWORD style = GetWindowLong(hwnd, GWL_STYLE);
        if (style & WS_VISIBLE) {
            window->flags |= SDL_WINDOW_SHOWN;
        } else {
            window->flags &= ~SDL_WINDOW_SHOWN;
        }
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

    /* All done! */
    window->driverdata = data;
    return 0;
}

int
WIN_CreateWindow(_THIS, SDL_Window * window)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    RAWINPUTDEVICE Rid;
    AXIS TabX, TabY;
    LOGCONTEXTA lc;
    HWND hwnd;
    HWND top;
    RECT rect;
    DWORD style = (WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    int x, y;
    int w, h;

    if (window->flags & SDL_WINDOW_BORDERLESS) {
        style |= WS_POPUP;
    } else {
        style |= (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    }
    if (window->flags & SDL_WINDOW_RESIZABLE) {
        style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    /* Figure out what the window area will be */
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        top = HWND_TOPMOST;
    } else {
        top = HWND_NOTOPMOST;
    }
    rect.left = 0;
    rect.top = 0;
    rect.right = window->w;
    rect.bottom = window->h;
    AdjustWindowRectEx(&rect, style, FALSE, 0);
    w = (rect.right - rect.left);
    h = (rect.bottom - rect.top);

    if (window->x == SDL_WINDOWPOS_CENTERED) {
        x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    } else if (window->x == SDL_WINDOWPOS_UNDEFINED) {
        x = CW_USEDEFAULT;
    } else {
        x = window->x + rect.left;
    }
    if (window->y == SDL_WINDOWPOS_CENTERED) {
        y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    } else if (window->y == SDL_WINDOWPOS_UNDEFINED) {
        y = CW_USEDEFAULT;
    } else {
        y = window->y + rect.top;
    }

    hwnd =
        CreateWindow(SDL_Appname, TEXT(""), style, x, y, w, h, NULL, NULL,
                     SDL_Instance, NULL);
    if (!hwnd) {
        WIN_SetError("Couldn't create window");
        return -1;
    }

    /* we're configuring the tablet data. See Wintab reference for more info */
    if (videodata->wintabDLL
        && videodata->WTInfoA(WTI_DEFSYSCTX, 0, &lc) != 0) {
        lc.lcPktData = PACKETDATA;
        lc.lcPktMode = PACKETMODE;
        lc.lcOptions |= CXO_MESSAGES;
        lc.lcOptions |= CXO_SYSTEM;
        lc.lcMoveMask = PACKETDATA;
        lc.lcBtnDnMask = lc.lcBtnUpMask = PACKETDATA;
        videodata->WTInfoA(WTI_DEVICES, DVC_X, &TabX);
        videodata->WTInfoA(WTI_DEVICES, DVC_Y, &TabY);
        lc.lcInOrgX = 0;
        lc.lcInOrgY = 0;
        lc.lcInExtX = TabX.axMax;
        lc.lcInExtY = TabY.axMax;
        lc.lcOutOrgX = 0;
        lc.lcOutOrgY = 0;
        lc.lcOutExtX = GetSystemMetrics(SM_CXSCREEN);
        lc.lcOutExtY = -GetSystemMetrics(SM_CYSCREEN);
        if (window->id > highestId) {
            HCTX *tmp_hctx;
            highestId = window->id;
            tmp_hctx =
                (HCTX *) SDL_realloc(g_hCtx, (highestId + 1) * sizeof(HCTX));
            if (!tmp_hctx) {
                SDL_OutOfMemory();
                DestroyWindow(hwnd);
                return -1;
            }
            g_hCtx = tmp_hctx;
        }
        g_hCtx[window->id] = videodata->WTOpenA(hwnd, &lc, TRUE);
    }

    /* we're telling the window, we want it to report raw input events from mice */
    Rid.usUsagePage = 0x01;
    Rid.usUsage = 0x02;
    Rid.dwFlags = RIDEV_INPUTSINK;
    Rid.hwndTarget = hwnd;
    RegisterRawInputDevices(&Rid, 1, sizeof(Rid));

    WIN_PumpEvents(_this);

    if (SetupWindowData(_this, window, hwnd, SDL_TRUE) < 0) {
        DestroyWindow(hwnd);
        return -1;
    }
#ifdef SDL_VIDEO_OPENGL_WGL
    if (window->flags & SDL_WINDOW_OPENGL) {
        if (WIN_GL_SetupWindow(_this, window) < 0) {
            WIN_DestroyWindow(_this, window);
            return -1;
        }
    }
#endif
    return 0;
}

int
WIN_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
    HWND hwnd = (HWND) data;
    LPTSTR title;
    int titleLen;

    /* Query the title from the existing window */
    titleLen = GetWindowTextLength(hwnd);
    title = SDL_stack_alloc(TCHAR, titleLen + 1);
    if (title) {
        titleLen = GetWindowText(hwnd, title, titleLen);
    } else {
        titleLen = 0;
    }
    if (titleLen > 0) {
        window->title = WIN_StringToUTF8(title);
    }
    if (title) {
        SDL_stack_free(title);
    }

    if (SetupWindowData(_this, window, hwnd, SDL_FALSE) < 0) {
        return -1;
    }
    return 0;
}

void
WIN_SetWindowTitle(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    LPTSTR title;

    if (window->title) {
        title = WIN_UTF8ToString(window->title);
    } else {
        title = NULL;
    }
    SetWindowText(hwnd, title ? title : TEXT(""));
    if (title) {
        SDL_free(title);
    }
}

void
WIN_SetWindowPosition(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    RECT rect;
    DWORD style;
    HWND top;
    int x, y;

    /* Figure out what the window area will be */
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        top = HWND_TOPMOST;
    } else {
        top = HWND_NOTOPMOST;
    }
    style = GetWindowLong(hwnd, GWL_STYLE);
    rect.left = 0;
    rect.top = 0;
    rect.right = window->w;
    rect.bottom = window->h;
    AdjustWindowRectEx(&rect, style,
                       (style & WS_CHILDWINDOW) ? FALSE : (GetMenu(hwnd) !=
                                                           NULL), 0);
    x = window->x + rect.left;
    y = window->y + rect.top;

    SetWindowPos(hwnd, top, x, y, 0, 0, (SWP_NOCOPYBITS | SWP_NOSIZE));
}

void
WIN_SetWindowSize(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    RECT rect;
    DWORD style;
    HWND top;
    int w, h;

    /* Figure out what the window area will be */
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        top = HWND_TOPMOST;
    } else {
        top = HWND_NOTOPMOST;
    }
    style = GetWindowLong(hwnd, GWL_STYLE);
    rect.left = 0;
    rect.top = 0;
    rect.right = window->w;
    rect.bottom = window->h;
    AdjustWindowRectEx(&rect, style,
                       (style & WS_CHILDWINDOW) ? FALSE : (GetMenu(hwnd) !=
                                                           NULL), 0);
    w = (rect.right - rect.left);
    h = (rect.bottom - rect.top);

    SetWindowPos(hwnd, top, 0, 0, h, w, (SWP_NOCOPYBITS | SWP_NOMOVE));
}

void
WIN_ShowWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;

    ShowWindow(hwnd, SW_SHOW);
}

void
WIN_HideWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;

    ShowWindow(hwnd, SW_HIDE);
}

void
WIN_RaiseWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    HWND top;

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        top = HWND_TOPMOST;
    } else {
        top = HWND_NOTOPMOST;
    }
    SetWindowPos(hwnd, top, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE));
}

void
WIN_MaximizeWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;

    ShowWindow(hwnd, SW_MAXIMIZE);
}

void
WIN_MinimizeWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;

    ShowWindow(hwnd, SW_MINIMIZE);
}

void
WIN_RestoreWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;

    ShowWindow(hwnd, SW_RESTORE);
}

void
WIN_SetWindowGrab(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;

    if ((window->flags & SDL_WINDOW_INPUT_GRABBED) &&
        (window->flags & SDL_WINDOW_INPUT_FOCUS)) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        ClientToScreen(hwnd, (LPPOINT) & rect);
        ClientToScreen(hwnd, (LPPOINT) & rect + 1);
        ClipCursor(&rect);
    } else {
        ClipCursor(NULL);
    }
}

void
WIN_DestroyWindow(_THIS, SDL_Window * window)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;

    if (data) {
#ifdef SDL_VIDEO_OPENGL_WGL
        if (window->flags & SDL_WINDOW_OPENGL) {
            WIN_GL_CleanupWindow(_this, window);
        }
#endif
        ReleaseDC(data->hwnd, data->hdc);
        if (data->created) {
            if (videodata->wintabDLL) {
                videodata->WTClose(g_hCtx[window->id]);
            }
            DestroyWindow(data->hwnd);
        }
        SDL_free(data);
    }
}

SDL_bool
WIN_GetWindowWMInfo(_THIS, SDL_Window * window, SDL_SysWMinfo * info)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    if (info->version.major <= SDL_MAJOR_VERSION) {
        info->window = hwnd;
        return SDL_TRUE;
    } else {
        SDL_SetError("Application not compiled with SDL %d.%d\n",
                     SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
        return SDL_FALSE;
    }
}


/*
 * Creates a HelperWindow used for DirectInput events.
 */
int
SDL_HelperWindowCreate(void)
{
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    WNDCLASSEX wce;

    /* Create the class. */
    SDL_zero(wce);
    wce.cbSize = sizeof(WNDCLASSEX);
    wce.lpfnWndProc = DefWindowProcA;
    wce.lpszClassName = (LPCWSTR) SDL_HelperWindowClassName;
    wce.hInstance = hInstance;

    /* Register the class. */
    SDL_HelperWindowClass = RegisterClassEx(&wce);
    if (SDL_HelperWindowClass == 0) {
        SDL_SetError("Unable to create Helper Window Class: error %d.",
                     GetLastError());
        return -1;
    }

    /* Create the window. */
    SDL_HelperWindow = CreateWindowEx(0, SDL_HelperWindowClassName,
                                      SDL_HelperWindowName,
                                      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, HWND_MESSAGE, NULL,
                                      hInstance, NULL);
    if (SDL_HelperWindow == NULL) {
        SDL_SetError("Unable to create Helper Window: error %d.",
                     GetLastError());
        return -1;
    }

    return 0;
}


/*
 * Destroys the HelperWindow previously created with SDL_HelperWindowCreate.
 */
void
SDL_HelperWindowDestroy(void)
{
    /* Destroy the window. */
    if (SDL_HelperWindow) {
        DestroyWindow(SDL_HelperWindow);
        SDL_HelperWindow = NULL;
    }

    /* Unregister the class. */
    if (SDL_HelperWindowClass) {
        UnregisterClass(SDL_HelperWindowClassName, GetModuleHandleA(NULL));
        SDL_HelperWindowClass = 0;
    }
}


/* vi: set ts=4 sw=4 expandtab: */
