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

#include <imm.h>
#include <oleauto.h>

static void IME_Init(SDL_VideoData *videodata, HWND hwnd);
static void IME_Enable(SDL_VideoData *videodata, HWND hwnd);
static void IME_Disable(SDL_VideoData *videodata, HWND hwnd);
static void IME_Quit(SDL_VideoData *videodata);

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
    data->ime_threadmgr = 0;
    data->ime_initialized = SDL_FALSE;
    data->ime_enabled = SDL_FALSE;
    data->ime_available = SDL_FALSE;
    data->ime_hwnd_main = 0;
    data->ime_hwnd_current = 0;
    data->ime_himc = 0;
    data->ime_composition[0] = 0;
    data->ime_readingstring[0] = 0;
    data->ime_cursor = 0;
    data->ime_hkl = 0;
    data->ime_himm32 = 0;
    data->GetReadingString = 0;
    data->ShowReadingWindow = 0;
    data->ImmLockIMC = 0;
    data->ImmUnlockIMC = 0;
    data->ImmLockIMCC = 0;
    data->ImmUnlockIMCC = 0;
    data->ime_uiless = SDL_FALSE;
    data->ime_threadmgrex = 0;
    data->ime_uielemsinkcookie = TF_INVALID_COOKIE;
    data->ime_alpnsinkcookie = TF_INVALID_COOKIE;
    data->ime_openmodesinkcookie = TF_INVALID_COOKIE;
    data->ime_convmodesinkcookie = TF_INVALID_COOKIE;
    data->ime_uielemsink = 0;
    data->ime_ippasink = 0;

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
        if (scancode == SDL_SCANCODE_UNKNOWN || keymap[scancode] >= 127) {
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
    if (window) {
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
    if (window) {
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

#define LANG_CHT MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANG_CHS MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)

#define MAKEIMEVERSION(major,minor) ((DWORD) (((BYTE)(major) << 24) | ((BYTE)(minor) << 16) ))
#define IMEID_VER(id) ((id) & 0xffff0000)
#define IMEID_LANG(id) ((id) & 0x0000ffff)

#define CHT_HKL_DAYI            ((HKL)0xE0060404)
#define CHT_HKL_NEW_PHONETIC    ((HKL)0xE0080404)
#define CHT_HKL_NEW_CHANG_JIE   ((HKL)0xE0090404)
#define CHT_HKL_NEW_QUICK       ((HKL)0xE00A0404)
#define CHT_HKL_HK_CANTONESE    ((HKL)0xE00B0404)
#define CHT_IMEFILENAME1        "TINTLGNT.IME"
#define CHT_IMEFILENAME2        "CINTLGNT.IME"
#define CHT_IMEFILENAME3        "MSTCIPHA.IME"
#define IMEID_CHT_VER42         (LANG_CHT | MAKEIMEVERSION(4, 2))
#define IMEID_CHT_VER43         (LANG_CHT | MAKEIMEVERSION(4, 3))
#define IMEID_CHT_VER44         (LANG_CHT | MAKEIMEVERSION(4, 4))
#define IMEID_CHT_VER50         (LANG_CHT | MAKEIMEVERSION(5, 0))
#define IMEID_CHT_VER51         (LANG_CHT | MAKEIMEVERSION(5, 1))
#define IMEID_CHT_VER52         (LANG_CHT | MAKEIMEVERSION(5, 2))
#define IMEID_CHT_VER60         (LANG_CHT | MAKEIMEVERSION(6, 0))
#define IMEID_CHT_VER_VISTA     (LANG_CHT | MAKEIMEVERSION(7, 0))

#define CHS_HKL                 ((HKL)0xE00E0804)
#define CHS_IMEFILENAME1        "PINTLGNT.IME"
#define CHS_IMEFILENAME2        "MSSCIPYA.IME"
#define IMEID_CHS_VER41         (LANG_CHS | MAKEIMEVERSION(4, 1))
#define IMEID_CHS_VER42         (LANG_CHS | MAKEIMEVERSION(4, 2))
#define IMEID_CHS_VER53         (LANG_CHS | MAKEIMEVERSION(5, 3))

#define LANG() LOWORD((videodata->ime_hkl))
#define PRIMLANG() ((WORD)PRIMARYLANGID(LANG()))
#define SUBLANG() SUBLANGID(LANG())

static void IME_UpdateInputLocale(SDL_VideoData *videodata);
static void IME_ClearComposition(SDL_VideoData *videodata);
static void IME_SetWindow(SDL_VideoData* videodata, HWND hwnd);
static void IME_SetupAPI(SDL_VideoData *videodata);
static DWORD IME_GetId(SDL_VideoData *videodata, UINT uIndex);
static void IME_SendEditingEvent(SDL_VideoData *videodata);
#define SDL_IsEqualIID(riid1, riid2) SDL_IsEqualGUID(riid1, riid2)
#define SDL_IsEqualGUID(rguid1, rguid2) (!SDL_memcmp(rguid1, rguid2, sizeof(GUID)))

static SDL_bool UILess_SetupSinks(SDL_VideoData *videodata);
static void UILess_ReleaseSinks(SDL_VideoData *videodata);
static void UILess_EnableUIUpdates(SDL_VideoData *videodata);
static void UILess_DisableUIUpdates(SDL_VideoData *videodata);

static void
IME_Init(SDL_VideoData *videodata, HWND hwnd)
{
    if (videodata->ime_initialized)
        return;

    videodata->ime_hwnd_main = hwnd;
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
        videodata->ime_com_initialized = SDL_TRUE;
        CoCreateInstance(&CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, &IID_ITfThreadMgr, &videodata->ime_threadmgr);
    }
    videodata->ime_initialized = SDL_TRUE;
    videodata->ime_himm32 = LoadLibraryA("imm32.dll");
    if (!videodata->ime_himm32) {
        videodata->ime_available = SDL_FALSE;
        return;
    }
    videodata->ImmLockIMC = (LPINPUTCONTEXT2 (WINAPI *)(HIMC))GetProcAddress(videodata->ime_himm32, "ImmLockIMC");
    videodata->ImmUnlockIMC = (BOOL (WINAPI *)(HIMC))GetProcAddress(videodata->ime_himm32, "ImmUnlockIMC");
    videodata->ImmLockIMCC = (LPVOID (WINAPI *)(HIMCC))GetProcAddress(videodata->ime_himm32, "ImmLockIMCC");
    videodata->ImmUnlockIMCC = (BOOL (WINAPI *)(HIMCC))GetProcAddress(videodata->ime_himm32, "ImmUnlockIMCC");

    IME_SetWindow(videodata, hwnd);
    videodata->ime_himc = ImmGetContext(hwnd);
    ImmReleaseContext(hwnd, videodata->ime_himc);
    if (!videodata->ime_himc) {
        videodata->ime_available = SDL_FALSE;
        IME_Disable(videodata, hwnd);
        return;
    }
    videodata->ime_available = SDL_TRUE;
    IME_UpdateInputLocale(videodata);
    IME_SetupAPI(videodata);
    videodata->ime_uiless = UILess_SetupSinks(videodata);
    IME_UpdateInputLocale(videodata);
    IME_Disable(videodata, hwnd);
}

static void
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
    IME_UpdateInputLocale(videodata);
    UILess_EnableUIUpdates(videodata);
}

static void
IME_Disable(SDL_VideoData *videodata, HWND hwnd)
{
    if (!videodata->ime_initialized || !videodata->ime_hwnd_current)
        return;

    IME_ClearComposition(videodata);
    if (videodata->ime_hwnd_current == videodata->ime_hwnd_main)
        ImmAssociateContext(videodata->ime_hwnd_current, NULL);

    videodata->ime_enabled = SDL_FALSE;
    UILess_DisableUIUpdates(videodata);
}

static void
IME_Quit(SDL_VideoData *videodata)
{
    if (!videodata->ime_initialized)
        return;

    UILess_ReleaseSinks(videodata);
    if (videodata->ime_hwnd_main)
        ImmAssociateContext(videodata->ime_hwnd_main, videodata->ime_himc);

    videodata->ime_hwnd_main = 0;
    videodata->ime_himc = 0;
    if (videodata->ime_himm32) {
        FreeLibrary(videodata->ime_himm32);
        videodata->ime_himm32 = 0;
    }
    if (videodata->ime_threadmgr) {
        videodata->ime_threadmgr->lpVtbl->Release(videodata->ime_threadmgr);
        videodata->ime_threadmgr = 0;
    }
    if (videodata->ime_com_initialized) {
        CoUninitialize();
        videodata->ime_com_initialized = SDL_FALSE;
    }
    videodata->ime_initialized = SDL_FALSE;
}

static void
IME_GetReadingString(SDL_VideoData *videodata, HWND hwnd)
{
    DWORD id = 0;
    HIMC himc = 0;
    WCHAR buffer[16];
    WCHAR *s = buffer;
    DWORD len = 0;
    DWORD err = 0;
    BOOL vertical = FALSE;
    UINT maxuilen = 0;
    static OSVERSIONINFOA osversion = {0};
    if (videodata->ime_uiless)
        return;

    videodata->ime_readingstring[0] = 0;
    if (!osversion.dwOSVersionInfoSize) {
        osversion.dwOSVersionInfoSize = sizeof(osversion);
        GetVersionExA(&osversion);
    }
    id = IME_GetId(videodata, 0);
    if (!id)
        return;

    himc = ImmGetContext(hwnd);
    if (!himc)
        return;

    if (videodata->GetReadingString) {
        len = videodata->GetReadingString(himc, 0, 0, &err, &vertical, &maxuilen);
        if (len) {
            if (len > SDL_arraysize(buffer))
                len = SDL_arraysize(buffer);

            len = videodata->GetReadingString(himc, len, s, &err, &vertical, &maxuilen);
        }
        SDL_wcslcpy(videodata->ime_readingstring, s, len);
    }
    else {
        LPINPUTCONTEXT2 lpimc = videodata->ImmLockIMC(himc);
        LPBYTE p = 0;
        s = 0;
        switch (id)
        {
        case IMEID_CHT_VER42:
        case IMEID_CHT_VER43:
        case IMEID_CHT_VER44:
            p = *(LPBYTE *)((LPBYTE)videodata->ImmLockIMCC(lpimc->hPrivate) + 24);
            if (!p)
                break;

            len = *(DWORD *)(p + 7*4 + 32*4);
            s = (WCHAR *)(p + 56);
            break;
        case IMEID_CHT_VER51:
        case IMEID_CHT_VER52:
        case IMEID_CHS_VER53:
            p = *(LPBYTE *)((LPBYTE)videodata->ImmLockIMCC(lpimc->hPrivate) + 4);
            if (!p)
                break;

            p = *(LPBYTE *)((LPBYTE)p + 1*4 + 5*4);
            if (!p)
                break;

            len = *(DWORD *)(p + 1*4 + (16*2+2*4) + 5*4 + 16*2);
            s = (WCHAR *)(p + 1*4 + (16*2+2*4) + 5*4);
            break;
        case IMEID_CHS_VER41:
            {
                int offset = (IME_GetId(videodata, 1) >= 0x00000002) ? 8 : 7;
                p = *(LPBYTE *)((LPBYTE)videodata->ImmLockIMCC(lpimc->hPrivate) + offset * 4);
                if (!p)
                    break;

                len = *(DWORD *)(p + 7*4 + 16*2*4);
                s = (WCHAR *)(p + 6*4 + 16*2*1);
            }
            break;
        case IMEID_CHS_VER42:
            if (osversion.dwPlatformId != VER_PLATFORM_WIN32_NT)
                break;

            p = *(LPBYTE *)((LPBYTE)videodata->ImmLockIMCC(lpimc->hPrivate) + 1*4 + 1*4 + 6*4);
            if (!p)
                break;

            len = *(DWORD *)(p + 1*4 + (16*2+2*4) + 5*4 + 16*2);
            s = (WCHAR *)(p + 1*4 + (16*2+2*4) + 5*4);
            break;
        }
        if (s)
            SDL_wcslcpy(videodata->ime_readingstring, s, len + 1);

        videodata->ImmUnlockIMCC(lpimc->hPrivate);
        videodata->ImmUnlockIMC(himc);
    }
    ImmReleaseContext(hwnd, himc);
    IME_SendEditingEvent(videodata);
}

static void
IME_InputLangChanged(SDL_VideoData *videodata)
{
    UINT lang = PRIMLANG();
    HWND hwndime = 0;
    IME_UpdateInputLocale(videodata);
    IME_SetupAPI(videodata);
    if (lang != PRIMLANG()) {
        IME_ClearComposition(videodata);
    }
    hwndime = ImmGetDefaultIMEWnd(videodata->ime_hwnd_current);
    if (hwndime) {
        SendMessageA(hwndime, WM_IME_CONTROL, IMC_OPENSTATUSWINDOW, 0);
        SendMessageA(hwndime, WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0);
    }
}

static DWORD
IME_GetId(SDL_VideoData *videodata, UINT uIndex)
{
    static HKL hklprev = 0;
    static DWORD dwRet[2] = {0};
    DWORD dwVerSize = 0;
    DWORD dwVerHandle = 0;
    LPVOID lpVerBuffer = 0;
    LPVOID lpVerData = 0;
    UINT cbVerData = 0;
    char szTemp[256];
    HKL hkl = 0;
    DWORD dwLang = 0;
    if (uIndex >= sizeof(dwRet) / sizeof(dwRet[0]))
        return 0;

    hkl = videodata->ime_hkl;
    if (hklprev == hkl)
        return dwRet[uIndex];

    hklprev = hkl;
    dwLang = ((DWORD)hkl & 0xffff);
    if (videodata->ime_uiless && LANG() == LANG_CHT) {
        dwRet[0] = IMEID_CHT_VER_VISTA;
        dwRet[1] = 0;
        return dwRet[0];
    }
    if (hkl != CHT_HKL_NEW_PHONETIC
        && hkl != CHT_HKL_NEW_CHANG_JIE
        && hkl != CHT_HKL_NEW_QUICK
        && hkl != CHT_HKL_HK_CANTONESE
        && hkl != CHS_HKL) {
        dwRet[0] = dwRet[1] = 0;
        return dwRet[uIndex];
    }
    if (ImmGetIMEFileNameA(hkl, szTemp, sizeof(szTemp) - 1) <= 0) {
        dwRet[0] = dwRet[1] = 0;
        return dwRet[uIndex];
    }
    if (!videodata->GetReadingString) {
        #define LCID_INVARIANT MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)
        if (CompareStringA(LCID_INVARIANT, NORM_IGNORECASE, szTemp, -1, CHT_IMEFILENAME1, -1) != 2
            && CompareStringA(LCID_INVARIANT, NORM_IGNORECASE, szTemp, -1, CHT_IMEFILENAME2, -1) != 2
            && CompareStringA(LCID_INVARIANT, NORM_IGNORECASE, szTemp, -1, CHT_IMEFILENAME3, -1) != 2
            && CompareStringA(LCID_INVARIANT, NORM_IGNORECASE, szTemp, -1, CHS_IMEFILENAME1, -1) != 2
            && CompareStringA(LCID_INVARIANT, NORM_IGNORECASE, szTemp, -1, CHS_IMEFILENAME2, -1) != 2) {
            dwRet[0] = dwRet[1] = 0;
            return dwRet[uIndex];
        }
        #undef LCID_INVARIANT
        dwVerSize = GetFileVersionInfoSizeA(szTemp, &dwVerHandle);
        if (dwVerSize) {
            lpVerBuffer = SDL_malloc(dwVerSize);
            if (lpVerBuffer) {
                if (GetFileVersionInfoA(szTemp, dwVerHandle, dwVerSize, lpVerBuffer)) {
                    if (VerQueryValueA(lpVerBuffer, "\\", &lpVerData, &cbVerData)) {
                        #define pVerFixedInfo   ((VS_FIXEDFILEINFO FAR*)lpVerData)
                        DWORD dwVer = pVerFixedInfo->dwFileVersionMS;
                        dwVer = (dwVer & 0x00ff0000) << 8 | (dwVer & 0x000000ff) << 16;
                        if (videodata->GetReadingString ||
                            dwLang == LANG_CHT && (
                            dwVer == MAKEIMEVERSION(4, 2) ||
                            dwVer == MAKEIMEVERSION(4, 3) ||
                            dwVer == MAKEIMEVERSION(4, 4) ||
                            dwVer == MAKEIMEVERSION(5, 0) ||
                            dwVer == MAKEIMEVERSION(5, 1) ||
                            dwVer == MAKEIMEVERSION(5, 2) ||
                            dwVer == MAKEIMEVERSION(6, 0))
                            ||
                            dwLang == LANG_CHS && (
                            dwVer == MAKEIMEVERSION(4, 1) ||
                            dwVer == MAKEIMEVERSION(4, 2) ||
                            dwVer == MAKEIMEVERSION(5, 3))) {
                            dwRet[0] = dwVer | dwLang;
                            dwRet[1] = pVerFixedInfo->dwFileVersionLS;
                            SDL_free(lpVerBuffer);
                            return dwRet[0];
                        }
                        #undef pVerFixedInfo
                    }
                }
            }
            SDL_free(lpVerBuffer);
        }
    }
    dwRet[0] = dwRet[1] = 0;
    return dwRet[uIndex];
}

static void
IME_SetupAPI(SDL_VideoData *videodata)
{
    char ime_file[MAX_PATH + 1];
    HMODULE hime = 0;
    HKL hkl = 0;
    videodata->GetReadingString = 0;
    videodata->ShowReadingWindow = 0;
    if (videodata->ime_uiless)
        return;

    hkl = videodata->ime_hkl;
    if (ImmGetIMEFileNameA(hkl, ime_file, sizeof(ime_file) - 1) <= 0)
        return;

    hime = LoadLibraryA(ime_file);
    if (!hime)
        return;

    videodata->GetReadingString = (UINT (WINAPI *)(HIMC, UINT, LPWSTR, PINT, BOOL*, PUINT))
        GetProcAddress(hime, "GetReadingString");
    videodata->ShowReadingWindow = (BOOL (WINAPI *)(HIMC, BOOL))
        GetProcAddress(hime, "ShowReadingWindow");

    if (videodata->ShowReadingWindow) {
        HIMC himc = ImmGetContext(videodata->ime_hwnd_current);
        if (himc) {
            videodata->ShowReadingWindow(himc, FALSE);
            ImmReleaseContext(videodata->ime_hwnd_current, himc);
        }
    }
}

static void
IME_SetWindow(SDL_VideoData* videodata, HWND hwnd)
{
    videodata->ime_hwnd_current = hwnd;
    if (videodata->ime_threadmgr) {
        struct ITfDocumentMgr *document_mgr = 0;
        if (SUCCEEDED(videodata->ime_threadmgr->lpVtbl->AssociateFocus(videodata->ime_threadmgr, hwnd, NULL, &document_mgr))) {
            if (document_mgr)
                document_mgr->lpVtbl->Release(document_mgr);
        }
    }
}

static void
IME_UpdateInputLocale(SDL_VideoData *videodata)
{
    static HKL hklprev = 0;
    videodata->ime_hkl = GetKeyboardLayout(0);
    if (hklprev == videodata->ime_hkl)
        return;

    hklprev = videodata->ime_hkl;
}

static void
IME_ClearComposition(SDL_VideoData *videodata)
{
    HIMC himc = 0;
    if (!videodata->ime_initialized)
        return;

    himc = ImmGetContext(videodata->ime_hwnd_current);
    if (!himc)
        return;

    ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    if (videodata->ime_uiless)
        ImmSetCompositionString(himc, SCS_SETSTR, TEXT(""), sizeof(TCHAR), TEXT(""), sizeof(TCHAR));

    ImmNotifyIME(himc, NI_CLOSECANDIDATE, 0, 0);
    ImmReleaseContext(videodata->ime_hwnd_current, himc);
    SDL_SendEditingText("", 0, 0);
}

static void
IME_ClearEditing(SDL_VideoData *videodata)
{

}

static void
IME_GetCompositionString(SDL_VideoData *videodata, HIMC himc, DWORD string)
{
    LONG length = ImmGetCompositionStringW(himc, string, videodata->ime_composition, sizeof(videodata->ime_composition));
    if (length < 0)
        length = 0;

    length /= sizeof(videodata->ime_composition[0]);
    videodata->ime_cursor = LOWORD(ImmGetCompositionStringW(himc, GCS_CURSORPOS, 0, 0));
    if (videodata->ime_composition[videodata->ime_cursor] == 0x3000) {
        int i;
        for (i = videodata->ime_cursor + 1; i < length; ++i)
            videodata->ime_composition[i - 1] = videodata->ime_composition[i];

        --length;
    }
    videodata->ime_composition[length] = 0;
}

static void
IME_SendInputEvent(SDL_VideoData *videodata)
{
    char *s = 0;
    s = WIN_StringToUTF8(videodata->ime_composition);
    SDL_SendKeyboardText(s);
    SDL_free(s);

    videodata->ime_composition[0] = 0;
    videodata->ime_readingstring[0] = 0;
    videodata->ime_cursor = 0;
}

static void
IME_SendEditingEvent(SDL_VideoData *videodata)
{
    char *s = 0;
    WCHAR buffer[SDL_TEXTEDITINGEVENT_TEXT_SIZE];
    buffer[0] = 0;
    if (videodata->ime_readingstring[0]) {
        size_t len = SDL_min(SDL_wcslen(videodata->ime_composition), (size_t)videodata->ime_cursor);
        SDL_wcslcpy(buffer, videodata->ime_composition, len + 1);
        SDL_wcslcat(buffer, videodata->ime_readingstring, sizeof(buffer));
        SDL_wcslcat(buffer, &videodata->ime_composition[len], sizeof(buffer) - len);
    }
    else {
        SDL_wcslcpy(buffer, videodata->ime_composition, sizeof(videodata->ime_composition));
    }
    s = WIN_StringToUTF8(buffer);
    SDL_SendEditingText(s, videodata->ime_cursor + SDL_wcslen(videodata->ime_readingstring), 0);
    SDL_free(s);
}

SDL_bool
IME_HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM *lParam, SDL_VideoData *videodata)
{
    SDL_bool trap = SDL_FALSE;
    HIMC himc = 0;
    if (!videodata->ime_initialized || !videodata->ime_available || !videodata->ime_enabled)
        return SDL_FALSE;

    switch (msg)
    {
    case WM_INPUTLANGCHANGE:
        //IME_InputLangChanged(videodata);
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
        if (*lParam & GCS_RESULTSTR) {
            IME_GetCompositionString(videodata, himc, GCS_RESULTSTR);
            IME_SendInputEvent(videodata);
        }
        if (*lParam & GCS_COMPSTR) {
            if (!videodata->ime_uiless)
                videodata->ime_readingstring[0] = 0;

            IME_GetCompositionString(videodata, himc, GCS_COMPSTR);
            IME_SendEditingEvent(videodata);
        }
        ImmReleaseContext(hwnd, himc);
        break;
    case WM_IME_ENDCOMPOSITION:
        videodata->ime_composition[0] = 0;
        videodata->ime_readingstring[0] = 0;
        videodata->ime_cursor = 0;
        SDL_SendEditingText("", 0, 0);
        break;
    case WM_IME_NOTIFY:
        switch (wParam)
        {
        case IMN_SETCONVERSIONMODE:
        case IMN_SETOPENSTATUS:
            IME_UpdateInputLocale(videodata);
            break;
        case IMN_OPENCANDIDATE:
        case IMN_CHANGECANDIDATE:
            trap = SDL_TRUE;
            break;
        case IMN_CLOSECANDIDATE:
            trap = SDL_TRUE;
            break;
        case IMN_PRIVATE:
            {
                DWORD dwId = IME_GetId(videodata, 0);
                IME_GetReadingString(videodata, hwnd);
                switch (dwId)
                {
                case IMEID_CHT_VER42:
                case IMEID_CHT_VER43:
                case IMEID_CHT_VER44:
                case IMEID_CHS_VER41:
                case IMEID_CHS_VER42:
                    if (*lParam == 1 || *lParam == 2)
                        trap = SDL_TRUE;

                    break;
                case IMEID_CHT_VER50:
                case IMEID_CHT_VER51:
                case IMEID_CHT_VER52:
                case IMEID_CHT_VER60:
                case IMEID_CHS_VER53:
                    if (*lParam == 16
                        || *lParam == 17
                        || *lParam == 26
                        || *lParam == 27
                        || *lParam == 28)
                        trap = SDL_TRUE;
                    break;
                }
            }
            break;
        default:
            trap = SDL_TRUE;
            break;
        }
        break;
    }
    return trap;
}

