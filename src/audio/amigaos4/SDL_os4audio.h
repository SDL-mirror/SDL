/*
    AmigaOS4 support for the SDL - Simple DirectMedia Layer
    Copyright (C) 2004  J�rgen Schober

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    J�rgen Schober
    juergen.schober@pointdesign.com
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
    struct AHIIFace      *ahi_IFace;
    struct AHIAudioCtrl  *ahi_AudioCtrl;
    uint32                ahi_Type;
    int                   currentBuffer; // buffer number to fill
    struct AHIRequest    *link;          // point to previous I/O request sent

    int                   audio_IsOpen;
    Uint32                audio_MixBufferSize;
    Uint8                *audio_MixBuffer[2];
    
    APTR                  audio_Mutex;
};

typedef struct SDL_PrivateAudioData OS4AudioData;

#endif
