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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sched.h>
#include <sys/asoundlib.h>
#include <sys/select.h>

#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_audiomem.h"
#include "SDL_audio_c.h"
#include "SDL_timer.h"
#include "SDL_nto_audio.h"

/* The tag name used by NTO audio */
#define DRIVER_NAME         "nto"

/* default channel communication parameters */
#define DEFAULT_CPARAMS_RATE 22050
#define DEFAULT_CPARAMS_VOICES 1
#define DEFAULT_CPARAMS_FRAG_SIZE 4096
#define DEFAULT_CPARAMS_FRAGS_MIN 1
#define DEFAULT_CPARAMS_FRAGS_MAX 1

/* Open the audio device for playback, and don't block if busy */
#define OPEN_FLAGS SND_PCM_OPEN_PLAYBACK

/* Audio driver functions */
static int NTO_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void NTO_WaitAudio(_THIS);
static void NTO_PlayAudio(_THIS);
static Uint8 *NTO_GetAudioBuf(_THIS);
static void NTO_CloseAudio(_THIS);

static snd_pcm_channel_status_t cstatus;
static snd_pcm_channel_params_t cparams;
static snd_pcm_channel_setup_t  csetup;

/* PCM transfer channel parameters initialize function */
static void init_pcm_cparams(snd_pcm_channel_params_t* cparams)
{
	memset(cparams,0,sizeof(snd_pcm_channel_params_t));

	cparams->channel = SND_PCM_CHANNEL_PLAYBACK;
	cparams->mode = SND_PCM_MODE_BLOCK;
	cparams->start_mode = SND_PCM_START_DATA;
	cparams->stop_mode  = SND_PCM_STOP_STOP;
	cparams->format.format = SND_PCM_SFMT_S16_LE;
	cparams->format.interleave = 1;
	cparams->format.rate = DEFAULT_CPARAMS_RATE;
	cparams->format.voices = DEFAULT_CPARAMS_VOICES;
	cparams->buf.block.frag_size = DEFAULT_CPARAMS_FRAG_SIZE;
	cparams->buf.block.frags_min = DEFAULT_CPARAMS_FRAGS_MIN;
	cparams->buf.block.frags_max = DEFAULT_CPARAMS_FRAGS_MAX;
}

static int Audio_Available(void)
{
	/*
		See if we can open a nonblocking channel.
		Return value '1' means we can.
		Return value '0' means we cannot.
	*/

	int available;
	int rval;
	snd_pcm_t *handle;

	available = 0;
	handle = NULL;

	rval = snd_pcm_open_preferred(&handle, NULL, NULL, OPEN_FLAGS);

	if (rval >= 0){
		available = 1;
		
		if ((rval = snd_pcm_close(handle)) < 0){
			SDL_SetError("snd_pcm_close failed: %s\n",snd_strerror(rval));
			available = 0;
		}
	}
	else{
		SDL_SetError("snd_pcm_open failed: %s\n", snd_strerror(rval));
	}
	
	#ifdef DEBUG_AUDIO
	fprintf(stderr,"AudioAvailable rtns %d\n", available);
	#endif
	
	return(available);
}

static void Audio_DeleteDevice(SDL_AudioDevice *device)
{
	#ifdef DEBUG_AUDIO
	fprintf(stderr,"Audio_DeleteDevice\n");
	#endif

	free(device->hidden);
	free(device);
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;
	#ifdef DEBUG_AUDIO
	fprintf(stderr,"Audio_CreateDevice\n");
	#endif
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
	audio_handle = NULL;

	/* Set the function pointers */
	this->OpenAudio = NTO_OpenAudio;
	this->WaitAudio = NTO_WaitAudio;
	this->PlayAudio = NTO_PlayAudio;
	this->GetAudioBuf = NTO_GetAudioBuf;
	this->CloseAudio = NTO_CloseAudio;

	this->free = Audio_DeleteDevice;

	return this;
}

