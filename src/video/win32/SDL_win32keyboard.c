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

#include "SDL_win32video.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_win32.h"

#include <msctf.h>
#include <imm.h>

#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC     0
#endif
#ifndef MAPVK_VSC_TO_VK
#define MAPVK_VSC_TO_VK     1
#endif
#ifndef MAPVK_VK_TO_CHAR
#define MAPVK_VK_TO_CHAR    2
#endif

/* Alphabetic scancodes for PC keyboards */
BYTE alpha_scancodes[26] = {
    30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24,
    25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44
};

BYTE keypad_scancodes[10] = {
    82, 79, 80, 81, 75, 76, 77, 71, 72, 73
};

void IME_Disable(SDL_VideoData *videodata, HWND hwnd);
void IME_Enable(SDL_VideoData *videodata, HWND hwnd);
void IME_Init(SDL_VideoData *videodata, HWND hwnd);
void IME_Quit(SDL_VideoData *videodata);

void
WIN_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int i;

    /* Make sure the alpha scancodes are correct.  T isn't usually remapped */
    if (MapVirtualKey('T', MAPVK_VK_TO_VSC) != alpha_scancodes['T' - 'A']) {
#if 0
        printf
            ("Fixing alpha scancode map, assuming US QWERTY layout!\nPlease send the following 26 lines of output to the SDL mailing list <sdl@libsdl.org>, including a description of your keyboard hardware.\n");
#endif
        for (i = 0; i < SDL_arraysize(alpha_scancodes); ++i) {
            alpha_scancodes[i] = MapVirtualKey('A' + i, MAPVK_VK_TO_VSC);
#if 0
            printf("%d = %d\n", i, alpha_scancodes[i]);
#endif
        }
    }
    if (MapVirtualKey(VK_NUMPAD0, MAPVK_VK_TO_VSC) != keypad_scancodes[0]) {
#if 0
        printf
            ("Fixing keypad scancode map!\nPlease send the following 10 lines of output to the SDL mailing list <sdl@libsdl.org>, including a description of your keyboard hardware.\n");
#endif
        for (i = 0; i < SDL_arraysize(keypad_scancodes); ++i) {
            keypad_scancodes[i] =
                MapVirtualKey(VK_NUMPAD0 + i, MAPVK_VK_TO_VSC);
#if 0
            printf("%d = %d\n", i, keypad_scancodes[i]);
#endif
        }
    }

    data->key_layout = win32_scancode_table;

    data->ime_com_initialized = SDL_FALSE;
    data->ime_thread_mgr = 0;
    data->ime_initialized = SDL_FALSE;
    data->ime_enabled = SDL_FALSE;
    data->ime_available = SDL_FALSE;
    data->ime_hwnd_main = 0;
    data->ime_hwnd_current = 0;
    data->ime_himc = 0;

    WIN_UpdateKeymap();

    SDL_SetScancodeName(SDL_SCANCODE_APPLICATION, "Menu");
    SDL_SetScancodeName(SDL_SCANCODE_LGUI, "Left Windows");
    SDL_SetScancodeName(SDL_SCANCODE_RGUI, "Right Windows");
}

void
WIN_UpdateKeymap()
{
    int i;
    SDL_scancode scancode;
    SDLKey keymap[SDL_NUM_SCANCODES];

    SDL_GetDefaultKeymap(keymap);

    for (i = 0; i < SDL_arraysize(win32_scancode_table); i++) {

        /* Make sure this scancode is a valid character scancode */
        scancode = win32_scancode_table[i];
        if (scancode == SDL_SCANCODE_UNKNOWN ||
            (keymap[scancode] & SDLK_SCANCODE_MASK)) {
            continue;
        }

        /* Alphabetic keys are handled specially, since Windows remaps them */
        if (i >= 'A' && i <= 'Z') {
            BYTE vsc = alpha_scancodes[i - 'A'];
            keymap[scancode] = MapVirtualKey(vsc, MAPVK_VSC_TO_VK) + 0x20;
        } else {
            keymap[scancode] = (MapVirtualKey(i, MAPVK_VK_TO_CHAR) & 0x7FFF);
        }
    }
    SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
}

void
WIN_QuitKeyboard(_THIS)
{
    IME_Quit((SDL_VideoData *)_this->driverdata);
}

void
WIN_StartTextInput(_THIS)
{
    SDL_Window *window = SDL_GetKeyboardFocus();
    if (window)
    {
        HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
        SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;
        IME_Init(videodata, hwnd);
        IME_Enable(videodata, hwnd);
    }
}

void
WIN_StopTextInput(_THIS)
{
    SDL_Window *window = SDL_GetKeyboardFocus();
    if (window)
    {
        HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
        SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;
        IME_Init(videodata, hwnd);
        IME_Disable(videodata, hwnd);
    }
}

void
WIN_SetTextInputRect(_THIS, SDL_Rect *rect)
{

}

void
IME_Disable(SDL_VideoData *videodata, HWND hwnd)
{
    if (!videodata->ime_initialized || !videodata->ime_hwnd_current)
        return;

    if (videodata->ime_hwnd_current == videodata->ime_hwnd_main)
        ImmAssociateContext(videodata->ime_hwnd_current, NULL);

    videodata->ime_enabled = SDL_FALSE;
}

