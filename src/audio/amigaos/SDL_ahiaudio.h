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

#ifndef _SDL_ahiaudio_h
#define _SDL_ahiaudio_h

#include <exec/exec.h>
#include <dos/dos.h>
#ifdef __SASC
#include <proto/exec.h>
#else
#include <inline/exec.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <devices/ahi.h>
#include "mydebug.h"
#include "SDL_sysaudio.h"

/* Hidden "this" pointer for the audio functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData {
	/* The handle for the audio device */
	struct AHIRequest *audio_req[2];
	struct MsgPort *audio_port;
	Sint32 freq,type,bytespersample,size;
	Uint8 *mixbuf[2];           /* The app mixing buffer */
	int current_buffer;
	Uint32 playing;
};

/* Old variable names */
#define audio_port		(this->hidden->audio_port)
#define audio_req		(this->hidden->audio_req)
#define mixbuf			(this->hidden->mixbuf)
#define current_buffer		(this->hidden->current_buffer)
#define playing			(this->hidden->playing)

#endif /* _SDL_ahiaudio_h */
