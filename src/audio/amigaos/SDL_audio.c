/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Allow access to a raw mixing buffer */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_timer.h"
#include "SDL_error.h"
#include "SDL_audio_c.h"
#include "SDL_audiomem.h"
#include "SDL_sysaudio.h"


/* Available audio drivers */
static AudioBootStrap *bootstrap[] = {
#ifdef unix
	&AUDIO_bootstrap,
#endif
#ifdef linux
	&DMA_bootstrap,
#endif
#ifdef ESD_SUPPORT
	&ESD_bootstrap,
#endif
#ifdef ENABLE_DIRECTX
	&DSOUND_bootstrap,
#endif
#ifdef ENABLE_WINDIB
	&WAVEOUT_bootstrap,
#endif
#ifdef __BEOS__
	&BAUDIO_bootstrap,
#endif
#ifdef macintosh
	&AUDIO_bootstrap,
#endif
#ifdef _AIX
	&Paud_bootstrap,
#endif
#ifdef ENABLE_CYBERGRAPHICS
	&AHI_bootstrap,
#endif
	NULL
};
SDL_AudioDevice *current_audio = NULL;

/* Various local functions */
int SDL_AudioInit(const char *driver_name);
void SDL_AudioQuit(void);

struct SignalSemaphore AudioSem;

/* The general mixing thread function */
int RunAudio(void *audiop)
{
	SDL_AudioDevice *audio = (SDL_AudioDevice *)audiop;
	Uint8 *stream;
	int    stream_len;
	void  *udata;
	void (*fill)(void *userdata,Uint8 *stream, int len);
	int    silence,started=0;

	D(bug("Task audio started audio struct:<%lx>...\n",audiop));

	D(bug("Before Openaudio..."));
	if(audio->OpenAudio(audio, &audio->spec)==-1)
	{
		return(-1);
	}

	D(bug("OpenAudio...OK\n"));

	/* Perform any thread setup */
	if ( audio->ThreadInit ) {
		audio->ThreadInit(audio);
	}
	audio->threadid = SDL_ThreadID();

	/* Set up the mixing function */
	fill  = audio->spec.callback;
	udata = audio->spec.userdata;
	if ( audio->convert.needed ) {
		if ( audio->convert.src_format == AUDIO_U8 ) {
			silence = 0x80;
			D(bug("*** Silence 0x80 ***\n"));
		} else {
			silence = 0;
		}
		stream_len = audio->convert.len;
	} else {
		silence = audio->spec.silence;
		stream_len = audio->spec.size;
	}
	stream = audio->fake_stream;

	ObtainSemaphore(&AudioSem);
	ReleaseSemaphore(&AudioSem);

	D(bug("Enering audio loop...\n"));

	D(if(audio->convert.needed)bug("*** Conversion needed.\n"));

	/* Loop, filling the audio buffers */
	while ( audio->enabled ) 
	{
		/* Wait for new current buffer to finish playing */

		if ( stream == audio->fake_stream )
			SDL_Delay((audio->spec.samples*1000)/audio->spec.freq);
		else
		{
			if(started>1)
			{
//				D(bug("Waiting audio...\n"));
				audio->WaitAudio(audio);
			}
		}

		ObtainSemaphore(&AudioSem);

		/* Fill the current buffer with sound */
		if ( audio->convert.needed ) {
			stream = audio->convert.buf;
		} else {
			stream = audio->GetAudioBuf(audio);
		}

		if(stream!=audio->fake_stream)
			memset(stream, silence, stream_len);

		if ( ! audio->paused ) {
			ObtainSemaphore(&audio->mixer_lock);
			(*fill)(udata, stream, stream_len);
			ReleaseSemaphore(&audio->mixer_lock);
		}

		/* Convert the audio if necessary */
		if ( audio->convert.needed ) {
			SDL_ConvertAudio(&audio->convert);
			stream = audio->GetAudioBuf(audio);
			memcpy(stream, audio->convert.buf,audio->convert.len_cvt);
		}

		if(stream!=audio->fake_stream)
		{
//			D(bug("Playing stream at %lx\n",stream));

			audio->PlayAudio(audio);
			started++;
		}
		ReleaseSemaphore(&AudioSem);

	}
	D(bug("Out of subtask loop...\n"));

	/* Wait for the audio to drain.. */
	if ( audio->WaitDone ) {
		audio->WaitDone(audio);
	}

	D(bug("WaitAudio...Done\n"));

	audio->CloseAudio(audio);

	D(bug("CloseAudio..Done, subtask exiting...\n"));

	return(0);
}

