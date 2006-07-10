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

#include "../SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"

#include "SDL_win32video.h"

/* This is included after SDL_win32video.h, which includes windows.h */
#include "SDL_syswm.h"


static int
SetupWindowData(SDL_Window * window, HWND hwnd, BOOL created)
{
    SDL_WindowData *data;

    /* Allocate the window data */
    data = (SDL_WindowData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    data->windowID = window->id;
    data->hwnd = hwnd;
    data->created = created;
    data->mouse_pressed = SDL_FALSE;
    data->videodata = (SDL_VideoData *) SDL_GetVideoDevice()->driverdata;

    /* Associate the data with the window */
    if (!SetProp(hwnd, TEXT("SDL_WindowData"), data)) {
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
    HWND hwnd;
    LPTSTR title = NULL;
    HWND top;
    RECT rect;
    DWORD style = 0;
    int x, y;
    int w, h;

    if (window->title) {
        title = WIN_UTF8ToString(window->title);
    } else {
        title = NULL;
    }

    if (window->flags & SDL_WINDOW_SHOWN) {
        style |= WS_VISIBLE;
    }
    if ((window->flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS))) {
        style |= WS_POPUP;
    } else {
        style |= (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    }
    if (window->flags & SDL_WINDOW_RESIZABLE) {
        style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    if (window->flags & SDL_WINDOW_MAXIMIZED) {
        style |= WS_MAXIMIZE;
    }
    if (window->flags & SDL_WINDOW_MINIMIZED) {
        style |= WS_MINIMIZE;
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

    if ((window->flags & SDL_WINDOW_FULLSCREEN) ||
        window->x == SDL_WINDOWPOS_CENTERED) {
        x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    } else if (window->x == SDL_WINDOWPOS_UNDEFINED) {
        x = CW_USEDEFAULT;
    } else {
        x = window->x + rect.left;
    }
    if ((window->flags & SDL_WINDOW_FULLSCREEN) ||
        window->y == SDL_WINDOWPOS_CENTERED) {
        y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    } else if (window->y == SDL_WINDOWPOS_UNDEFINED) {
        y = CW_USEDEFAULT;
    } else {
        y = window->y + rect.top;
    }

    hwnd = CreateWindow(SDL_Appname,
                        title ? title : TEXT(""),
                        style, x, y, w, h, NULL, NULL, SDL_Instance, NULL);
    WIN_PumpEvents(_this);

    if (title) {
        SDL_free(title);
    }

    if (!hwnd) {
        WIN_SetError("Couldn't create window");
        return -1;
    }

    if (SetupWindowData(window, hwnd, TRUE) < 0) {
        DestroyWindow(hwnd);
        return -1;
    }
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

    if (SetupWindowData(window, hwnd, FALSE) < 0) {
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

    if ((window->flags & SDL_WINDOW_FULLSCREEN) ||
        window->x == SDL_WINDOWPOS_CENTERED) {
        x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
        window->x = x - rect.left;
    } else {
        x = window->x + rect.left;
    }
    if ((window->flags & SDL_WINDOW_FULLSCREEN) ||
        window->y == SDL_WINDOWPOS_CENTERED) {
        y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
        window->y = y - rect.top;
    } else {
        y = window->y + rect.top;
    }
    SetWindowPos(hwnd, top, x, y, h, w, (SWP_NOCOPYBITS | SWP_NOSIZE));
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
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;

    if (data) {
        if (data->created) {
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
        /* FIXME! */
        info->hglrc = NULL;
        return SDL_TRUE;
    } else {
        SDL_SetError("Application not compiled with SDL %d.%d\n",
                     SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
        return SDL_FALSE;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
