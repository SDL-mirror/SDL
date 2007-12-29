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

#include "SDL_win32video.h"
#include "SDL_syswm.h"
#include "SDL_vkeys.h"
#include "../../events/SDL_events_c.h"

/*#define WMMSG_DEBUG*/
#ifdef WMMSG_DEBUG
#include <stdio.h>
#include "wmmsg.h"
#endif

/* Masks for processing the windows KEYDOWN and KEYUP messages */
#define REPEATED_KEYMASK	(1<<30)
#define EXTENDED_KEYMASK	(1<<24)

/* Make sure XBUTTON stuff is defined that isn't in older Platform SDKs... */
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x020C
#endif
#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(w) (HIWORD(w))
#endif

static SDLKey
TranslateKey(WPARAM vkey)
{
    SDLKey key;

    /* FIXME: Assign vkey directly to key if in ASCII range */
    switch (vkey) {
    case VK_BACK:
        key = SDLK_BACKSPACE;
        break;
    case VK_TAB:
        key = SDLK_TAB;
        break;
    case VK_CLEAR:
        key = SDLK_CLEAR;
        break;
    case VK_RETURN:
        key = SDLK_RETURN;
        break;
    case VK_PAUSE:
        key = SDLK_PAUSE;
        break;
    case VK_ESCAPE:
        key = SDLK_ESCAPE;
        break;
    case VK_SPACE:
        key = SDLK_SPACE;
        break;
    case VK_APOSTROPHE:
        key = SDLK_QUOTE;
        break;
    case VK_COMMA:
        key = SDLK_COMMA;
        break;
    case VK_MINUS:
        key = SDLK_MINUS;
        break;
    case VK_PERIOD:
        key = SDLK_PERIOD;
        break;
    case VK_SLASH:
        key = SDLK_SLASH;
        break;
    case VK_0:
        key = SDLK_0;
        break;
    case VK_1:
        key = SDLK_1;
        break;
    case VK_2:
        key = SDLK_2;
        break;
    case VK_3:
        key = SDLK_3;
        break;
    case VK_4:
        key = SDLK_4;
        break;
    case VK_5:
        key = SDLK_5;
        break;
    case VK_6:
        key = SDLK_6;
        break;
    case VK_7:
        key = SDLK_7;
        break;
    case VK_8:
        key = SDLK_8;
        break;
    case VK_9:
        key = SDLK_9;
        break;
    case VK_SEMICOLON:
        key = SDLK_SEMICOLON;
        break;
    case VK_EQUALS:
        key = SDLK_EQUALS;
        break;
    case VK_LBRACKET:
        key = SDLK_LEFTBRACKET;
        break;
    case VK_BACKSLASH:
        key = SDLK_BACKSLASH;
        break;
    case VK_OEM_102:
        key = SDLK_LESS;
        break;
    case VK_RBRACKET:
        key = SDLK_RIGHTBRACKET;
        break;
    case VK_GRAVE:
        key = SDLK_BACKQUOTE;
        break;
    case VK_BACKTICK:
        key = SDLK_BACKQUOTE;
        break;
    case VK_A:
        key = SDLK_a;
        break;
    case VK_B:
        key = SDLK_b;
        break;
    case VK_C:
        key = SDLK_c;
        break;
    case VK_D:
        key = SDLK_d;
        break;
    case VK_E:
        key = SDLK_e;
        break;
    case VK_F:
        key = SDLK_f;
        break;
    case VK_G:
        key = SDLK_g;
        break;
    case VK_H:
        key = SDLK_h;
        break;
    case VK_I:
        key = SDLK_i;
        break;
    case VK_J:
        key = SDLK_j;
        break;
    case VK_K:
        key = SDLK_k;
        break;
    case VK_L:
        key = SDLK_l;
        break;
    case VK_M:
        key = SDLK_m;
        break;
    case VK_N:
        key = SDLK_n;
        break;
    case VK_O:
        key = SDLK_o;
        break;
    case VK_P:
        key = SDLK_p;
        break;
    case VK_Q:
        key = SDLK_q;
        break;
    case VK_R:
        key = SDLK_r;
        break;
    case VK_S:
        key = SDLK_s;
        break;
    case VK_T:
        key = SDLK_t;
        break;
    case VK_U:
        key = SDLK_u;
        break;
    case VK_V:
        key = SDLK_v;
        break;
    case VK_W:
        key = SDLK_w;
        break;
    case VK_X:
        key = SDLK_x;
        break;
    case VK_Y:
        key = SDLK_y;
        break;
    case VK_Z:
        key = SDLK_z;
        break;
    case VK_DELETE:
        key = SDLK_DELETE;
        break;
    case VK_NUMPAD0:
        key = SDLK_KP0;
        break;
    case VK_NUMPAD1:
        key = SDLK_KP1;
        break;
    case VK_NUMPAD2:
        key = SDLK_KP2;
        break;
    case VK_NUMPAD3:
        key = SDLK_KP3;
        break;
    case VK_NUMPAD4:
        key = SDLK_KP4;
        break;
    case VK_NUMPAD5:
        key = SDLK_KP5;
        break;
    case VK_NUMPAD6:
        key = SDLK_KP6;
        break;
    case VK_NUMPAD7:
        key = SDLK_KP7;
        break;
    case VK_NUMPAD8:
        key = SDLK_KP8;
        break;
    case VK_NUMPAD9:
        key = SDLK_KP9;
        break;
    case VK_DECIMAL:
        key = SDLK_KP_PERIOD;
        break;
    case VK_DIVIDE:
        key = SDLK_KP_DIVIDE;
        break;
    case VK_MULTIPLY:
        key = SDLK_KP_MULTIPLY;
        break;
    case VK_SUBTRACT:
        key = SDLK_KP_MINUS;
        break;
    case VK_ADD:
        key = SDLK_KP_PLUS;
        break;
    case VK_UP:
        key = SDLK_UP;
        break;
    case VK_DOWN:
        key = SDLK_DOWN;
        break;
    case VK_RIGHT:
        key = SDLK_RIGHT;
        break;
    case VK_LEFT:
        key = SDLK_LEFT;
        break;
    case VK_INSERT:
        key = SDLK_INSERT;
        break;
    case VK_HOME:
        key = SDLK_HOME;
        break;
    case VK_END:
        key = SDLK_END;
        break;
    case VK_PRIOR:
        key = SDLK_PAGEUP;
        break;
    case VK_NEXT:
        key = SDLK_PAGEDOWN;
        break;
    case VK_F1:
        key = SDLK_F1;
        break;
    case VK_F2:
        key = SDLK_F2;
        break;
    case VK_F3:
        key = SDLK_F3;
        break;
    case VK_F4:
        key = SDLK_F4;
        break;
    case VK_F5:
        key = SDLK_F5;
        break;
    case VK_F6:
        key = SDLK_F6;
        break;
    case VK_F7:
        key = SDLK_F7;
        break;
    case VK_F8:
        key = SDLK_F8;
        break;
    case VK_F9:
        key = SDLK_F9;
        break;
    case VK_F10:
        key = SDLK_F10;
        break;
    case VK_F11:
        key = SDLK_F11;
        break;
    case VK_F12:
        key = SDLK_F12;
        break;
    case VK_F13:
        key = SDLK_F13;
        break;
    case VK_F14:
        key = SDLK_F14;
        break;
    case VK_F15:
        key = SDLK_F15;
        break;
    case VK_NUMLOCK:
        key = SDLK_NUMLOCK;
        break;
    case VK_CAPITAL:
        key = SDLK_CAPSLOCK;
        break;
    case VK_SCROLL:
        key = SDLK_SCROLLOCK;
        break;
    case VK_RSHIFT:
        key = SDLK_RSHIFT;
        break;
    case VK_LSHIFT:
        key = SDLK_LSHIFT;
        break;
    case VK_RCONTROL:
        key = SDLK_RCTRL;
        break;
    case VK_LCONTROL:
        key = SDLK_LCTRL;
        break;
    case VK_RMENU:
        key = SDLK_RALT;
        break;
    case VK_LMENU:
        key = SDLK_LALT;
        break;
    case VK_RWIN:
        key = SDLK_RSUPER;
        break;
    case VK_LWIN:
        key = SDLK_LSUPER;
        break;
    case VK_HELP:
        key = SDLK_HELP;
        break;
    case VK_PRINT:
        key = SDLK_PRINT;
        break;
    case VK_SNAPSHOT:
        key = SDLK_PRINT;
        break;
    case VK_CANCEL:
        key = SDLK_BREAK;
        break;
    case VK_APPS:
        key = SDLK_MENU;
        break;
    default:
        key = SDLK_UNKNOWN;
        break;
    }
    return key;
}