int SDL_AudioInit(const char *driver_name)
{
	SDL_AudioDevice *audio;
	int i = 0, idx;

	/* Check to make sure we don't overwrite 'current_audio' */
	if ( current_audio != NULL ) {
		SDL_AudioQuit();
	}

	/* Select the proper audio driver */
	audio = NULL;
	idx = 0;

	InitSemaphore(&AudioSem);

	if ( audio == NULL ) {
		if ( driver_name != NULL ) {
			if ( strrchr(driver_name, ':') != NULL ) {
				idx = atoi(strrchr(driver_name, ':')+1);
			}
			for ( i=0; bootstrap[i]; ++i ) {
				if (strncmp(bootstrap[i]->name, driver_name,
				            strlen(bootstrap[i]->name)) == 0) {
					if ( bootstrap[i]->available() ) {
						audio=bootstrap[i]->create(idx);
						break;
					}
				}
			}
		} else {
			for ( i=0; bootstrap[i]; ++i ) {
				if ( bootstrap[i]->available() ) {
					audio = bootstrap[i]->create(idx);
					if ( audio != NULL ) {
						break;
					}
				}
			}
		}
		if ( audio == NULL ) {
			SDL_SetError("No available audio device");
#if 0 /* Don't fail SDL_Init() if audio isn't available.
         SDL_OpenAudio() will handle it at that point.  *sigh*
       */
			return(-1);
#endif
		}
	}
	current_audio = audio;
	if ( current_audio ) {
		current_audio->name = bootstrap[i]->name;
	}
	return(0);
}

char *SDL_AudioDriverName(char *namebuf, int maxlen)
{
	if ( current_audio != NULL ) {
		strncpy(namebuf, current_audio->name, maxlen-1);
		namebuf[maxlen-1] = '\0';
		return(namebuf);
	}
	return(NULL);
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
	SDL_AudioDevice *audio;

	/* Start up the audio driver, if necessary */
	if ( ! current_audio ) {
		if ( (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) ||
		     (current_audio == NULL) ) {
			return(-1);
		}
	}
	audio = current_audio;

	D(bug("Chiamata SDL_OpenAudio...\n"));

	/* Verify some parameters */
	if ( desired->callback == NULL ) {
		SDL_SetError("SDL_OpenAudio() passed a NULL callback");
		return(-1);
	}
	switch ( desired->channels ) {
	    case 1:	/* Mono */
	    case 2:	/* Stereo */
		break;
	    default:
		SDL_SetError("1 (mono) and 2 (stereo) channels supported");
		return(-1);
	}

	/* Create a semaphore for locking the sound buffers */
	InitSemaphore(&audio->mixer_lock);

	/* Calculate the silence and size of the audio specification */
	SDL_CalculateAudioSpec(desired);

	/* Open the audio subsystem */
	memcpy(&audio->spec, desired, sizeof(audio->spec));
	audio->convert.needed = 0;
	audio->enabled = 1;
	audio->paused  = 1;

	ObtainSemaphore(&AudioSem);

	audio->thread = SDL_CreateThread(RunAudio, audio);

	if ( audio->thread == NULL ) {
		ReleaseSemaphore(&AudioSem);
		SDL_CloseAudio();
		SDL_SetError("Couldn't create audio thread");
		return(-1);
	}

	/* If the audio driver changes the buffer size, accept it */
	if ( audio->spec.samples != desired->samples ) {
		desired->samples = audio->spec.samples;
		SDL_CalculateAudioSpec(desired);
	}

	/* Allocate a fake audio memory buffer */
	audio->fake_stream = SDL_AllocAudioMem(audio->spec.size);
	if ( audio->fake_stream == NULL ) {
		ReleaseSemaphore(&AudioSem);
		SDL_CloseAudio();
		SDL_OutOfMemory();
		return(-1);
	}

	/* See if we need to do any conversion */
	if ( memcmp(desired, &audio->spec, sizeof(audio->spec)) == 0 ) {
		/* Just copy over the desired audio specification */
		if ( obtained != NULL ) {
			memcpy(obtained, &audio->spec, sizeof(audio->spec));
		}
	} else {
		/* Copy over the audio specification if possible */
		if ( obtained != NULL ) {
			memcpy(obtained, &audio->spec, sizeof(audio->spec));
		} else {
			/* Build an audio conversion block */
			D(bug("Need conversion:\n desired: C:%ld F:%ld T:%lx\navailable: C:%ld F:%ld T:%lx\n",
				desired->channels, desired->freq, desired->format,
				audio->spec.channels,audio->spec.freq,audio->spec.format));

			Forbid();

// Magari poi lo sostiutisco con un semaforo.

			if ( SDL_BuildAudioCVT(&audio->convert,
				desired->format, desired->channels,
						desired->freq,
				audio->spec.format, audio->spec.channels,
						audio->spec.freq) < 0 ) {
				ReleaseSemaphore(&AudioSem);
				SDL_CloseAudio();
				return(-1);
			}
			if ( audio->convert.needed ) {
				audio->convert.len = desired->size;
				audio->convert.buf =(Uint8 *)SDL_AllocAudioMem(
				   audio->convert.len*audio->convert.len_mult);
				if ( audio->convert.buf == NULL ) {
					ReleaseSemaphore(&AudioSem);
					SDL_CloseAudio();
					SDL_OutOfMemory();
					return(-1);
				}
			}
		}
	}

	ReleaseSemaphore(&AudioSem);

	D(bug("SDL_OpenAudio USCITA...\n"));

	return(0);
}

