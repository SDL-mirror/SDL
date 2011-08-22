/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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
#include "SDL_config.h"

#ifndef _SDL_xaudio2_h
#define _SDL_xaudio2_h

#include "../SDL_sysaudio.h"

#if SDL_AUDIO_DRIVER_XAUDIO2
#include <dxsdkver.h> /* XAudio2 exists as of the March 2008 DirectX SDK */
#if (defined(_DXSDK_BUILD_MAJOR) && (_DXSDK_BUILD_MAJOR >= 1284))
#   define SDL_HAVE_XAUDIO2_H 1
#endif
#endif

#ifdef SDL_HAVE_XAUDIO2_H
#include <XAudio2.h>

/* Hidden "this" pointer for the audio functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    IXAudio2 *ixa2;
    IXAudio2SourceVoice *source;
    IXAudio2MasteringVoice *mastering;
    HANDLE semaphore;
    Uint8 *mixbuf;
    int mixlen;
    Uint8 *nextbuf;
};
#endif

#endif /* _SDL_xaudio2_h */

/* vi: set ts=4 sw=4 expandtab: */
