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



/* Allow access to a raw mixing buffer */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_audiomem.h"
#include "SDL_audio_c.h"
#include "SDL_timer.h"
#include "SDL_alsa_audio.h"

/* The tag name used by ALSA audio */
#define DRIVER_NAME         "alsa"

/* The default ALSA audio driver */
#define DEFAULT_DEVICE	"default"

/* Audio driver functions */
static int ALSA_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void ALSA_WaitAudio(_THIS);
static void ALSA_PlayAudio(_THIS);
static Uint8 *ALSA_GetAudioBuf(_THIS);
static void ALSA_CloseAudio(_THIS);

static const char *get_audio_device()
{
	const char *device;
	
	device = getenv("AUDIODEV");	/* Is there a standard variable name? */
	if ( device == NULL ) {
		device = DEFAULT_DEVICE;
	}
	return device;
}

/* Audio driver bootstrap functions */

static int Audio_Available(void)
{
	int available;
	int status;
	snd_pcm_t *handle;

	available = 0;
	status = snd_pcm_open(&handle, get_audio_device(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if ( status >= 0 ) {
		available = 1;
        	snd_pcm_close(handle);
	}
	return(available);
}

static void Audio_DeleteDevice(SDL_AudioDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_AudioDevice *)malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			free(this);
		}
		return(0);
	}
	memset(this->hidden, 0, (sizeof *this->hidden));

	/* Set the function pointers */
	this->OpenAudio = ALSA_OpenAudio;
	this->WaitAudio = ALSA_WaitAudio;
	this->PlayAudio = ALSA_PlayAudio;
	this->GetAudioBuf = ALSA_GetAudioBuf;
	this->CloseAudio = ALSA_CloseAudio;

	this->free = Audio_DeleteDevice;

	return this;
}

