/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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

#ifndef _SDL_coreaudio_h
#define _SDL_coreaudio_h

#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_AudioDevice *this

#if (MAC_OS_X_VERSION_MIN_REQUIRED < 1060) || \
    (!defined(AUDIO_UNIT_VERSION) || ((AUDIO_UNIT_VERSION + 0) < 1060))
typedef struct ComponentDescription	AudioComponentDesc_t;
typedef Component			AudioComponent_t;
typedef AudioUnit			AudioComponentInstance_t;
#define AudioComponentInstanceNew_fn		OpenAComponent
#define AudioComponentInstanceDispose_fn	CloseComponent
#define AudioComponentFindNext_fn		FindNextComponent
#else
typedef AudioComponentDescription	AudioComponentDesc_t;
typedef AudioComponent			AudioComponent_t;
typedef AudioComponentInstance		AudioComponentInstance_t;
#define AudioComponentInstanceNew_fn		AudioComponentInstanceNew
#define AudioComponentInstanceDispose_fn	AudioComponentInstanceDispose
#define AudioComponentFindNext_fn		AudioComponentFindNext
#endif

struct SDL_PrivateAudioData {
	AudioComponentInstance_t outputAudioUnit;
	void *buffer;
	UInt32 bufferOffset;
	UInt32 bufferSize;
};

/* Old variable names */
#define outputAudioUnit		(this->hidden->outputAudioUnit)
#define buffer		(this->hidden->buffer)
#define bufferOffset		(this->hidden->bufferOffset)
#define bufferSize		(this->hidden->bufferSize)

#endif /* _SDL_coreaudio_h */