STDMETHODIMP_(ULONG) TSFSink_AddRef(TSFSink *sink)
{
    return ++sink->refcount;
}

STDMETHODIMP_(ULONG)TSFSink_Release(TSFSink *sink)
{
    --sink->refcount;
    if (sink->refcount == 0)
    {
        SDL_free(sink);
        return 0;
    }
    return sink->refcount;
}

STDMETHODIMP UIElementSink_QueryInterface(TSFSink *sink, REFIID riid, PVOID *ppv)
{
    if (!ppv)
        return E_INVALIDARG;

    *ppv = 0;
    if (SDL_IsEqualIID(riid, &IID_IUnknown))
        *ppv = (IUnknown *)sink;
    else if (SDL_IsEqualIID(riid, &IID_ITfUIElementSink))
        *ppv = (ITfUIElementSink *)sink;

    if (*ppv) {
        TSFSink_AddRef(sink);
        return S_OK;
    }
    return E_NOINTERFACE;
}

ITfUIElement *UILess_GetUIElement(SDL_VideoData *videodata, DWORD dwUIElementId)
{
    ITfUIElementMgr *puiem = 0;
    ITfUIElement *pelem = 0;
    ITfThreadMgrEx *threadmgrex = videodata->ime_threadmgrex;

    if (SUCCEEDED(threadmgrex->lpVtbl->QueryInterface(threadmgrex, &IID_ITfUIElementMgr, (LPVOID *)&puiem))) {
        puiem->lpVtbl->GetUIElement(puiem, dwUIElementId, &pelem);
        puiem->lpVtbl->Release(puiem);
    }
    return pelem;
}

