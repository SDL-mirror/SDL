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

/*
	Audio interrupt variables and callback function

	Patrice Mandin
*/

#include <string.h>

#include "SDL_types.h"
#include "SDL_audio.h"

#include "SDL_mintaudio.h"
#include "SDL_mintaudio_stfa.h"

/* The audio device */

SDL_AudioDevice *SDL_MintAudio_device;
Uint8 *SDL_MintAudio_audiobuf[2];	/* Pointers to buffers */
unsigned long SDL_MintAudio_audiosize;		/* Length of audio buffer=spec->size */
unsigned short SDL_MintAudio_numbuf;		/* Buffer to play */
unsigned short SDL_MintAudio_mutex;
cookie_stfa_t	*SDL_MintAudio_stfa;

/* The callback function, called by each driver whenever needed */

void SDL_MintAudio_Callback(void)
{
	Uint8 *buffer;

 	buffer = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
	memset(buffer, SDL_MintAudio_device->spec.silence, SDL_MintAudio_device->spec.size);

	if ( ! SDL_MintAudio_device->paused ) {
		if ( SDL_MintAudio_device->convert.needed ) {
			SDL_MintAudio_device->spec.callback(SDL_MintAudio_device->spec.userdata,
				(Uint8 *)SDL_MintAudio_device->convert.buf,SDL_MintAudio_device->convert.len);
			SDL_ConvertAudio(&SDL_MintAudio_device->convert);
			memcpy(buffer, SDL_MintAudio_device->convert.buf, SDL_MintAudio_device->convert.len_cvt);
		} else {
			SDL_MintAudio_device->spec.callback(SDL_MintAudio_device->spec.userdata, buffer, SDL_MintAudio_device->spec.size);
		}
	}
}

/* Simple function to search for the nearest frequency */
int SDL_MintAudio_SearchFrequency(_THIS, int falcon_codec, int desired_freq)
{
	int i;

	/* Only 1 freq ? */
	if (MINTAUDIO_nfreq==1) {
		return(MINTAUDIO_sfreq);
	}

	/* Check the array */
	for (i=MINTAUDIO_sfreq; i<MINTAUDIO_nfreq-1; i++) {
		/* Remove unusable falcon codec frequencies */
		if (falcon_codec) {
			if ((i==6) || (i==8) || (i==10)) {
				continue;
			}
		}

		if (desired_freq >= ((MINTAUDIO_hardfreq[i]+MINTAUDIO_hardfreq[i+1])>>1)) {
			return i;
		}
	}

	/* Not in the array, give the latest */
	return MINTAUDIO_nfreq-1;
}
