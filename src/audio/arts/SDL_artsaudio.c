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

/* Allow access to a raw mixing buffer */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_audiomem.h"
#include "SDL_audio_c.h"
#include "SDL_timer.h"
#include "SDL_audiodev_c.h"
#include "SDL_artsaudio.h"

#ifdef ARTSC_DYNAMIC
#include "SDL_name.h"
#include "SDL_loadso.h"
#else
#define SDL_NAME(X)	X
#endif

/* The tag name used by artsc audio */
#define ARTSC_DRIVER_NAME         "artsc"

/* Audio driver functions */
static int ARTSC_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void ARTSC_WaitAudio(_THIS);
static void ARTSC_PlayAudio(_THIS);
static Uint8 *ARTSC_GetAudioBuf(_THIS);
static void ARTSC_CloseAudio(_THIS);

#ifdef ARTSC_DYNAMIC

static const char *arts_library = ARTSC_DYNAMIC;
static void *arts_handle = NULL;
static int arts_loaded = 0;

static int (*SDL_NAME(arts_init))(void);
static void (*SDL_NAME(arts_free))(void);
static arts_stream_t (*SDL_NAME(arts_play_stream))(int rate, int bits, int channels, const char *name);
static int (*SDL_NAME(arts_stream_set))(arts_stream_t s, arts_parameter_t param, int value);
static int (*SDL_NAME(arts_stream_get))(arts_stream_t s, arts_parameter_t param);
static int (*SDL_NAME(arts_write))(arts_stream_t s, const void *buffer, int count);
static void (*SDL_NAME(arts_close_stream))(arts_stream_t s);

static struct {
	const char *name;
	void **func;
} arts_functions[] = {
	{ "arts_init",		(void **)&SDL_NAME(arts_init)		},
	{ "arts_free",		(void **)&SDL_NAME(arts_free)		},
	{ "arts_play_stream",	(void **)&SDL_NAME(arts_play_stream)	},
	{ "arts_stream_set",	(void **)&SDL_NAME(arts_stream_set)	},
	{ "arts_stream_get",	(void **)&SDL_NAME(arts_stream_get)	},
	{ "arts_write",		(void **)&SDL_NAME(arts_write)		},
	{ "arts_close_stream",	(void **)&SDL_NAME(arts_close_stream)	},
};

static void UnloadARTSLibrary()
{
	if ( arts_loaded ) {
		SDL_UnloadObject(arts_handle);
		arts_handle = NULL;
		arts_loaded = 0;
	}
}

static int LoadARTSLibrary(void)
{
	int i, retval = -1;

	arts_handle = SDL_LoadObject(arts_library);
	if ( arts_handle ) {
		arts_loaded = 1;
		retval = 0;
		for ( i=0; i<SDL_TABLESIZE(arts_functions); ++i ) {
			*arts_functions[i].func = SDL_LoadFunction(arts_handle, arts_functions[i].name);
			if ( ! arts_functions[i].func ) {
				retval = -1;
				UnloadARTSLibrary();
				break;
			}
		}
	}
	return retval;
}

#else

static void UnloadARTSLibrary()
{
	return;
}

static int LoadARTSLibrary(void)
{
	return 0;
}

#endif /* ARTSC_DYNAMIC */

/* Audio driver bootstrap functions */

static int Audio_Available(void)
{
	int available = 0;

	if ( LoadARTSLibrary() < 0 ) {
		return available;
	}
	if ( SDL_NAME(arts_init)() == 0 ) {
		available = 1;
		SDL_NAME(arts_free)();
	}
	UnloadARTSLibrary();

	return available;
}

static void Audio_DeleteDevice(SDL_AudioDevice *device)
{
	free(device->hidden);
	free(device);
	UnloadARTSLibrary();
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	LoadARTSLibrary();
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
	stream = 0;

	/* Set the function pointers */
	this->OpenAudio = ARTSC_OpenAudio;
	this->WaitAudio = ARTSC_WaitAudio;
	this->PlayAudio = ARTSC_PlayAudio;
	this->GetAudioBuf = ARTSC_GetAudioBuf;
	this->CloseAudio = ARTSC_CloseAudio;

	this->free = Audio_DeleteDevice;

	return this;
}