STDMETHODIMP UIElementSink_BeginUIElement(TSFSink *sink, DWORD dwUIElementId, BOOL *pbShow)
{
    ITfUIElement *element = UILess_GetUIElement((SDL_VideoData *)sink->data, dwUIElementId);
    ITfReadingInformationUIElement *preading = 0;
    SDL_VideoData *videodata = (SDL_VideoData *)sink->data;
    if (!element)
        return E_INVALIDARG;

    *pbShow = FALSE;
    if (SUCCEEDED(element->lpVtbl->QueryInterface(element, &IID_ITfReadingInformationUIElement, (LPVOID *)&preading))) {
        BSTR bstr;
        if (SUCCEEDED(preading->lpVtbl->GetString(preading, &bstr)) && bstr) {
            WCHAR *s = (WCHAR *)bstr;
            SysFreeString(bstr);
        }
        preading->lpVtbl->Release(preading);
    }
    return S_OK;
}

STDMETHODIMP UIElementSink_UpdateUIElement(TSFSink *sink, DWORD dwUIElementId)
{
    ITfUIElement *element = UILess_GetUIElement((SDL_VideoData *)sink->data, dwUIElementId);
    ITfReadingInformationUIElement *preading = 0;
    SDL_VideoData *videodata = (SDL_VideoData *)sink->data;
    if (!element)
        return E_INVALIDARG;

    if (SUCCEEDED(element->lpVtbl->QueryInterface(element, &IID_ITfReadingInformationUIElement, (LPVOID *)&preading))) {
        BSTR bstr;
        if (SUCCEEDED(preading->lpVtbl->GetString(preading, &bstr)) && bstr) {
            WCHAR *s = (WCHAR *)bstr;
            SDL_wcslcpy(videodata->ime_readingstring, s, sizeof(videodata->ime_readingstring));
            IME_SendEditingEvent(videodata);
            SysFreeString(bstr);
        }
        preading->lpVtbl->Release(preading);
    }
    return S_OK;
}

