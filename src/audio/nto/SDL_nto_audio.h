/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

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

#ifndef _NTO_PCM_audio_h
#define _NTO_PCM_audio_h

#include "SDL_sysaudio.h"
#include <sys/asoundlib.h>

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData {
	/* The audio device handle */
	 snd_pcm_t *audio_handle;

	/* The audio file descriptor */
	int audio_fd;

	/* The parent process id, to detect when application quits */
	pid_t parent;

	/* Raw mixing buffer */
	Uint8 *pcm_buf;
	int    pcm_len;
};
#define FUDGE_TICKS	10	/* The scheduler overhead ticks per frame */

/* Old variable names */
#define audio_handle	(this->hidden->audio_handle)
#define audio_fd		(this->hidden->audio_fd)
#define parent			(this->hidden->parent)
#define pcm_buf			(this->hidden->pcm_buf)
#define pcm_len			(this->hidden->pcm_len)

#endif /* _NTO_PCM_audio_h */
