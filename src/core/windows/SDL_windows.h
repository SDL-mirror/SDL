/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

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

/* This is an include file for windows.h with the SDL build settings */

#ifndef _INCLUDED_WINDOWS_H
#define _INCLUDED_WINDOWS_H

#define WIN32_LEAN_AND_MEAN
#define STRICT
#ifndef UNICODE
#define UNICODE 1
#endif
#undef _WIN32_WINNT
#define _WIN32_WINNT  0x501   /* Need 0x410 for AlphaBlend() and 0x500 for EnumDisplayDevices(), 0x501 for raw input */

#include <windows.h>
#include <xinput.h>

/* Routines to convert from UTF8 to native Windows text */
#if UNICODE
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "UCS-2-INTERNAL", (char *)(S), (SDL_wcslen(S)+1)*sizeof(WCHAR))
#define WIN_UTF8ToString(S) (WCHAR *)SDL_iconv_string("UCS-2-INTERNAL", "UTF-8", (char *)(S), SDL_strlen(S)+1)
#else
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "ASCII", (char *)(S), (SDL_strlen(S)+1))
#define WIN_UTF8ToString(S) SDL_iconv_string("ASCII", "UTF-8", (char *)(S), SDL_strlen(S)+1)
#endif

/* Sets an error message based on GetLastError() */
extern void WIN_SetError(const char *prefix);

/* Wrap up the oddities of CoInitialize() into a common function. */
extern HRESULT WIN_CoInitialize(void);
extern void WIN_CoUninitialize(void);

/* typedef's for XInput structs we use */
typedef struct
{
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
    DWORD dwPaddingReserved;
} XINPUT_GAMEPAD_EX;

typedef struct 
{
    DWORD dwPacketNumber;
    XINPUT_GAMEPAD_EX Gamepad;
} XINPUT_STATE_EX;


/* Forward decl's for XInput API's we load dynamically and use if available */
typedef DWORD (WINAPI *XInputGetState_t)
	(
	DWORD         dwUserIndex,  // [in] Index of the gamer associated with the device
	XINPUT_STATE_EX* pState        // [out] Receives the current state
	);

typedef DWORD (WINAPI *XInputSetState_t)
	(
	DWORD             dwUserIndex,  // [in] Index of the gamer associated with the device
	XINPUT_VIBRATION* pVibration    // [in, out] The vibration information to send to the controller
	);

typedef DWORD (WINAPI *XInputGetCapabilities_t)
	(
	DWORD                dwUserIndex,   // [in] Index of the gamer associated with the device
	DWORD                dwFlags,       // [in] Input flags that identify the device type
	XINPUT_CAPABILITIES* pCapabilities  // [out] Receives the capabilities
	);

extern int WIN_LoadXInputDLL(void);
extern void WIN_UnloadXInputDLL(void);

extern XInputGetState_t SDL_XInputGetState;
extern XInputSetState_t SDL_XInputSetState;
extern XInputGetCapabilities_t SDL_XInputGetCapabilities;
extern DWORD SDL_XInputVersion;  // ((major << 16) & 0xFF00) | (minor & 0xFF)

#define XINPUTGETSTATE			SDL_XInputGetState
#define XINPUTSETSTATE			SDL_XInputSetState
#define XINPUTGETCAPABILITIES	SDL_XInputGetCapabilities
#define INVALID_XINPUT_USERID 255
#define SDL_XINPUT_MAX_DEVICES 4

#ifndef XINPUT_CAPS_FFB_SUPPORTED
#define XINPUT_CAPS_FFB_SUPPORTED 0x0001
#endif

#endif /* _INCLUDED_WINDOWS_H */

/* vi: set ts=4 sw=4 expandtab: */
