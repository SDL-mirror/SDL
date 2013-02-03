/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

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

/**
 *  \file SDL_system.h
 *  
 *  Include file for platform specific SDL API functions
 */

#ifndef _SDL_system_h
#define _SDL_system_h

#include "SDL_stdinc.h"

#if defined(__IPHONEOS__) && __IPHONEOS__
#include "SDL_video.h"
#include "SDL_keyboard.h"
#endif

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/* Platform specific functions for iOS */
#if defined(__IPHONEOS__) && __IPHONEOS__

extern DECLSPEC int SDLCALL SDL_iPhoneSetAnimationCallback(SDL_Window * window, int interval, void (*callback)(void*), void *callbackParam);
extern DECLSPEC void SDLCALL SDL_iPhoneSetEventPump(SDL_bool enabled);

#endif /* __IPHONEOS__ */


/* Platform specific functions for Android */
#if defined(__ANDROID__) && __ANDROID__

/* Get the JNI environment for the current thread
   This returns JNIEnv*, but the prototype is void* so we don't need jni.h
 */
extern DECLSPEC void * SDLCALL SDL_AndroidGetJNIEnv();

/* Get the SDL Activity object for the application
   This returns jobject, but the prototype is void* so we don't need jni.h
 */
extern DECLSPEC void * SDLCALL SDL_AndroidGetActivity();

/* See the official Android developer guide for more information:
   http://developer.android.com/guide/topics/data/data-storage.html
*/
#define SDL_ANDROID_EXTERNAL_STORAGE_READ   0x01
#define SDL_ANDROID_EXTERNAL_STORAGE_WRITE  0x02

/* Get the path used for internal storage for this application.
   This path is unique to your application and cannot be written to
   by other applications.
 */
extern DECLSPEC const char * SDLCALL SDL_AndroidGetInternalStoragePath();

/* Get the current state of external storage, a bitmask of these values:
    SDL_ANDROID_EXTERNAL_STORAGE_READ
    SDL_ANDROID_EXTERNAL_STORAGE_WRITE
   If external storage is currently unavailable, this will return 0.
*/
extern DECLSPEC int SDLCALL SDL_AndroidGetExternalStorageState();

/* Get the path used for external storage for this application.
   This path is unique to your application, but is public and can be
   written to by other applications.
 */
extern DECLSPEC const char * SDLCALL SDL_AndroidGetExternalStoragePath();

#endif /* __ANDROID__ */


/* Platform specific functions for Windows RT */
#if defined(__WINRT__) && __WINRT__

/* Gets the path to the installed app's root directory.

   This function may be used safely on Windows Phone 8.
*/
extern DECLSPEC const wchar_t * SDLCALL SDL_WinRTGetInstalledLocationPath();

/* Gets the path to the local app data store.
   Files and directories that should be limited to the local device can be
   created in this path.

   This function may be used safely on Windows Phone 8, as opposed to
   SDL_WinRTGetRoamingFolderPath() and SDL_WinRTGetTemporaryFolderPath(),
   which do not work on Windows Phone 8 (and will return NULL if called
   from this platform).
 */
extern DECLSPEC const wchar_t * SDLCALL SDL_WinRTGetLocalFolderPath();

/* Gets the path to the roaming app data store.
   Files and directories that should roam to different devices can be
   created in this path.  Be sure to read Microsoft's documentation on
   roaming files for more information on how this works, as restrictions
   do apply.

   Please note that on Windows Phone 8, this function will return NULL,
   as Windows Phone 8 apps do not have an accessible roaming app data
   store.
 */
extern DECLSPEC const wchar_t * SDLCALL SDL_WinRTGetRoamingFolderPath();

/* Gets the path to the temporary app data store.
   Files and directories may be written here, however they may be deleted
   by Windows at a future date.

   Please note that on Windows Phone 8, this function will return NULL,
   as Windows Phone 8 apps do not have an accessible temporary app data
   store.
*/
extern DECLSPEC const wchar_t * SDLCALL SDL_WinRTGetTemporaryFolderPath();

#endif /* __WINRT__ */


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_system_h */

/* vi: set ts=4 sw=4 expandtab: */