LRESULT CALLBACK
WIN_WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SDL_WindowData *data;

    /* Send a SDL_SYSWMEVENT if the application wants them */
    if (SDL_ProcessEvents[SDL_SYSWMEVENT] == SDL_ENABLE) {
        SDL_SysWMmsg wmmsg;

        SDL_VERSION(&wmmsg.version);
        wmmsg.hwnd = hwnd;
        wmmsg.msg = msg;
        wmmsg.wParam = wParam;
        wmmsg.lParam = lParam;
        SDL_SendSysWMEvent(&wmmsg);
    }

    /* Get the window data for the window */
    data = (SDL_WindowData *) GetProp(hwnd, TEXT("SDL_WindowData"));
    if (!data) {
        return CallWindowProc(DefWindowProc, hwnd, msg, wParam, lParam);
    }
#ifdef WMMSG_DEBUG
    {
        FILE *log = fopen("wmmsg.txt", "a");
        fprintf(log, "Received windows message: %p ", hwnd);
        if (msg > MAX_WMMSG) {
            fprintf(log, "%d", msg);
        } else {
            fprintf(log, "%s", wmtab[msg]);
        }
        fprintf(log, " -- 0x%X, 0x%X\n", wParam, lParam);
        fclose(log);
    }
#endif

    switch (msg) {

    case WM_SHOWWINDOW:
        {
            if (wParam) {
                SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_SHOWN, 0,
                                    0);
            } else {
                SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_HIDDEN, 0,
                                    0);
            }
        }
        break;

    case WM_ACTIVATE:
        {
            int index;
            SDL_Keyboard *keyboard;
            BOOL minimized;

            minimized = HIWORD(wParam);
            index = data->videodata->keyboard;
            keyboard = SDL_GetKeyboard(index);
            if (!minimized && (LOWORD(wParam) != WA_INACTIVE)) {
                SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_SHOWN,
                                    0, 0);
                SDL_SendWindowEvent(data->windowID,
                                    SDL_WINDOWEVENT_RESTORED, 0, 0);
                if (IsZoomed(hwnd)) {
                    SDL_SendWindowEvent(data->windowID,
                                        SDL_WINDOWEVENT_MAXIMIZED, 0, 0);
                }
                if (keyboard && keyboard->focus != data->windowID) {
                    SDL_SetKeyboardFocus(index, data->windowID);
                }
                /* FIXME: Update keyboard state */
            } else {
                if (keyboard && keyboard->focus == data->windowID) {
                    SDL_SetKeyboardFocus(index, 0);
                }
                if (minimized) {
                    SDL_SendWindowEvent(data->windowID,
                                        SDL_WINDOWEVENT_MINIMIZED, 0, 0);
                }
            }
            return (0);
        }
        break;

    case WM_MOUSEMOVE:
        {
            int index;
            SDL_Mouse *mouse;
            int x, y;

            index = data->videodata->mouse;
            mouse = SDL_GetMouse(index);

            if (mouse->focus != data->windowID) {
                TRACKMOUSEEVENT tme;

                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);

                SDL_SetMouseFocus(index, data->windowID);
            }

            /* mouse has moved within the window */
            x = LOWORD(lParam);
            y = HIWORD(lParam);
            if (mouse->relative_mode) {
                int w, h;
                POINT center;
                SDL_GetWindowSize(data->windowID, &w, &h);
                center.x = (w / 2);
                center.y = (h / 2);
                x -= center.x;
                y -= center.y;
                if (x || y) {
                    ClientToScreen(hwnd, &center);
                    SetCursorPos(center.x, center.y);
                    SDL_SendMouseMotion(index, 1, x, y);
                }
            } else {
                SDL_SendMouseMotion(index, 0, x, y);
            }
        }
        return (0);

    case WM_MOUSELEAVE:
        {
            int index;
            SDL_Mouse *mouse;

            index = data->videodata->mouse;
            mouse = SDL_GetMouse(index);

            if (mouse->focus == data->windowID) {
                SDL_SetMouseFocus(index, 0);
            }
        }
        return (0);

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        {
            int xbuttonval = 0;
            int index;
            SDL_Mouse *mouse;
            Uint8 button, state;

            /* DJM:
               We want the SDL window to take focus so that
               it acts like a normal windows "component"
               (e.g. gains keyboard focus on a mouse click).
             */
            SetFocus(hwnd);

            index = data->videodata->mouse;
            mouse = SDL_GetMouse(index);

            /* Figure out which button to use */
            switch (msg) {
            case WM_LBUTTONDOWN:
                button = SDL_BUTTON_LEFT;
                state = SDL_PRESSED;
                break;
            case WM_LBUTTONUP:
                button = SDL_BUTTON_LEFT;
                state = SDL_RELEASED;
                break;
            case WM_MBUTTONDOWN:
                button = SDL_BUTTON_MIDDLE;
                state = SDL_PRESSED;
                break;
            case WM_MBUTTONUP:
                button = SDL_BUTTON_MIDDLE;
                state = SDL_RELEASED;
                break;
            case WM_RBUTTONDOWN:
                button = SDL_BUTTON_RIGHT;
                state = SDL_PRESSED;
                break;
            case WM_RBUTTONUP:
                button = SDL_BUTTON_RIGHT;
                state = SDL_RELEASED;
                break;
            case WM_XBUTTONDOWN:
                xbuttonval = GET_XBUTTON_WPARAM(wParam);
                button = SDL_BUTTON_X1 + xbuttonval - 1;
                state = SDL_PRESSED;
                break;
            case WM_XBUTTONUP:
                xbuttonval = GET_XBUTTON_WPARAM(wParam);
                button = SDL_BUTTON_X1 + xbuttonval - 1;
                state = SDL_RELEASED;
                break;
            default:
                /* Eh? Unknown button? */
                return (0);
            }
            if (state == SDL_PRESSED) {
                /* Grab mouse so we get up events */
                if (++data->mouse_pressed > 0) {
                    SetCapture(hwnd);
                }
            } else {
                /* Release mouse after all up events */
                if (--data->mouse_pressed <= 0) {
                    ReleaseCapture();
                    data->mouse_pressed = 0;
                }
            }

            if (!mouse->relative_mode) {
                int x, y;
                x = LOWORD(lParam);
                y = HIWORD(lParam);
                SDL_SendMouseMotion(index, 0, x, y);
            }
            SDL_SendMouseButton(index, state, button);

            /*
             * MSDN says:
             *  "Unlike the WM_LBUTTONUP, WM_MBUTTONUP, and WM_RBUTTONUP
             *   messages, an application should return TRUE from [an
             *   XBUTTON message] if it processes it. Doing so will allow
             *   software that simulates this message on Microsoft Windows
             *   systems earlier than Windows 2000 to determine whether
             *   the window procedure processed the message or called
             *   DefWindowProc to process it.
             */
            if (xbuttonval > 0) {
                return (TRUE);
            }
        }
        return (0);

    case WM_MOUSEWHEEL:
        {
            int index;
            int motion = (short) HIWORD(wParam);

            index = data->videodata->mouse;
            SDL_SendMouseWheel(index, 0, motion);
        }
        return (0);

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        {
            int index;

            /* Ignore repeated keys */
            if (lParam & REPEATED_KEYMASK) {
                return (0);
            }

            index = data->videodata->keyboard;
            switch (wParam) {
            case VK_CONTROL:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_RCONTROL;
                else
                    wParam = VK_LCONTROL;
                break;
            case VK_SHIFT:
                /* EXTENDED trick doesn't work here */
                {
                    Uint8 *state = SDL_GetKeyState(NULL);
                    if (state[SDLK_LSHIFT] == SDL_RELEASED
                        && (GetKeyState(VK_LSHIFT) & 0x8000)) {
                        wParam = VK_LSHIFT;
                    } else if (state[SDLK_RSHIFT] == SDL_RELEASED
                               && (GetKeyState(VK_RSHIFT) & 0x8000)) {
                        wParam = VK_RSHIFT;
                    } else {
                        /* Probably a key repeat */
                        return (0);
                    }
                }
                break;
            case VK_MENU:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_RMENU;
                else
                    wParam = VK_LMENU;
                break;
            }
            SDL_SendKeyboardKey(index, SDL_PRESSED, (Uint8) HIWORD(lParam),
                                TranslateKey(wParam));
        }
        return (0);

    case WM_SYSKEYUP:
    case WM_KEYUP:
        {
            int index;

            index = data->videodata->keyboard;
            switch (wParam) {
            case VK_CONTROL:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_RCONTROL;
                else
                    wParam = VK_LCONTROL;
                break;
            case VK_SHIFT:
                /* EXTENDED trick doesn't work here */
                {
                    Uint8 *state = SDL_GetKeyState(NULL);
                    if (state[SDLK_LSHIFT] == SDL_PRESSED
                        && !(GetKeyState(VK_LSHIFT) & 0x8000)) {
                        wParam = VK_LSHIFT;
                    } else if (state[SDLK_RSHIFT] == SDL_PRESSED
                               && !(GetKeyState(VK_RSHIFT) & 0x8000)) {
                        wParam = VK_RSHIFT;
                    } else {
                        /* Probably a key repeat */
                        return (0);
                    }
                }
                break;
            case VK_MENU:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_RMENU;
                else
                    wParam = VK_LMENU;
                break;
            }
            /* Windows only reports keyup for print screen */
            if (wParam == VK_SNAPSHOT
                && SDL_GetKeyState(NULL)[SDLK_PRINT] == SDL_RELEASED) {
                SDL_SendKeyboardKey(index, SDL_PRESSED,
                                    (Uint8) HIWORD(lParam),
                                    TranslateKey(wParam));
            }
            SDL_SendKeyboardKey(index, SDL_RELEASED, (Uint8) HIWORD(lParam),
                                TranslateKey(wParam));
        }
        return (0);

    case WM_GETMINMAXINFO:
        {
            MINMAXINFO *info;
            RECT size;
            int x, y;
            int w, h;
            int style;

            /* If we allow resizing, let the resize happen naturally */
            if (SDL_GetWindowFlags(data->windowID) & SDL_WINDOW_RESIZABLE) {
                return (0);
            }

            /* Get the current position of our window */
            GetWindowRect(hwnd, &size);
            x = size.left;
            y = size.top;

            /* Calculate current size of our window */
            SDL_GetWindowSize(data->windowID, &w, &h);
            size.top = 0;
            size.left = 0;
            size.bottom = h;
            size.right = w;

            /* DJM - according to the docs for GetMenu(), the
               return value is undefined if hwnd is a child window.
               Aparently it's too difficult for MS to check
               inside their function, so I have to do it here.
             */
            style = GetWindowLong(hwnd, GWL_STYLE);
            AdjustWindowRect(&size,
                             style,
                             style & WS_CHILDWINDOW ? FALSE : GetMenu(hwnd) !=
                             NULL);

            w = size.right - size.left;
            h = size.bottom - size.top;

            /* Fix our size to the current size */
            info = (MINMAXINFO *) lParam;
            info->ptMaxSize.x = w;
            info->ptMaxSize.y = h;
            info->ptMaxPosition.x = x;
            info->ptMaxPosition.y = y;
            info->ptMinTrackSize.x = w;
            info->ptMinTrackSize.y = h;
            info->ptMaxTrackSize.x = w;
            info->ptMaxTrackSize.y = h;
        }
        return (0);

    case WM_WINDOWPOSCHANGED:
        {
            RECT rect;
            int x, y;
            int w, h;
            Uint32 window_flags;

            GetClientRect(hwnd, &rect);
            ClientToScreen(hwnd, (LPPOINT) & rect);
            ClientToScreen(hwnd, (LPPOINT) & rect + 1);

            window_flags = SDL_GetWindowFlags(data->windowID);
            if ((window_flags & SDL_WINDOW_INPUT_GRABBED) &&
                (window_flags & SDL_WINDOW_INPUT_FOCUS)) {
                ClipCursor(&rect);
            }

            x = rect.left;
            y = rect.top;
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_MOVED, x, y);

            w = rect.right - rect.left;
            h = rect.bottom - rect.top;
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_RESIZED, w,
                                h);
        }
        break;

    case WM_SETCURSOR:
        {
            /*
               Uint16 hittest;

               hittest = LOWORD(lParam);
               if (hittest == HTCLIENT) {
               SetCursor(SDL_hcursor);
               return (TRUE);
               }
             */
        }
        break;

        /* We are about to get palette focus! */
    case WM_QUERYNEWPALETTE:
        {
            /*
               WIN_RealizePalette(current_video);
               return (TRUE);
             */
        }
        break;

        /* Another application changed the palette */
    case WM_PALETTECHANGED:
        {
            /*
               WIN_PaletteChanged(current_video, (HWND) wParam);
             */
        }
        break;

        /* We were occluded, refresh our display */
    case WM_PAINT:
        {
            RECT rect;
            if (GetUpdateRect(hwnd, &rect, FALSE)) {
                ValidateRect(hwnd, &rect);
                SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_EXPOSED,
                                    0, 0);
            }
        }
        return (0);

        /* We'll do our own drawing, prevent flicker */
    case WM_ERASEBKGND:
        {
        }
        return (1);

    case WM_SYSCOMMAND:
        {
            /* Don't start the screensaver or blank the monitor in fullscreen apps */
            if ((wParam & 0xFFF0) == SC_SCREENSAVE ||
                (wParam & 0xFFF0) == SC_MONITORPOWER) {
                if (SDL_GetWindowFlags(data->windowID) &
                    SDL_WINDOW_FULLSCREEN) {
                    return (0);
                }
            }
        }
        break;

    case WM_CLOSE:
        {
            SDL_SendWindowEvent(data->windowID, SDL_WINDOWEVENT_CLOSE, 0, 0);
        }
        return (0);
    }
    return CallWindowProc(data->wndproc, hwnd, msg, wParam, lParam);
}

