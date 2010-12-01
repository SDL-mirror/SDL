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

#ifndef _SDL_win32video_h
#define _SDL_win32video_h

#include "../SDL_sysvideo.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT
#ifndef UNICODE
#define UNICODE
#endif
#undef WINVER
#define WINVER  0x500           /* Need 0x410 for AlphaBlend() and 0x500 for EnumDisplayDevices() */

#include <windows.h>

#ifndef __GNUC__
#include <msctf.h>
#else
#include "SDL_msctf.h"
#endif

#include <imm.h>

#define MAX_CANDLIST    10
#define MAX_CANDLENGTH  256

#if SDL_VIDEO_RENDER_D3D
//#include <d3d9.h>
#define D3D_DEBUG_INFO
#include "d3d9.h"
#endif

#if SDL_VIDEO_RENDER_DDRAW
/* WIN32_LEAN_AND_MEAN was defined, so we have to include this by hand */
#include <objbase.h>
#include "ddraw.h"
#endif

#include "SDL_win32clipboard.h"
#include "SDL_win32events.h"
#include "SDL_win32gamma.h"
#include "SDL_win32keyboard.h"
#include "SDL_win32modes.h"
#include "SDL_win32mouse.h"
#include "SDL_win32opengl.h"
#include "SDL_win32window.h"
#include "SDL_events.h"

#ifdef UNICODE
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "UCS-2", (char *)S, (SDL_wcslen(S)+1)*sizeof(WCHAR))
#define WIN_UTF8ToString(S) (WCHAR *)SDL_iconv_string("UCS-2", "UTF-8", (char *)S, SDL_strlen(S)+1)
#else
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "ASCII", (char *)S, (SDL_strlen(S)+1))
#define WIN_UTF8ToString(S) SDL_iconv_string("ASCII", "UTF-8", (char *)S, SDL_strlen(S)+1)
#endif
extern void WIN_SetError(const char *prefix);

enum { RENDER_NONE, RENDER_D3D, RENDER_DDRAW, RENDER_GDI, RENDER_GAPI, RENDER_RAW };

#if WINVER < 0x0601
/* Touch input definitions */
#define TWF_FINETOUCH	1
#define TWF_WANTPALM	2

#define TOUCHEVENTF_MOVE 0x0001
#define TOUCHEVENTF_DOWN 0x0002
#define TOUCHEVENTF_UP   0x0004

DECLARE_HANDLE(HTOUCHINPUT);

typedef struct _TOUCHINPUT {
	LONG      x;
	LONG      y;
	HANDLE    hSource;
	DWORD     dwID;
	DWORD     dwFlags;
	DWORD     dwMask;
	DWORD     dwTime;
	ULONG_PTR dwExtraInfo;
	DWORD     cxContact;
	DWORD     cyContact;
} TOUCHINPUT, *PTOUCHINPUT;

#endif /* WINVER < 0x0601 */

typedef BOOL  (*PFNSHFullScreen)(HWND, DWORD);
typedef void  (*PFCoordTransform)(SDL_Window*, POINT*);

typedef struct  
{
    void **lpVtbl;
    int refcount;
    void *data;
} TSFSink;

/* Definition from Win98DDK version of IMM.H */
typedef struct tagINPUTCONTEXT2 {
    HWND hWnd;
    BOOL fOpen;
    POINT ptStatusWndPos;
    POINT ptSoftKbdPos;
    DWORD fdwConversion;
    DWORD fdwSentence;
    union {
        LOGFONTA A;
        LOGFONTW W;
    } lfFont;
    COMPOSITIONFORM cfCompForm;
    CANDIDATEFORM cfCandForm[4];
    HIMCC hCompStr;
    HIMCC hCandInfo;
    HIMCC hGuideLine;
    HIMCC hPrivate;
    DWORD dwNumMsgBuf;
    HIMCC hMsgBuf;
    DWORD fdwInit;
    DWORD dwReserve[3];
} INPUTCONTEXT2, *PINPUTCONTEXT2, NEAR *NPINPUTCONTEXT2, FAR *LPINPUTCONTEXT2;

/* Private display data */

typedef struct SDL_VideoData
{
    int render;

#if SDL_VIDEO_RENDER_D3D
    HANDLE d3dDLL;
    IDirect3D9 *d3d;
#endif
#if SDL_VIDEO_RENDER_DDRAW
    HANDLE ddrawDLL;
    IDirectDraw *ddraw;
#endif
#ifdef _WIN32_WCE
    HMODULE hAygShell;
    PFNSHFullScreen SHFullScreen;
    PFCoordTransform CoordTransform;
#endif

    const SDL_scancode *key_layout;
	DWORD clipboard_count;

	/* Touch input functions */
	HANDLE userDLL;
	BOOL (WINAPI *CloseTouchInputHandle)( HTOUCHINPUT );
	BOOL (WINAPI *GetTouchInputInfo)( HTOUCHINPUT, UINT, PTOUCHINPUT, int );
	BOOL (WINAPI *RegisterTouchWindow)( HWND, ULONG );

    SDL_bool ime_com_initialized;
    struct ITfThreadMgr *ime_threadmgr;
    SDL_bool ime_initialized;
    SDL_bool ime_enabled;
    SDL_bool ime_available;
    HWND ime_hwnd_main;
    HWND ime_hwnd_current;
    HIMC ime_himc;

    WCHAR ime_composition[SDL_TEXTEDITINGEVENT_TEXT_SIZE];
    WCHAR ime_readingstring[16];
    int ime_cursor;

    SDL_bool ime_candlist;
    WCHAR ime_candidates[MAX_CANDLIST][MAX_CANDLENGTH];
    DWORD ime_candcount;
    DWORD ime_candref;
    DWORD ime_candsel;
    UINT ime_candpgsize;
    int ime_candlistindexbase;
    SDL_bool ime_candvertical;

    SDL_Texture *ime_candtex;
    SDL_bool ime_dirty;
    SDL_Rect ime_rect;
    SDL_Rect ime_candlistrect;
    int ime_winwidth;
    int ime_winheight;

    HKL ime_hkl;
    HMODULE ime_himm32;
    UINT (WINAPI *GetReadingString)(HIMC himc, UINT uReadingBufLen, LPWSTR lpwReadingBuf, PINT pnErrorIndex, BOOL *pfIsVertical, PUINT puMaxReadingLen);
    BOOL (WINAPI *ShowReadingWindow)(HIMC himc, BOOL bShow);
    LPINPUTCONTEXT2 (WINAPI *ImmLockIMC)(HIMC himc);
    BOOL (WINAPI *ImmUnlockIMC)(HIMC himc);
    LPVOID (WINAPI *ImmLockIMCC)(HIMCC himcc);
    BOOL (WINAPI *ImmUnlockIMCC)(HIMCC himcc);

    SDL_bool ime_uiless;
    struct ITfThreadMgrEx *ime_threadmgrex;
    DWORD ime_uielemsinkcookie;
    DWORD ime_alpnsinkcookie;
    DWORD ime_openmodesinkcookie;
    DWORD ime_convmodesinkcookie;
    TSFSink *ime_uielemsink;
    TSFSink *ime_ippasink;
} SDL_VideoData;

#endif /* _SDL_win32video_h */

/* vi: set ts=4 sw=4 expandtab: */