STDMETHODIMP UIElementSink_EndUIElement(TSFSink *sink, DWORD dwUIElementId)
{
    ITfUIElement *element = UILess_GetUIElement((SDL_VideoData *)sink->data, dwUIElementId);
    ITfReadingInformationUIElement *preading = 0;
    SDL_VideoData *videodata = (SDL_VideoData *)sink->data;
    if (!element)
        return E_INVALIDARG;

    if (SUCCEEDED(element->lpVtbl->QueryInterface(element, &IID_ITfReadingInformationUIElement, (LPVOID *)&preading))) {
        videodata->ime_readingstring[0] = 0;
        IME_SendEditingEvent(videodata);
        preading->lpVtbl->Release(preading);
    }
    return S_OK;
}

STDMETHODIMP IPPASink_QueryInterface(TSFSink *sink, REFIID riid, PVOID *ppv)
{
    if (!ppv)
        return E_INVALIDARG;

    *ppv = 0;
    if (SDL_IsEqualIID(riid, &IID_IUnknown))
        *ppv = (IUnknown *)sink;
    else if (SDL_IsEqualIID(riid, &IID_ITfInputProcessorProfileActivationSink))
        *ppv = (ITfInputProcessorProfileActivationSink *)sink;

    if (*ppv) {
        TSFSink_AddRef(sink);
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP IPPASink_OnActivated(TSFSink *sink, DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags)
{
    if (SDL_IsEqualIID(catid, &GUID_TFCAT_TIP_KEYBOARD) && (dwFlags & TF_IPSINK_FLAG_ACTIVE))
        IME_InputLangChanged((SDL_VideoData *)sink->data);

    return S_OK;
}

static void *vtUIElementSink[] = {
    (void *)(UIElementSink_QueryInterface),
    (void *)(TSFSink_AddRef),
    (void *)(TSFSink_Release),
    (void *)(UIElementSink_BeginUIElement),
    (void *)(UIElementSink_UpdateUIElement),
    (void *)(UIElementSink_EndUIElement)
};

static void *vtIPPASink[] = {
    (void *)(IPPASink_QueryInterface),
    (void *)(TSFSink_AddRef),
    (void *)(TSFSink_Release),
    (void *)(IPPASink_OnActivated)
};

static void
UILess_EnableUIUpdates(SDL_VideoData *videodata)
{
    ITfSource *source = 0;
    if (!videodata->ime_threadmgrex || videodata->ime_uielemsinkcookie != TF_INVALID_COOKIE)
        return;

    if (SUCCEEDED(videodata->ime_threadmgrex->lpVtbl->QueryInterface(videodata->ime_threadmgrex, &IID_ITfSource, (LPVOID *)&source))) {
        source->lpVtbl->AdviseSink(source, &IID_ITfUIElementSink, (IUnknown *)videodata->ime_uielemsink, &videodata->ime_uielemsinkcookie);
        source->lpVtbl->Release(source);
    }
}

static void
UILess_DisableUIUpdates(SDL_VideoData *videodata)
{
    ITfSource *source = 0;
    if (!videodata->ime_threadmgrex || videodata->ime_uielemsinkcookie == TF_INVALID_COOKIE)
        return;

    if (SUCCEEDED(videodata->ime_threadmgrex->lpVtbl->QueryInterface(videodata->ime_threadmgrex, &IID_ITfSource, (LPVOID *)&source))) {
        source->lpVtbl->UnadviseSink(source, videodata->ime_uielemsinkcookie);
        videodata->ime_uielemsinkcookie = TF_INVALID_COOKIE;
        source->lpVtbl->Release(source);
    }
}

static SDL_bool
UILess_SetupSinks(SDL_VideoData *videodata)
{
    TfClientId clientid = 0;
    SDL_bool result = SDL_FALSE;
    ITfSource *source = 0;
    if (FAILED(CoCreateInstance(&CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, &IID_ITfThreadMgrEx, &videodata->ime_threadmgrex)))
        return SDL_FALSE;

    if (FAILED(videodata->ime_threadmgrex->lpVtbl->ActivateEx(videodata->ime_threadmgrex, &clientid, TF_TMAE_UIELEMENTENABLEDONLY)))
        return SDL_FALSE;

    videodata->ime_uielemsink = SDL_malloc(sizeof(TSFSink));
    videodata->ime_ippasink = SDL_malloc(sizeof(TSFSink));

    videodata->ime_uielemsink->lpVtbl = vtUIElementSink;
    videodata->ime_uielemsink->refcount = 1;
    videodata->ime_uielemsink->data = videodata;

    videodata->ime_ippasink->lpVtbl = vtIPPASink;
    videodata->ime_ippasink->refcount = 1;
    videodata->ime_ippasink->data = videodata;

    if (SUCCEEDED(videodata->ime_threadmgrex->lpVtbl->QueryInterface(videodata->ime_threadmgrex, &IID_ITfSource, (LPVOID *)&source))) {
        if (SUCCEEDED(source->lpVtbl->AdviseSink(source, &IID_ITfUIElementSink, (IUnknown *)videodata->ime_uielemsink, &videodata->ime_uielemsinkcookie))) {
            if (SUCCEEDED(source->lpVtbl->AdviseSink(source, &IID_ITfInputProcessorProfileActivationSink, (IUnknown *)videodata->ime_ippasink, &videodata->ime_alpnsinkcookie))) {
                result = SDL_TRUE;
            }
        }
        source->lpVtbl->Release(source);
    }
    return result;
}

#define SAFE_RELEASE(p)                             \
{                                                   \
    if (p) {                                        \
        (p)->lpVtbl->Release((p));                  \
        (p) = 0;                                    \
    }                                               \
}

static void
UILess_ReleaseSinks(SDL_VideoData *videodata)
{
    ITfSource *source = 0;
    if (videodata->ime_threadmgrex && SUCCEEDED(videodata->ime_threadmgrex->lpVtbl->QueryInterface(videodata->ime_threadmgrex, &IID_ITfSource, &source))) {
        source->lpVtbl->UnadviseSink(source, videodata->ime_uielemsinkcookie);
        source->lpVtbl->UnadviseSink(source, videodata->ime_alpnsinkcookie);
        SAFE_RELEASE(source);
        videodata->ime_threadmgrex->lpVtbl->Deactivate(videodata->ime_threadmgrex);
        SAFE_RELEASE(videodata->ime_threadmgrex);
        TSFSink_Release(videodata->ime_uielemsink);
        videodata->ime_uielemsink = 0;
        TSFSink_Release(videodata->ime_ippasink);
        videodata->ime_ippasink = 0;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