void
WIN_PumpEvents(_THIS)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static int app_registered = 0;
LPTSTR SDL_Appname = NULL;
Uint32 SDL_Appstyle = 0;
HINSTANCE SDL_Instance = NULL;

/* Register the class for this application */
int
SDL_RegisterApp(char *name, Uint32 style, void *hInst)
{
    WNDCLASS class;

    /* Only do this once... */
    if (app_registered) {
        ++app_registered;
        return (0);
    }
    if (!name && !SDL_Appname) {
        name = "SDL_app";
        SDL_Appstyle = (CS_BYTEALIGNCLIENT | CS_OWNDC);
        SDL_Instance = hInst ? hInst : GetModuleHandle(NULL);
    }

    if (name) {
        SDL_Appname = WIN_UTF8ToString(name);
        SDL_Appstyle = style;
        SDL_Instance = hInst ? hInst : GetModuleHandle(NULL);
    }

    /* Register the application class */
    class.hCursor = NULL;
    class.hIcon = LoadImage(SDL_Instance, SDL_Appname,
                            IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    class.lpszMenuName = NULL;
    class.lpszClassName = SDL_Appname;
    class.hbrBackground = NULL;
    class.hInstance = SDL_Instance;
    class.style = SDL_Appstyle;
    class.lpfnWndProc = DefWindowProc;
    class.cbWndExtra = 0;
    class.cbClsExtra = 0;
    if (!RegisterClass(&class)) {
        SDL_SetError("Couldn't register application class");
        return (-1);
    }

    app_registered = 1;
    return (0);
}

/* Unregisters the windowclass registered in SDL_RegisterApp above. */
void
SDL_UnregisterApp()
{
    WNDCLASS class;

    /* SDL_RegisterApp might not have been called before */
    if (!app_registered) {
        return;
    }
    --app_registered;
    if (app_registered == 0) {
        /* Check for any registered window classes. */
        if (GetClassInfo(SDL_Instance, SDL_Appname, &class)) {
            UnregisterClass(SDL_Appname, SDL_Instance);
        }
        SDL_free(SDL_Appname);
        SDL_Appname = NULL;
    }
}

/* Sets an error message based on GetLastError() */
void
WIN_SetError(const char *prefix)
{
    TCHAR buffer[1024];
    char *message;

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  GetLastError(), 0, buffer, SDL_arraysize(buffer), NULL);

    message = WIN_StringToUTF8(buffer);
    SDL_SetError("%s%s%s", prefix ? prefix : "", prefix ? ":" : "", message);
    SDL_free(message);
}

/* vi: set ts=4 sw=4 expandtab: */
