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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#ifdef OSS_USE_SOUNDCARD_H
/* This is installed on some systems */
#include <soundcard.h>
#else
/* This is recommended by OSS */
#include <sys/soundcard.h>
#endif

#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_audiomem.h"
#include "SDL_audio_c.h"
#include "SDL_timer.h"
#include "SDL_audiodev_c.h"
#include "SDL_dspaudio.h"

/* The tag name used by DSP audio */
#define DSP_DRIVER_NAME         "dsp"

/* Open the audio device for playback, and don't block if busy */
/*#define USE_BLOCKING_WRITES*/
#define OPEN_FLAGS	(O_WRONLY|O_NONBLOCK)

/* Audio driver functions */
static int DSP_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void DSP_WaitAudio(_THIS);
static void DSP_PlayAudio(_THIS);
static Uint8 *DSP_GetAudioBuf(_THIS);
static void DSP_CloseAudio(_THIS);

/* Audio driver bootstrap functions */

static int Audio_Available(void)
{
	int fd;
	int available;

	available = 0;
	fd = SDL_OpenAudioPath(NULL, 0, OPEN_FLAGS, 0);
	if ( fd >= 0 ) {
		available = 1;
		close(fd);
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
	audio_fd = -1;

	/* Set the function pointers */
	this->OpenAudio = DSP_OpenAudio;
	this->WaitAudio = DSP_WaitAudio;
	this->PlayAudio = DSP_PlayAudio;
	this->GetAudioBuf = DSP_GetAudioBuf;
	this->CloseAudio = DSP_CloseAudio;

	this->free = Audio_DeleteDevice;

	return this;
}

AudioBootStrap DSP_bootstrap = {
	DSP_DRIVER_NAME, "OSS /dev/dsp standard audio",
	Audio_Available, Audio_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void DSP_WaitAudio(_THIS)
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

#ifndef USE_BLOCKING_WRITES /* Not necessary when using blocking writes */
	/* See if we need to use timed audio synchronization */
	if ( frame_ticks ) {
		/* Use timer for general audio synchronization */
		Sint32 ticks;

		ticks = ((Sint32)(next_frame - SDL_GetTicks()))-FUDGE_TICKS;
		if ( ticks > 0 ) {
			SDL_Delay(ticks);
		}
	} else {
		/* Use select() for audio synchronization */
		fd_set fdset;
		struct timeval timeout;

		FD_ZERO(&fdset);
		FD_SET(audio_fd, &fdset);
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
#ifdef DEBUG_AUDIO
		fprintf(stderr, "Waiting for audio to get ready\n");
#endif
		if ( select(audio_fd+1, NULL, &fdset, NULL, &timeout) <= 0 ) {
			const char *message =
			"Audio timeout - buggy audio driver? (disabled)";
			/* In general we should never print to the screen,
			   but in this case we have no other way of letting
			   the user know what happened.
			*/
			fprintf(stderr, "SDL: %s\n", message);
			this->enabled = 0;
			/* Don't try to close - may hang */
			audio_fd = -1;
#ifdef DEBUG_AUDIO
			fprintf(stderr, "Done disabling audio\n");
#endif
		}
#ifdef DEBUG_AUDIO
		fprintf(stderr, "Ready!\n");
#endif
	}
#endif /* !USE_BLOCKING_WRITES */
}

static void DSP_PlayAudio(_THIS)
{
	int written, p=0;

	/* Write the audio data, checking for EAGAIN on broken audio drivers */
	do {
		written = write(audio_fd, &mixbuf[p], mixlen-p);
		if (written>0)
		   p += written;
		if (written == -1 && errno != 0 && errno != EAGAIN && errno != EINTR)
		{
		   /* Non recoverable error has occurred. It should be reported!!! */
		   perror("audio");
		   break;
		}

		if ( p < written || ((written < 0) && ((errno == 0) || (errno == EAGAIN))) ) {
			SDL_Delay(1);	/* Let a little CPU time go by */
		}
	} while ( p < written );

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

static Uint8 *DSP_GetAudioBuf(_THIS)
{
	return(mixbuf);
}

static void DSP_CloseAudio(_THIS)
{
	if ( mixbuf != NULL ) {
		SDL_FreeAudioMem(mixbuf);
		mixbuf = NULL;
	}
	if ( audio_fd >= 0 ) {
		int value;
		ioctl(audio_fd, SNDCTL_DSP_RESET, &value);
		close(audio_fd);
		audio_fd = -1;
	}
}

static int DSP_ReopenAudio(_THIS, const char *audiodev, int format,
						SDL_AudioSpec *spec)
{
	int frag_spec;
	int value;

	/* Close and then reopen the audio device */
	close(audio_fd);
	audio_fd = open(audiodev, O_WRONLY, 0);
	if ( audio_fd < 0 ) {
		SDL_SetError("Couldn't open %s: %s", audiodev, strerror(errno));
		return(-1);
	}

	/* Calculate the final parameters for this audio specification */
	SDL_CalculateAudioSpec(spec);

	/* Determine the power of two of the fragment size */
	for ( frag_spec = 0; (0x01<<frag_spec) < spec->size; ++frag_spec );
	if ( (0x01<<frag_spec) != spec->size ) {
		SDL_SetError("Fragment size must be a power of two");
		return(-1);
	}
	frag_spec |= 0x00020000;	/* two fragments, for low latency */

	/* Set the audio buffering parameters */
#ifdef DEBUG_AUDIO
	fprintf(stderr, "Requesting %d fragments of size %d\n",
		(frag_spec >> 16), 1<<(frag_spec&0xFFFF));
#endif
	if ( ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &frag_spec) < 0 ) {
		fprintf(stderr, "Warning: Couldn't set audio fragment size\n");
	}
#ifdef DEBUG_AUDIO
	{ audio_buf_info info;
	  ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info);
	  fprintf(stderr, "fragments = %d\n", info.fragments);
	  fprintf(stderr, "fragstotal = %d\n", info.fragstotal);
	  fprintf(stderr, "fragsize = %d\n", info.fragsize);
	  fprintf(stderr, "bytes = %d\n", info.bytes);
	}
#endif

	/* Set the audio format */
	value = format;
	if ( (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &value) < 0) ||
						(value != format) ) {
		SDL_SetError("Couldn't set audio format");
		return(-1);
	}

	/* Set the number of channels of output */
	value = spec->channels;
