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

/* default card and device numbers as listed in dev/snd */
static int card_no = 0;
static int device_no = 0;

/* default channel communication parameters */
#define DEFAULT_CPARAMS_RATE 22050
#define DEFAULT_CPARAMS_VOICES 1
#define DEFAULT_CPARAMS_FRAG_SIZE 512
#define DEFAULT_CPARAMS_FRAGS_MIN 1
#define DEFAULT_CPARAMS_FRAGS_MAX -1

/* Open the audio device for playback, and don't block if busy */
#define OPEN_FLAGS	(SND_PCM_OPEN_PLAYBACK|SND_PCM_OPEN_NONBLOCK)

/* Audio driver functions */
static int PCM_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void PCM_WaitAudio(_THIS);
static void PCM_PlayAudio(_THIS);
static Uint8 *PCM_GetAudioBuf(_THIS);
static void PCM_CloseAudio(_THIS);

/* PCM transfer channel parameters initialize function */
static void init_pcm_cparams(snd_pcm_channel_params_t* cparams)
{
	memset(cparams,0,sizeof(snd_pcm_channel_params_t));

	cparams->channel = SND_PCM_CHANNEL_PLAYBACK;
	cparams->mode = SND_PCM_MODE_BLOCK;
	cparams->start_mode = SND_PCM_START_DATA; //_FULL
	cparams->stop_mode  = SND_PCM_STOP_STOP;
	cparams->format.format = SND_PCM_SFMT_S16_LE;
	cparams->format.interleave = 1;
	cparams->format.rate = DEFAULT_CPARAMS_RATE;
	cparams->format.voices = DEFAULT_CPARAMS_VOICES;
	cparams->buf.block.frag_size = DEFAULT_CPARAMS_FRAG_SIZE;
	cparams->buf.block.frags_min = DEFAULT_CPARAMS_FRAGS_MIN;
	cparams->buf.block.frags_max = DEFAULT_CPARAMS_FRAGS_MAX;
}

/* Audio driver bootstrap functions */

