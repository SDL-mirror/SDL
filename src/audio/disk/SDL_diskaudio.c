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

    This file hacked^H^H^H^H^H^Hwritten by Ryan C. Gordon
        (icculus@linuxgames.com)
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Output raw audio data to a file. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_audiomem.h"
#include "SDL_audio_c.h"
#include "SDL_timer.h"
#include "SDL_audiodev_c.h"
#include "SDL_diskaudio.h"

/* The tag name used by DISK audio */
#define DISKAUD_DRIVER_NAME         "disk"

/* environment variables and defaults. */
#define DISKENVR_OUTFILE         "SDL_DISKAUDIOFILE"
#define DISKDEFAULT_OUTFILE      "sdlaudio.raw"
#define DISKENVR_WRITEDELAY      "SDL_DISKAUDIODELAY"
#define DISKDEFAULT_WRITEDELAY   150

/* Audio driver functions */
static int DISKAUD_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void DISKAUD_WaitAudio(_THIS);
static void DISKAUD_PlayAudio(_THIS);
static Uint8 *DISKAUD_GetAudioBuf(_THIS);
static void DISKAUD_CloseAudio(_THIS);

static const char *DISKAUD_GetOutputFilename(void)
{
    const char *envr = getenv(DISKENVR_OUTFILE);
    return((envr != NULL) ? envr : DISKDEFAULT_OUTFILE);
}

/* Audio driver bootstrap functions */
static int DISKAUD_Available(void)
{
#if 0
    int fd;
	int available;
    int exists = 0;
    struct stat statbuf;
    const char *fname = DISKAUD_GetOutputFilename();
	const char *envr = getenv("SDL_AUDIODRIVER");
	available = 0;

	if ((envr) && (strcmp(envr, DISKAUD_DRIVER_NAME) == 0)) {
		if (stat(fname, &statbuf) == 0)
			exists = 1;

		fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		if ( fd != -1 ) {
			available = 1;
			close(fd);
			if (!exists) {
				unlink(fname);
			}
		}
	}
	return(available);
#else
	const char *envr = getenv("SDL_AUDIODRIVER");
	if ((envr) && (strcmp(envr, DISKAUD_DRIVER_NAME) == 0)) {
		return(1);
	}

	return(0);
#endif
}

static void DISKAUD_DeleteDevice(SDL_AudioDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_AudioDevice *DISKAUD_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;
    const char *envr;

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

    envr = getenv(DISKENVR_WRITEDELAY);
    this->hidden->write_delay = (envr) ? atoi(envr) : DISKDEFAULT_WRITEDELAY;

	/* Set the function pointers */
	this->OpenAudio = DISKAUD_OpenAudio;
	this->WaitAudio = DISKAUD_WaitAudio;
	this->PlayAudio = DISKAUD_PlayAudio;
	this->GetAudioBuf = DISKAUD_GetAudioBuf;
	this->CloseAudio = DISKAUD_CloseAudio;

	this->free = DISKAUD_DeleteDevice;

	return this;
}

AudioBootStrap DISKAUD_bootstrap = {
	DISKAUD_DRIVER_NAME, "direct-to-disk audio",
	DISKAUD_Available, DISKAUD_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void DISKAUD_WaitAudio(_THIS)
{
    SDL_Delay(this->hidden->write_delay);
}

static void DISKAUD_PlayAudio(_THIS)
{
	int written;

	/* Write the audio data, checking for EAGAIN on broken audio drivers */
	do {
		written = write(this->hidden->audio_fd,
                        this->hidden->mixbuf,
                        this->hidden->mixlen);
		if ( (written < 0) && ((errno == 0) || (errno == EAGAIN)) ) {
			SDL_Delay(1);	/* Let a little CPU time go by */
		}
	} while ( (written < 0) && 
	          ((errno == 0) || (errno == EAGAIN) || (errno == EINTR)) );

	/* If we couldn't write, assume fatal error for now */
	if ( written < 0 ) {
		this->enabled = 0;
	}
#ifdef DEBUG_AUDIO
	fprintf(stderr, "Wrote %d bytes of audio data\n", written);
#endif
}

static Uint8 *DISKAUD_GetAudioBuf(_THIS)
{
	return(this->hidden->mixbuf);
}

static void DISKAUD_CloseAudio(_THIS)
{
	if ( this->hidden->mixbuf != NULL ) {
		SDL_FreeAudioMem(this->hidden->mixbuf);
		this->hidden->mixbuf = NULL;
	}
	if ( this->hidden->audio_fd >= 0 ) {
		close(this->hidden->audio_fd);
		this->hidden->audio_fd = -1;
	}
}

static int DISKAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
    const char *fname = DISKAUD_GetOutputFilename();

	/* Open the audio device */
    this->hidden->audio_fd = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if ( this->hidden->audio_fd < 0 ) {
		SDL_SetError("Couldn't open %s: %s", fname, strerror(errno));
		return(-1);
	}

    fprintf(stderr, "WARNING: You are using the SDL disk writer"
                    " audio driver!\n Writing to file [%s].\n", fname);

	/* Allocate mixing buffer */
	this->hidden->mixlen = spec->size;
	this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
	if ( this->hidden->mixbuf == NULL ) {
		return(-1);
	}
	memset(this->hidden->mixbuf, spec->silence, spec->size);

	/* We're ready to rock and roll. :-) */
	return(0);
}