void
IME_Enable(SDL_VideoData *videodata, HWND hwnd)
{
    if (!videodata->ime_initialized || !videodata->ime_hwnd_current)
        return;

    if (!videodata->ime_available) {
        IME_Disable(videodata, hwnd);
        return;
    }
    if (videodata->ime_hwnd_current == videodata->ime_hwnd_main)
        ImmAssociateContext(videodata->ime_hwnd_current, videodata->ime_himc);

    videodata->ime_enabled = SDL_TRUE;
}

void
IME_Init(SDL_VideoData *videodata, HWND hwnd)
{
    if (videodata->ime_initialized)
        return;

    videodata->ime_hwnd_main = hwnd;
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
        videodata->ime_com_initialized = SDL_TRUE;
        CoCreateInstance(&CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, &IID_ITfThreadMgr, &videodata->ime_thread_mgr);
    }
    videodata->ime_initialized = SDL_TRUE;
    videodata->ime_hwnd_current = videodata->ime_hwnd_main;
    if (videodata->ime_thread_mgr) {
        struct ITfDocumentMgr *document_mgr = 0;
        if (SUCCEEDED(videodata->ime_thread_mgr->lpVtbl->AssociateFocus(videodata->ime_thread_mgr, hwnd, NULL, &document_mgr))) {
            if (document_mgr)
                document_mgr->lpVtbl->Release(document_mgr);
        }
    }
    videodata->ime_himc = ImmGetContext(hwnd);
    ImmReleaseContext(hwnd, videodata->ime_himc);
    if (!videodata->ime_himc) {
        videodata->ime_available = SDL_FALSE;
        IME_Disable(videodata, hwnd);
        return;
    }
    videodata->ime_available = SDL_TRUE;
    IME_Disable(videodata, hwnd);
}

void
IME_Quit(SDL_VideoData *videodata)
{
    if (!videodata->ime_initialized)
        return;

    if (videodata->ime_hwnd_main)
        ImmAssociateContext(videodata->ime_hwnd_main, videodata->ime_himc);

    videodata->ime_hwnd_main = 0;
    videodata->ime_himc = 0;
    if (videodata->ime_thread_mgr)
    {
        videodata->ime_thread_mgr->lpVtbl->Release(videodata->ime_thread_mgr);
        videodata->ime_thread_mgr = 0;
    }
    if (videodata->ime_com_initialized)
    {
        CoUninitialize();
        videodata->ime_com_initialized = SDL_FALSE;
    }
    videodata->ime_initialized = SDL_FALSE;
}

SDL_bool
IME_HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM *lParam, SDL_VideoData *videodata)
{
    SDL_bool trap = SDL_FALSE;
    HIMC himc = 0;
    WCHAR Buffer[SDL_TEXTINPUTEVENT_TEXT_SIZE / 2];
    if (!videodata->ime_initialized || !videodata->ime_available || !videodata->ime_enabled)
        return SDL_FALSE;

    switch (msg)
    {
    case WM_INPUTLANGCHANGE:
        break;
    case WM_IME_SETCONTEXT:
        *lParam = 0;
        break;
    case WM_IME_STARTCOMPOSITION:
        trap = SDL_TRUE;
        break;
    case WM_IME_COMPOSITION:
        trap = SDL_TRUE;
        himc = ImmGetContext(hwnd);
        if (*lParam & GCS_RESULTSTR)
        {
            LONG Length = 0;
            char *s = 0;
            Length = ImmGetCompositionStringW(himc, GCS_RESULTSTR, Buffer, sizeof(Buffer) * sizeof(Buffer[0]));
            Buffer[Length / sizeof(Buffer[0])] = 0;
            s = WIN_StringToUTF8(Buffer);
            SDL_SendKeyboardText(s);
            SDL_free(s);
        }
        if (*lParam & GCS_COMPSTR)
        {
            LONG Length = 0;
            DWORD Cursor = 0;
            char *s = 0;
            Length = ImmGetCompositionStringW(himc, GCS_COMPSTR, Buffer, sizeof(Buffer) * sizeof(Buffer[0]));
            Buffer[Length / sizeof(Buffer[0])] = 0;
            s = WIN_StringToUTF8(Buffer);
            Cursor = LOWORD(ImmGetCompositionStringW(himc, GCS_CURSORPOS, 0, 0));
            SDL_SendEditingText(s, Cursor, 0);
            SDL_free(s);
        }
        ImmReleaseContext(hwnd, himc);
        break;
    case WM_IME_ENDCOMPOSITION:
        SDL_SendKeyboardText("");
        break;
    case WM_IME_NOTIFY:
        switch (wParam)
        {
        case IMN_SETCONVERSIONMODE:
            break;
        case IMN_SETOPENSTATUS:
            break;
        case IMN_OPENCANDIDATE:
        case IMN_CHANGECANDIDATE:
            trap = SDL_TRUE;
            break;
        case IMN_CLOSECANDIDATE:
            trap = SDL_TRUE;
            break;
        case IMN_PRIVATE:
            break;
        default:
            trap = SDL_TRUE;
            break;
        }
        break;
    }
    return trap;
}

/* vi: set ts=4 sw=4 expandtab: */