static int Audio_Available(void)
/*
	See if we can open a nonblocking channel.
	Return value '1' means we can.
	Return value '0' means we cannot.
*/
{
	int available;
	int rval;
	snd_pcm_t *handle;
	snd_pcm_channel_params_t cparams;
#ifdef DEBUG_AUDIO
	snd_pcm_channel_status_t cstatus;
#endif

	available = 0;
	handle = NULL;

	init_pcm_cparams(&cparams);
	
	rval = snd_pcm_open(&handle, card_no, device_no, OPEN_FLAGS);
	if (rval >= 0)
	{
		rval = snd_pcm_plugin_params(handle, &cparams);

#ifdef DEBUG_AUDIO
		snd_pcm_plugin_status(handle, &cstatus);
		printf("status after snd_pcm_plugin_params call = %d\n",cstatus.status);
#endif
		if (rval >= 0)
		{
			available = 1;
		}
		else
		{
	        	SDL_SetError("snd_pcm_channel_params failed: %s\n", snd_strerror (rval));
		}

        if ((rval = snd_pcm_close(handle)) < 0)
        {
            SDL_SetError("snd_pcm_close failed: %s\n",snd_strerror(rval));
			available = 0;
        }
	}
	else
	{
       SDL_SetError("snd_pcm_open failed: %s\n", snd_strerror(rval));
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
	audio_handle = NULL;

	/* Set the function pointers */
	this->OpenAudio = PCM_OpenAudio;
	this->WaitAudio = PCM_WaitAudio;
	this->PlayAudio = PCM_PlayAudio;
	this->GetAudioBuf = PCM_GetAudioBuf;
	this->CloseAudio = PCM_CloseAudio;

	this->free = Audio_DeleteDevice;

	return this;
}

AudioBootStrap ALSA_bootstrap = {
	DRIVER_NAME, "ALSA PCM audio",
	Audio_Available, Audio_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void PCM_WaitAudio(_THIS)
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

	/* See if we need to use timed audio synchronization */
	if ( frame_ticks ) 
	{
		/* Use timer for general audio synchronization */
		Sint32 ticks;

		ticks = ((Sint32)(next_frame - SDL_GetTicks()))-FUDGE_TICKS;
		if ( ticks > 0 ) 
		{
			SDL_Delay(ticks);
		}
	}
    else 
	{
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
	    if ( select(audio_fd+1, NULL, &fdset, NULL, &timeout) <= 0 ) 
		{
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
}

static snd_pcm_channel_status_t cstatus;

static void PCM_PlayAudio(_THIS)
{
    int written, rval;

    /* Write the audio data, checking for EAGAIN (buffer full) and underrun */
    do {
		written = snd_pcm_plugin_write(audio_handle, pcm_buf, pcm_len);
#ifdef DEBUG_AUDIO
		fprintf(stderr, "written = %d pcm_len = %d\n",written,pcm_len);
#endif
		if (written != pcm_len)
		{
	        if (errno == EAGAIN) 
			{
            	SDL_Delay(1);   /* Let a little CPU time go by and try to write again */
#ifdef DEBUG_AUDIO
				fprintf(stderr, "errno == EAGAIN\n");
#endif
        	}
			else
			{
		        if( (rval = snd_pcm_plugin_status(audio_handle, &cstatus)) < 0 )
        		{
		            SDL_SetError("snd_pcm_plugin_status failed: %s\n", snd_strerror(rval));
        		    return;
		        }
				if ( (cstatus.status == SND_PCM_STATUS_UNDERRUN)
					||(cstatus.status == SND_PCM_STATUS_READY) )
				{
#ifdef DEBUG_AUDIO
					fprintf(stderr, "buffer underrun\n");
#endif
					if ( (rval = snd_pcm_plugin_prepare (audio_handle,SND_PCM_CHANNEL_PLAYBACK)) < 0 )
					{
						SDL_SetError("snd_pcm_plugin_prepare failed: %s\n",snd_strerror(rval) );
						return;
					}
					/* if we reach here, try to write again */
				}
			}
		}
    } while ( (written < 0) && ((errno == 0) || (errno == EAGAIN)) );

    /* Set the next write frame */
   if ( frame_ticks ) {
	    next_frame += frame_ticks;
	}

    /* If we couldn't write, assume fatal error for now */
    if ( written < 0 ) {
        this->enabled = 0;
    }
	return;
}

static Uint8 *PCM_GetAudioBuf(_THIS)
{
	return(pcm_buf);
}

static void PCM_CloseAudio(_THIS)
{
	int rval;

	if ( pcm_buf != NULL ) {
		free(pcm_buf);
		pcm_buf = NULL;
	}
	if ( audio_handle != NULL ) {
		if ((rval = snd_pcm_plugin_flush(audio_handle,SND_PCM_CHANNEL_PLAYBACK)) < 0)
		{
        	SDL_SetError("snd_pcm_plugin_flush failed: %s\n",snd_strerror(rval));
			return;
		}
		if ((rval = snd_pcm_close(audio_handle)) < 0)
		{
			SDL_SetError("snd_pcm_close failed: %s\n",snd_strerror(rval));
			return;
		}
		audio_handle = NULL;
	}
}

static int PCM_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	int rval;
	snd_pcm_channel_params_t cparams;
	snd_pcm_channel_setup_t  csetup;
	int format;
	Uint16 test_format;
	int twidth;

	/* initialize channel transfer parameters to default */
	init_pcm_cparams(&cparams);

	/* Reset the timer synchronization flag */
	frame_ticks = 0.0;

	/* Open the audio device */
	
	rval = snd_pcm_open(&audio_handle, card_no, device_no, OPEN_FLAGS);
	if ( rval < 0 ) {
		SDL_SetError("snd_pcm_open failed: %s\n", snd_strerror(rval));
		return(-1);
	}

#ifdef PLUGIN_DISABLE_MMAP /* This is gone in newer versions of ALSA? */
    /* disable count status parameter */
    if ((rval = snd_plugin_set_disable(audio_handle, PLUGIN_DISABLE_MMAP))<0)
    {
        SDL_SetError("snd_plugin_set_disable failed: %s\n", snd_strerror(rval));
        return(-1);
    }
#endif

	pcm_buf = NULL;

	/* Try for a closest match on audio format */
	format = 0;
	for ( test_format = SDL_FirstAudioFormat(spec->format);
						! format && test_format; ) 
	{
#ifdef DEBUG_AUDIO
		fprintf(stderr, "Trying format 0x%4.4x spec->samples %d\n", test_format,spec->samples);
#endif
			/* if match found set format to equivalent ALSA format */
        switch ( test_format ) {
			case AUDIO_U8:
				format = SND_PCM_SFMT_U8;
				cparams.buf.block.frag_size = spec->samples * spec->channels;
				break;
			case AUDIO_S8:
				format = SND_PCM_SFMT_S8;
				cparams.buf.block.frag_size = spec->samples * spec->channels;
				break;
			case AUDIO_S16LSB:
				format = SND_PCM_SFMT_S16_LE;
				cparams.buf.block.frag_size = spec->samples*2 * spec->channels;
				break;
			case AUDIO_S16MSB:
				format = SND_PCM_SFMT_S16_BE;
				cparams.buf.block.frag_size = spec->samples*2 * spec->channels;
				break;
			case AUDIO_U16LSB:
				format = SND_PCM_SFMT_U16_LE;
				cparams.buf.block.frag_size = spec->samples*2 * spec->channels;
				break;
			case AUDIO_U16MSB:
				format = SND_PCM_SFMT_U16_BE;
				cparams.buf.block.frag_size = spec->samples*2 * spec->channels;
				break;
			default:
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
	cparams.format.format = format;

	/* Set mono or stereo audio (currently only two channels supported) */
	cparams.format.voices = spec->channels;
	
	#ifdef DEBUG_AUDIO
	printf("intializing channels %d\n", cparams.format.voices);
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

#ifdef DEBUG_AUDIO
    else
    {
        fprintf(stderr,"requested format: %d\n",cparams.format.format);
        fprintf(stderr,"requested frag size: %d\n",cparams.buf.block.frag_size);
        fprintf(stderr,"requested max frags: %d\n\n",cparams.buf.block.frags_max);

        fprintf(stderr,"real format: %d\n", csetup.format.format );
        fprintf(stderr,"real frag size : %d\n", csetup.buf.block.frag_size );
		fprintf(stderr,"real max frags : %d\n", csetup.buf.block.frags_max );
    }
#endif // DEBUG_AUDIO

    /*  Allocate memory to the audio buffer and initialize with silence
        (Note that buffer size must be a multiple of fragment size, so find closest multiple)
    */
    
    twidth = snd_pcm_format_width(format);
    if (twidth < 0) {
        printf("snd_pcm_format_width failed\n");
        twidth = 0;
    }
#ifdef DEBUG_AUDIO
    printf("format is %d bits wide\n",twidth);
#endif      
    
    pcm_len = csetup.buf.block.frag_size * (twidth/8) * csetup.format.voices ;
    
#ifdef DEBUG_AUDIO    
    printf("pcm_len set to %d\n", pcm_len);
#endif
    
    if (pcm_len == 0)
    {
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
    if( (audio_fd = snd_pcm_file_descriptor(audio_handle, device_no)) < 0)
    {
       fprintf(stderr, "snd_pcm_file_descriptor failed with error code: %d\n", audio_fd);
    }

	/* Trigger audio playback */
	rval = snd_pcm_plugin_prepare( audio_handle, SND_PCM_CHANNEL_PLAYBACK);
	if (rval < 0) {
       SDL_SetError("snd_pcm_plugin_prepare failed: %s\n", snd_strerror (rval));
       return(-1);
	}
	rval =  snd_pcm_playback_go(audio_handle);
    if (rval < 0) {
       SDL_SetError("snd_pcm_playback_go failed: %s\n", snd_strerror (rval));
       return(-1);
    }

    /* Check to see if we need to use select() workaround */
    { char *workaround;
        workaround = getenv("SDL_DSP_NOSELECT");
        if ( workaround ) {
            frame_ticks = (float)(spec->samples*1000)/spec->freq;
            next_frame = SDL_GetTicks()+frame_ticks;
        }
    }

	/* Get the parent process id (we're the parent of the audio thread) */
	parent = getpid();

	/* We're ready to rock and roll. :-) */
	return(0);
}