AudioBootStrap ARTSC_bootstrap = {
	ARTSC_DRIVER_NAME, "Analog Realtime Synthesizer",
	Audio_Available, Audio_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void ARTSC_WaitAudio(_THIS)
{
	Sint32 ticks;

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

	/* Use timer for general audio synchronization */
	ticks = ((Sint32)(next_frame - SDL_GetTicks()))-FUDGE_TICKS;
	if ( ticks > 0 ) {
		SDL_Delay(ticks);
	}
}

static void ARTSC_PlayAudio(_THIS)
{
	int written;

	/* Write the audio data */
	written = SDL_NAME(arts_write)(stream, mixbuf, mixlen);
	
	/* If timer synchronization is enabled, set the next write frame */
	if ( frame_ticks ) {
		next_frame += frame_ticks;
	}

	/* If we couldn't write, assume fatal error for now */
	if ( written < 0 ) {
		this->enabled = 0;
	}
#ifdef DEBUG_AUDIO
	fprintf(stderr, "Wrote %d bytes of audio data\n", written);
#endif
}

static Uint8 *ARTSC_GetAudioBuf(_THIS)
{
	return(mixbuf);
}

static void ARTSC_CloseAudio(_THIS)
{
	if ( mixbuf != NULL ) {
		SDL_FreeAudioMem(mixbuf);
		mixbuf = NULL;
	}
	if ( stream ) {
		SDL_NAME(arts_close_stream)(stream);
		stream = 0;
	}
	SDL_NAME(arts_free)();
}

static int ARTSC_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	int bits, frag_spec;
	Uint16 test_format, format;

	/* Reset the timer synchronization flag */
	frame_ticks = 0.0;

	mixbuf = NULL;

	/* Try for a closest match on audio format */
	format = 0;
	bits = 0;
	for ( test_format = SDL_FirstAudioFormat(spec->format);
						! format && test_format; ) {
#ifdef DEBUG_AUDIO
		fprintf(stderr, "Trying format 0x%4.4x\n", test_format);
#endif
		switch ( test_format ) {
			case AUDIO_U8:
				bits = 8;
				format = 1;
				break;
			case AUDIO_S16LSB:
				bits = 16;
				format = 1;
				break;
			default:
				format = 0;
				break;
		}
		if ( ! format ) {
			test_format = SDL_NextAudioFormat();
		}
	}
	if ( format == 0 ) {
		SDL_SetError("Couldn't find any hardware audio formats");
		return(-1);
	}
	spec->format = test_format;

	if ( SDL_NAME(arts_init)() != 0 ) {
		SDL_SetError("Unable to initialize ARTS");
		return(-1);
	}
	stream = SDL_NAME(arts_play_stream)(spec->freq, bits, spec->channels, "SDL");

	/* Calculate the final parameters for this audio specification */
	SDL_CalculateAudioSpec(spec);

	/* Determine the power of two of the fragment size */
	for ( frag_spec = 0; (0x01<<frag_spec) < spec->size; ++frag_spec );
	if ( (0x01<<frag_spec) != spec->size ) {
		SDL_SetError("Fragment size must be a power of two");
		return(-1);
	}
	frag_spec |= 0x00020000;	/* two fragments, for low latency */

#ifdef ARTS_P_PACKET_SETTINGS
	SDL_NAME(arts_stream_set)(stream, ARTS_P_PACKET_SETTINGS, frag_spec);
#else
	SDL_NAME(arts_stream_set)(stream, ARTS_P_PACKET_SIZE, frag_spec&0xffff);
	SDL_NAME(arts_stream_set)(stream, ARTS_P_PACKET_COUNT, frag_spec>>16);
#endif
	spec->size = SDL_NAME(arts_stream_get)(stream, ARTS_P_PACKET_SIZE);

	/* Allocate mixing buffer */
	mixlen = spec->size;
	mixbuf = (Uint8 *)SDL_AllocAudioMem(mixlen);
	if ( mixbuf == NULL ) {
		return(-1);
	}
	memset(mixbuf, spec->silence, spec->size);

	/* Get the parent process id (we're the parent of the audio thread) */
	parent = getpid();

	/* We're ready to rock and roll. :-) */
	return(0);
}
