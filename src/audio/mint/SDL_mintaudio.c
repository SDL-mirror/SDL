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
unsigned long SDL_MintAudio_clocktics;
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

/* Add a new frequency/clock/predivisor to the current list */
void SDL_MintAudio_AddFrequency(_THIS, Uint32 frequency, Uint32 clock, Uint32 prediv)
{
	int i, p;

	if (MINTAUDIO_freqcount==MINTAUDIO_maxfreqs) {
		return;
	}

	/* Search where to insert the frequency (highest first) */
	for (p=0; p<MINTAUDIO_freqcount; p++) {
		if (frequency > MINTAUDIO_frequencies[p].frequency) {
			break;
		}
	}

	/* Put all following ones farer */
	if (MINTAUDIO_freqcount>0) {
		for (i=MINTAUDIO_freqcount; i>p; i--) {
			MINTAUDIO_frequencies[i].frequency = MINTAUDIO_frequencies[i-1].frequency;
			MINTAUDIO_frequencies[i].masterclock = MINTAUDIO_frequencies[i-1].masterclock;
			MINTAUDIO_frequencies[i].predivisor = MINTAUDIO_frequencies[i-1].predivisor;
		}
	}

	/* And insert new one */
	MINTAUDIO_frequencies[p].frequency = frequency;
	MINTAUDIO_frequencies[p].masterclock = clock;
	MINTAUDIO_frequencies[p].predivisor = prediv;

	MINTAUDIO_freqcount++;
}

/* Search for the nearest frequency */
int SDL_MintAudio_SearchFrequency(_THIS, int desired_freq)
{
	int i;

	/* Only 1 freq ? */
	if (MINTAUDIO_freqcount==1) {
		return 0;
	}

	/* Check the array */
	for (i=0; i<MINTAUDIO_freqcount; i++) {
		if (desired_freq >= ((MINTAUDIO_frequencies[i].frequency+
			MINTAUDIO_frequencies[i+1].frequency)>>1)) {
			return i;
		}
	}

	/* Not in the array, give the latest */
	return MINTAUDIO_freqcount-1;
}