AudioBootStrap QNXNTOAUDIO_bootstrap = {
	DRIVER_NAME, "QNX6 NTO PCM audio",
	Audio_Available, Audio_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void NTO_WaitAudio(_THIS)
{
	fd_set wfds;

	FD_SET( audio_fd, &wfds );
	switch( select( audio_fd + 1, NULL, &wfds, NULL, NULL ) )
	{
		case -1:
		case 0:
			/* Error */
			SDL_SetError("select() in NTO_WaitAudio failed: %s\n", strerror(errno));
			break;
		default:
			if(FD_ISSET(audio_fd, &wfds))
				return;
	}
}

static void NTO_PlayAudio(_THIS)
{
	int written, rval;
	int towrite;

	#ifdef DEBUG_AUDIO
	fprintf(stderr, "NTO_PlayAudio\n");
	#endif

	if( !this->enabled){
		return;
	}

	towrite = pcm_len;
	
	/* Write the audio data, checking for EAGAIN (buffer full) and underrun */
	do {
		written = snd_pcm_plugin_write(audio_handle, pcm_buf, towrite);
		#ifdef DEBUG_AUDIO
		fprintf(stderr, "NTO_PlayAudio: written = %d towrite = %d\n",written,towrite);
		#endif
		if (written != towrite){
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)){
				SDL_Delay(1);   /* Let a little CPU time go by and try to write again */
				#ifdef DEBUG_AUDIO
				fprintf(stderr, "errno == EAGAIN written %d\n", written);
				#endif
				towrite -= written; //we wrote some data
				continue;
			}		
			else if((errno == EINVAL) || (errno == EIO)){
				if(errno == EIO){
					#ifdef DEBUG_AUDIO
					fprintf(stderr,"snd_pcm_plugin_write failed EIO: %s\n", snd_strerror(written));
					#endif
				}
				if(errno == EINVAL){
					#ifdef DEBUG_AUDIO
					fprintf(stderr,"snd_pcm_plugin_write failed EINVAL: %s\n", snd_strerror(written));	
					#endif
				}
				
				memset(&cstatus, 0, sizeof(cstatus));
				if( (rval = snd_pcm_plugin_status(audio_handle, &cstatus)) < 0 ){
					#ifdef DEBUG_AUDIO
					fprintf(stderr, "snd_pcm_plugin_status failed %s\n",snd_strerror(rval));
					#endif
					SDL_SetError("snd_pcm_plugin_status failed: %s\n", snd_strerror(rval));
					return;
				}	
		        
				if ( (cstatus.status == SND_PCM_STATUS_UNDERRUN) || (cstatus.status == SND_PCM_STATUS_READY) ){
					#ifdef DEBUG_AUDIO
					fprintf(stderr, "buffer underrun\n");
					#endif
					if ( (rval = snd_pcm_plugin_prepare (audio_handle,SND_PCM_CHANNEL_PLAYBACK)) < 0 ){
						#ifdef DEBUG_AUDIO
						fprintf(stderr, "NTO_PlayAudio: prepare failed %s\n",snd_strerror(rval));
						#endif
						SDL_SetError("snd_pcm_plugin_prepare failed: %s\n",snd_strerror(rval) );
						return;
					}
				}		        		
 				continue;
			}
			else{
				#ifdef DEBUG_AUDIO
				fprintf(stderr, "NTO_PlayAudio: snd_pcm_plugin_write failed unknown errno %d %s\n",errno, snd_strerror(rval));
				#endif
				return;
			}
			
		}
		else
		{
			towrite -= written; //we wrote all remaining data
		}
	} while ( (towrite > 0)  && (this->enabled) );

	/* If we couldn't write, assume fatal error for now */
	if ( towrite != 0 ) {
		this->enabled = 0;
	}
	return;
}

static Uint8 *NTO_GetAudioBuf(_THIS)
{
	#ifdef DEBUG_AUDIO
	fprintf(stderr, "NTO_GetAudioBuf: pcm_buf %X\n",(Uint8 *)pcm_buf);
	#endif
	return(pcm_buf);
}

static void NTO_CloseAudio(_THIS)
{
	int rval;

	#ifdef DEBUG_AUDIO
	fprintf(stderr, "NTO_CloseAudio\n");
	#endif

	this->enabled = 0;

	if ( audio_handle != NULL ) {
		if ((rval = snd_pcm_plugin_flush(audio_handle,SND_PCM_CHANNEL_PLAYBACK)) < 0){
			SDL_SetError("snd_pcm_plugin_flush failed: %s\n",snd_strerror(rval));
			return;
		}
		if ((rval = snd_pcm_close(audio_handle)) < 0){
			SDL_SetError("snd_pcm_close failed: %s\n",snd_strerror(rval));
			return;
		}
		audio_handle = NULL;
	}
}

