/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#ifndef _SDL_coreaudio_h
#define _SDL_coreaudio_h

#include "SDL_sysaudio.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData {
	AudioUnit outputAudioUnit;
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
