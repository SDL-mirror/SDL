/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

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
#ifndef _SDL_os4audio_h
#define _SDL_os4audio_h

#include <exec/types.h>
#include <exec/ports.h>
#include <devices/ahi.h>

#include "SDL_types.h"

#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the video functions */
#define _THIS   SDL_AudioDevice *_this

struct SDL_PrivateAudioData
{
    struct MsgPort       *ahi_ReplyPort;
    struct AHIRequest    *ahi_IORequest[2];
    //struct AHIIFace      *ahi_IFace;
    //struct AHIAudioCtrl  *ahi_AudioCtrl;
    uint32                ahi_Type;
    int                   currentBuffer; // buffer number to fill
    struct AHIRequest    *link;          // point to previous I/O request sent

    int                   audio_IsOpen;
    Uint32                audio_MixBufferSize;
    Uint8                *audio_MixBuffer[2];
    
    //APTR                  audio_Mutex;
};

typedef struct SDL_PrivateAudioData OS4AudioData;

#endif