static int NTO_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	int rval;
	int format;
	Uint16 test_format;
	int twidth;
	int found;

	#ifdef DEBUG_AUDIO
	fprintf(stderr, "NTO_OpenAudio\n");
	#endif
	
	audio_handle = NULL;
	this->enabled = 0;

	if ( pcm_buf != NULL ) {
		free((Uint8 *)pcm_buf); 
		pcm_buf = NULL;
	}
	 
	/* initialize channel transfer parameters to default */
	init_pcm_cparams(&cparams);

	/* Open the audio device */
	rval = snd_pcm_open_preferred(&audio_handle, NULL, NULL, OPEN_FLAGS);
	if ( rval < 0 ) {
		SDL_SetError("snd_pcm_open failed: %s\n", snd_strerror(rval));
		return(-1);
	}

	/* enable count status parameter */
	if ((rval = snd_pcm_plugin_set_disable(audio_handle, PLUGIN_DISABLE_MMAP))<0){
		SDL_SetError("snd_pcm_plugin_set_disable failed: %s\n", snd_strerror(rval));
		return(-1);
	}

	/* Try for a closest match on audio format */
	format = 0;
 	found = 0; /* can't use format as SND_PCM_SFMT_U8 = 0 in nto */
	for ( test_format = SDL_FirstAudioFormat(spec->format);	!found ; ) 
	{
		#ifdef DEBUG_AUDIO
		fprintf(stderr, "Trying format 0x%4.4x spec->samples %d\n", test_format,spec->samples);
		#endif
		
		/* if match found set format to equivalent ALSA format */
		switch ( test_format ) {
			case AUDIO_U8:
				format = SND_PCM_SFMT_U8;
				found = 1;
				break;
			case AUDIO_S8:
				format = SND_PCM_SFMT_S8;
				found = 1;
				break;
			case AUDIO_S16LSB:
				format = SND_PCM_SFMT_S16_LE;
				found = 1;
				break;
			case AUDIO_S16MSB:
				format = SND_PCM_SFMT_S16_BE;
				found = 1;
				break;
			case AUDIO_U16LSB:
				format = SND_PCM_SFMT_U16_LE;
				found = 1;
				break;
			case AUDIO_U16MSB:
				format = SND_PCM_SFMT_U16_BE;
				found = 1;
				break;
			default:
				break;
		}
		if ( ! found ) {
			test_format = SDL_NextAudioFormat();
		}
	}

	/* assumes test_format not 0 on success */
	if ( test_format == 0 ) {
		SDL_SetError("Couldn't find any hardware audio formats");
		return(-1);
	}

	spec->format = test_format;

	/* Set the audio format */
	cparams.format.format = format;

	/* Set mono or stereo audio (currently only two channels supported) */
	cparams.format.voices = spec->channels;
	
	#ifdef DEBUG_AUDIO
	fprintf(stderr,"intializing channels %d\n", cparams.format.voices);
	#endif

	/* Set rate */
	cparams.format.rate = spec->freq ;

	/* Setup the transfer parameters according to cparams */
	rval = snd_pcm_plugin_params(audio_handle, &cparams);
	if (rval < 0) {
		SDL_SetError("snd_pcm_channel_params failed: %s\n", snd_strerror (rval));
		return(-1);
	}

	/*  Make sure channel is setup right one last time */
	memset( &csetup, 0, sizeof( csetup ) );
	csetup.channel = SND_PCM_CHANNEL_PLAYBACK;
	if ( snd_pcm_plugin_setup( audio_handle, &csetup ) < 0 )
	{
		SDL_SetError("Unable to setup playback channel\n" );
		return(-1);
	}
	else
	{
		#ifdef DEBUG_AUDIO
		fprintf(stderr,"requested format: %d\n",cparams.format.format);
		fprintf(stderr,"requested frag size: %d\n",cparams.buf.block.frag_size);
		fprintf(stderr,"requested max frags: %d\n\n",cparams.buf.block.frags_max);

		fprintf(stderr,"real format: %d\n", csetup.format.format );
		fprintf(stderr,"real frag size : %d\n", csetup.buf.block.frag_size );
		fprintf(stderr,"real max frags : %d\n", csetup.buf.block.frags_max );
		#endif
	}

	/*
		Allocate memory to the audio buffer and initialize with silence (Note that
		buffer size must be a multiple of fragment size, so find closest multiple)
	*/
	
	twidth = snd_pcm_format_width(format);
	if (twidth < 0) {
		printf("snd_pcm_format_width failed\n");
		twidth = 0;
	}

	#ifdef DEBUG_AUDIO
	fprintf(stderr,"format is %d bits wide\n",twidth);
	#endif

	pcm_len = spec->size ;
	
	#ifdef DEBUG_AUDIO
	fprintf(stderr,"pcm_len set to %d\n", pcm_len);
	#endif

	if (pcm_len == 0){
		pcm_len = csetup.buf.block.frag_size;
	}

	pcm_buf = (Uint8*)malloc(pcm_len);
	if (pcm_buf == NULL) {
		SDL_SetError("pcm_buf malloc failed\n");
		return(-1);
	}
	memset(pcm_buf,spec->silence,pcm_len);

	#ifdef DEBUG_AUDIO
	fprintf(stderr,"pcm_buf malloced and silenced.\n");
	#endif

	/* get the file descriptor */
	if( (audio_fd = snd_pcm_file_descriptor(audio_handle, SND_PCM_CHANNEL_PLAYBACK)) < 0){
		fprintf(stderr, "snd_pcm_file_descriptor failed with error code: %d\n", audio_fd);
	}

	/* Trigger audio playback */
	rval = snd_pcm_plugin_prepare( audio_handle, SND_PCM_CHANNEL_PLAYBACK);
	if (rval < 0) {
		SDL_SetError("snd_pcm_plugin_prepare failed: %s\n", snd_strerror (rval));
		return(-1);
	}

	this->enabled = 1;

	/* Get the parent process id (we're the parent of the audio thread) */
	parent = getpid();

	/* We're ready to rock and roll. :-) */
	return(0);
}