#ifdef SNDCTL_DSP_CHANNELS
	if ( ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &value) < 0 ) {
#endif
		value = (spec->channels > 1);
		ioctl(audio_fd, SNDCTL_DSP_STEREO, &value);
		value = (value ? 2 : 1);
#ifdef SNDCTL_DSP_CHANNELS
	}
#endif
	if ( value != spec->channels ) {
		SDL_SetError("Couldn't set audio channels");
		return(-1);
	}

	/* Set the DSP frequency */
	value = spec->freq;
	if ( ioctl(audio_fd, SOUND_PCM_WRITE_RATE, &value) < 0 ) {
		SDL_SetError("Couldn't set audio frequency");
		return(-1);
	}
	spec->freq = value;

	/* We successfully re-opened the audio */
	return(0);
}

static int DSP_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	char audiodev[1024];
	int format;
	int value;
	Uint16 test_format;

	/* Reset the timer synchronization flag */
	frame_ticks = 0.0;

	/* Open the audio device */
	audio_fd = SDL_OpenAudioPath(audiodev, sizeof(audiodev), OPEN_FLAGS, 0);
	if ( audio_fd < 0 ) {
		SDL_SetError("Couldn't open %s: %s", audiodev, strerror(errno));
		return(-1);
	}
	mixbuf = NULL;

#ifdef USE_BLOCKING_WRITES
	/* Make the file descriptor use blocking writes with fcntl() */
	{ long flags;
		flags = fcntl(audio_fd, F_GETFL);
		flags &= ~O_NONBLOCK;
		if ( fcntl(audio_fd, F_SETFL, flags) < 0 ) {
			SDL_SetError("Couldn't set audio blocking mode");
			return(-1);
		}
	}
#endif

	/* Get a list of supported hardware formats */
	if ( ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &value) < 0 ) {
		SDL_SetError("Couldn't get audio format list");
		return(-1);
	}

	/* Try for a closest match on audio format */
	format = 0;
	for ( test_format = SDL_FirstAudioFormat(spec->format);
						! format && test_format; ) {
#ifdef DEBUG_AUDIO
		fprintf(stderr, "Trying format 0x%4.4x\n", test_format);
#endif
		switch ( test_format ) {
			case AUDIO_U8:
				if ( value & AFMT_U8 ) {
					format = AFMT_U8;
				}
				break;
			case AUDIO_S8:
				if ( value & AFMT_S8 ) {
					format = AFMT_S8;
				}
				break;
			case AUDIO_S16LSB:
				if ( value & AFMT_S16_LE ) {
					format = AFMT_S16_LE;
				}
				break;
			case AUDIO_S16MSB:
				if ( value & AFMT_S16_BE ) {
					format = AFMT_S16_BE;
				}
				break;
			case AUDIO_U16LSB:
				if ( value & AFMT_U16_LE ) {
					format = AFMT_U16_LE;
				}
				break;
			case AUDIO_U16MSB:
				if ( value & AFMT_U16_BE ) {
					format = AFMT_U16_BE;
				}
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

	/* Set the audio format */
	value = format;
	if ( (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &value) < 0) ||
						(value != format) ) {
		SDL_SetError("Couldn't set audio format");
		return(-1);
	}

	/* Set the number of channels of output */
	value = spec->channels;
#ifdef SNDCTL_DSP_CHANNELS
	if ( ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &value) < 0 ) {
#endif
		value = (spec->channels > 1);
		ioctl(audio_fd, SNDCTL_DSP_STEREO, &value);
		value = (value ? 2 : 1);
#ifdef SNDCTL_DSP_CHANNELS
	}
#endif
	spec->channels = value;

	/* Because some drivers don't allow setting the buffer size
	   after setting the format, we must re-open the audio device
	   once we know what format and channels are supported
	 */
	if ( DSP_ReopenAudio(this, audiodev, format, spec) < 0 ) {
		/* Error is set by DSP_ReopenAudio() */
		return(-1);
	}

	/* Allocate mixing buffer */
	mixlen = spec->size;
	mixbuf = (Uint8 *)SDL_AllocAudioMem(mixlen);
	if ( mixbuf == NULL ) {
		return(-1);
	}
	memset(mixbuf, spec->silence, spec->size);

#ifndef USE_BLOCKING_WRITES
	/* Check to see if we need to use select() workaround */
	{ char *workaround;
		workaround = getenv("SDL_DSP_NOSELECT");
		if ( workaround ) {
			frame_ticks = (float)(spec->samples*1000)/spec->freq;
			next_frame = SDL_GetTicks()+frame_ticks;
		}
	}
#endif /* !USE_BLOCKING_WRITES */

	/* Get the parent process id (we're the parent of the audio thread) */
	parent = getpid();

	/* We're ready to rock and roll. :-) */
	return(0);
}