AudioBootStrap ALSA_bootstrap = {
	DRIVER_NAME, "ALSA 0.9 PCM audio",
	Audio_Available, Audio_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void ALSA_WaitAudio(_THIS)
{
	/* Check to see if the thread-parent process is still alive */
	{ static int cnt = 0;
		/* Note that this only works with thread implementations 
		   that use a different process id for each thread.
		*/
		if (parent && (((++cnt)%10) == 0)) { /* Check every 10 loops */
			if ( kill(parent, 0) < 0 ) {
				this->enabled = 0;
			}
		}
	}
}

static void ALSA_PlayAudio(_THIS)
{
	int           status;
	int           sample_len;
	signed short *sample_buf;

	sample_len = this->spec.samples;
	sample_buf = (signed short *)mixbuf;
	while ( sample_len > 0 ) {
		status = snd_pcm_writei(pcm_handle, sample_buf, sample_len);
		if ( status < 0 ) {
			if ( status == -EAGAIN ) {
				SDL_Delay(1);
				continue;
			}
			if ( status == -ESTRPIPE ) {
				do {
					SDL_Delay(1);
					status = snd_pcm_resume(pcm_handle);
				} while ( status == -EAGAIN );
			}
			if ( status < 0 ) {
				status = snd_pcm_prepare(pcm_handle);
			}
			if ( status < 0 ) {
				/* Hmm, not much we can do - abort */
				this->enabled = 0;
				return;
			}
			continue;
		}
		sample_buf += status * this->spec.channels;
		sample_len -= status;
	}
}

static Uint8 *ALSA_GetAudioBuf(_THIS)
{
	return(mixbuf);
}

static void ALSA_CloseAudio(_THIS)
{
	if ( mixbuf != NULL ) {
		SDL_FreeAudioMem(mixbuf);
		mixbuf = NULL;
	}
	if ( pcm_handle ) {
		snd_pcm_drain(pcm_handle);
		snd_pcm_close(pcm_handle);
		pcm_handle = NULL;
	}
}

static int ALSA_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	int                  status;
	snd_pcm_hw_params_t *params;
	snd_pcm_format_t     format;
	snd_pcm_uframes_t    frames;
	Uint16               test_format;

	/* Open the audio device */
	status = snd_pcm_open(&pcm_handle, get_audio_device(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if ( status < 0 ) {
		SDL_SetError("Couldn't open audio device: %s", snd_strerror(status));
		return(-1);
	}

	/* Figure out what the hardware is capable of */
	snd_pcm_hw_params_alloca(&params);
	status = snd_pcm_hw_params_any(pcm_handle, params);
	if ( status < 0 ) {
		SDL_SetError("Couldn't get hardware config: %s", snd_strerror(status));
		ALSA_CloseAudio(this);
		return(-1);
	}

	/* SDL only uses interleaved sample output */
	status = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if ( status < 0 ) {
		SDL_SetError("Couldn't set interleaved access: %s", snd_strerror(status));
		ALSA_CloseAudio(this);
		return(-1);
	}

	/* Try for a closest match on audio format */
	status = -1;
	for ( test_format = SDL_FirstAudioFormat(spec->format);
	      test_format && (status < 0); ) {
		switch ( test_format ) {
			case AUDIO_U8:
				format = SND_PCM_FORMAT_U8;
				break;
			case AUDIO_S8:
				format = SND_PCM_FORMAT_S8;
				break;
			case AUDIO_S16LSB:
				format = SND_PCM_FORMAT_S16_LE;
				break;
			case AUDIO_S16MSB:
				format = SND_PCM_FORMAT_S16_BE;
				break;
			case AUDIO_U16LSB:
				format = SND_PCM_FORMAT_U16_LE;
				break;
			case AUDIO_U16MSB:
				format = SND_PCM_FORMAT_U16_BE;
				break;
			default:
				format = 0;
				break;
		}
		if ( format != 0 ) {
			status = snd_pcm_hw_params_set_format(pcm_handle, params, format);
		}
		if ( status < 0 ) {
			test_format = SDL_NextAudioFormat();
		}
	}
	if ( status < 0 ) {
		SDL_SetError("Couldn't find any hardware audio formats");
		ALSA_CloseAudio(this);
		return(-1);
	}
	spec->format = test_format;

	/* Set the number of channels */
	status = snd_pcm_hw_params_set_channels(pcm_handle, params, spec->channels);
	if ( status < 0 ) {
		status = snd_pcm_hw_params_get_channels(params);
		if ( (status <= 0) || (status > 2) ) {
			SDL_SetError("Couldn't set audio channels");
			ALSA_CloseAudio(this);
			return(-1);
		}
		spec->channels = status;
	}

	/* Set the audio rate */
	status = snd_pcm_hw_params_set_rate_near(pcm_handle, params, spec->freq, NULL);
	if ( status < 0 ) {
		SDL_SetError("Couldn't set audio frequency: %s", snd_strerror(status));
		ALSA_CloseAudio(this);
		return(-1);
	}
	spec->freq = status;

	/* Set the buffer size, in samples */
	frames = spec->samples;
	frames = snd_pcm_hw_params_set_period_size_near(pcm_handle, params, frames, NULL);
	spec->samples = frames;
	snd_pcm_hw_params_set_periods_near(pcm_handle, params, 2, NULL);

	/* "set" the hardware with the desired parameters */
	status = snd_pcm_hw_params(pcm_handle, params);
	if ( status < 0 ) {
		SDL_SetError("Couldn't set audio parameters: %s", snd_strerror(status));
		ALSA_CloseAudio(this);
		return(-1);
	}

	/* Calculate the final parameters for this audio specification */
	SDL_CalculateAudioSpec(spec);

	/* Allocate mixing buffer */
	mixlen = spec->size;
	mixbuf = (Uint8 *)SDL_AllocAudioMem(mixlen);
	if ( mixbuf == NULL ) {
		ALSA_CloseAudio(this);
		return(-1);
	}
	memset(mixbuf, spec->silence, spec->size);

	/* Get the parent process id (we're the parent of the audio thread) */
	parent = getpid();

	/* Switch to blocking mode for playback */
	snd_pcm_nonblock(pcm_handle, 0);

	/* We're ready to rock and roll. :-) */
	return(0);
}