SDL_audiostatus SDL_GetAudioStatus(void)
{
	SDL_AudioDevice *audio = current_audio;
	SDL_audiostatus status;

	status = SDL_AUDIO_STOPPED;
	if ( audio && audio->enabled ) {
		if ( audio->paused ) {
			status = SDL_AUDIO_PAUSED;
		} else {
			status = SDL_AUDIO_PLAYING;
		}
	}
	return(status);
}

void SDL_PauseAudio (int pause_on)
{
	SDL_AudioDevice *audio = current_audio;

	if ( audio ) {
		audio->paused = pause_on;
	}
}

void SDL_LockAudio (void)
{
	SDL_AudioDevice *audio = current_audio;

	/* Obtain a lock on the mixing buffers */
	if ( audio ) {
		if ( audio->thread && (SDL_ThreadID() == audio->threadid) ) {
			return;
		}
		ObtainSemaphore(&audio->mixer_lock);
	}
}

void SDL_UnlockAudio (void)
{
	SDL_AudioDevice *audio = current_audio;

	/* Release lock on the mixing buffers */
	if ( audio ) {
		if ( audio->thread && (SDL_ThreadID() == audio->threadid) ) {
			return;
		}
		ReleaseSemaphore(&audio->mixer_lock);
	}
}

void SDL_CloseAudio (void)
{
	SDL_AudioDevice *audio = current_audio;

	if ( audio ) {
		if(audio->enabled)
		{
			audio->enabled = 0;

			if ( audio->thread != NULL ) {
				D(bug("Waiting audio thread...\n"));
				SDL_WaitThread(audio->thread, NULL);
				D(bug("...audio replied\n"));
			}
		}

		if ( audio->fake_stream != NULL ) {
			SDL_FreeAudioMem(audio->fake_stream);
			audio->fake_stream=NULL;
		}
		if ( audio->convert.needed && current_audio->convert.buf!=NULL) {
			SDL_FreeAudioMem(audio->convert.buf);
			current_audio->convert.buf=NULL;
		}
	}
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SDL_AudioQuit(void)
{
	if ( current_audio ) {
		if(current_audio->enabled)
		{
			D(bug("Closing audio in AudioQuit...\n"));
			current_audio->enabled = 0;

			if ( current_audio->thread != NULL ) {
				D(bug("Waiting audio thread...\n"));
				SDL_WaitThread(current_audio->thread, NULL);
				D(bug("...audio replied\n"));
			}
		}
		if ( current_audio->fake_stream != NULL ) {
			SDL_FreeAudioMem(current_audio->fake_stream);
		}
		if ( current_audio->convert.needed && 
				current_audio->convert.buf) {
			SDL_FreeAudioMem(current_audio->convert.buf);
		}

		current_audio->free(current_audio);
		current_audio = NULL;
	}
}

#define NUM_FORMATS	6
static int format_idx;
static int format_idx_sub;
static Uint16 format_list[NUM_FORMATS][NUM_FORMATS] = {
 { AUDIO_U8, AUDIO_S8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB },
 { AUDIO_S8, AUDIO_U8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB },
 { AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8 },
 { AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8 },
 { AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U8, AUDIO_S8 },
 { AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_U8, AUDIO_S8 },
};

Uint16 SDL_FirstAudioFormat(Uint16 format)
{
	for ( format_idx=0; format_idx < NUM_FORMATS; ++format_idx ) {
		if ( format_list[format_idx][0] == format ) {
			break;
		}
	}
	format_idx_sub = 0;
	return(SDL_NextAudioFormat());
}

Uint16 SDL_NextAudioFormat(void)
{
	if ( (format_idx == NUM_FORMATS) || (format_idx_sub == NUM_FORMATS) ) {
		return(0);
	}
	return(format_list[format_idx][format_idx_sub++]);
}

void SDL_CalculateAudioSpec(SDL_AudioSpec *spec)
{
	switch (spec->format) {
		case AUDIO_U8:
			spec->silence = 0x80;
			break;
		default:
			spec->silence = 0x00;
			break;
	}
	spec->size = (spec->format&0xFF)/8;
	spec->size *= spec->channels;
	spec->size *= spec->samples;
}
