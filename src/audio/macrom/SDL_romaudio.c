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
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#if TARGET_API_MAC_CARBON
#  include <Carbon.h>
#else
#  include <Sound.h> /* SoundManager interface */
#  include <Gestalt.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "SDL_endian.h"
#include "SDL_audio.h"
#include "SDL_audio_c.h"
#include "SDL_audiomem.h"
#include "SDL_sysaudio.h"
#include "SDL_romaudio.h"

/* Audio driver functions */

static void Mac_CloseAudio(_THIS);
static int Mac_OpenAudio(_THIS, SDL_AudioSpec *spec);

/* Audio driver bootstrap functions */


static int Audio_Available(void)
{
    return(1);
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
    this->OpenAudio   = Mac_OpenAudio;
    this->CloseAudio  = Mac_CloseAudio;
    this->free        = Audio_DeleteDevice;

    return this;
}

AudioBootStrap SNDMGR_bootstrap = {
	"sndmgr", "MacOS SoundManager 3.0",
	Audio_Available, Audio_CreateDevice
};

#if TARGET_API_MAC_CARBON

static UInt8  *buffer[2];
static volatile UInt32 running = 0;
static CmpSoundHeader header;

static void callBackProc (SndChannel *chan, SndCommand *cmd_passed ) {
   
   UInt32 fill_me, play_me;
   SndCommand cmd; 
   SDL_AudioDevice *audio = (SDL_AudioDevice *)chan->userInfo;
   
   fill_me = cmd_passed->param2;  /* buffer that has just finished playing, so fill it */      
   play_me = ! fill_me;           /* filled buffer to play _now_ */

   if ( ! audio->enabled ) {
      return;
   }
   
   header.samplePtr = (Ptr)buffer[play_me];
   
   cmd.cmd = bufferCmd;
   cmd.param1 = 0; 
   cmd.param2 = (long)&header;

   SndDoCommand (chan, &cmd, 0);
   
   memset (buffer[fill_me], 0, audio->spec.size);
   
   if ( ! audio->paused ) {
        if ( audio->convert.needed ) {
            #if MACOSX
                SDL_mutexP(audio->mixer_lock);
            #endif
                audio->spec.callback(audio->spec.userdata,
                    (Uint8 *)audio->convert.buf,audio->convert.len);
            #if MACOSX
                SDL_mutexV(audio->mixer_lock);
            #endif 
               SDL_ConvertAudio(&audio->convert);
#if 0
            if ( audio->convert.len_cvt != audio->spec.size ) {
                /* Uh oh... probably crashes here; */
            }
#endif
            memcpy(buffer[fill_me], audio->convert.buf,
                            audio->convert.len_cvt);
        } else {
            #if MACOSX
                SDL_mutexP(audio->mixer_lock);
            #endif
            audio->spec.callback(audio->spec.userdata,
                (Uint8 *)buffer[fill_me], audio->spec.size);
            #if MACOSX
                SDL_mutexV(audio->mixer_lock);
            #endif
        }
    }

    if ( running ) {
         
      cmd.cmd = callBackCmd;
      cmd.param1 = 0;
      cmd.param2 = play_me;
   
      SndDoCommand (chan, &cmd, 0);
   }

}

static int Mac_OpenAudio(_THIS, SDL_AudioSpec *spec) {

   SndCallBackUPP callback;
   int sample_bits;
   int i;
   long initOptions;
   
   /* Very few conversions are required, but... */
    switch (spec->format) {
        case AUDIO_S8:
        spec->format = AUDIO_U8;
        break;
        case AUDIO_U16LSB:
        spec->format = AUDIO_S16LSB;
        break;
        case AUDIO_U16MSB:
        spec->format = AUDIO_S16MSB;
        break;
    }
    SDL_CalculateAudioSpec(spec);
    
    /* initialize bufferCmd header */
    memset (&header, 0, sizeof(header));
    callback = NewSndCallBackUPP (callBackProc);
    sample_bits = spec->size / spec->samples / spec->channels * 8;

#ifdef DEBUG_AUDIO
    fprintf(stderr,
	"Audio format 0x%x, channels = %d, sample_bits = %d, frequency = %d\n",
	spec->format, spec->channels, sample_bits, spec->freq);
#endif /* DEBUG_AUDIO */
    
    header.numChannels = spec->channels;
    header.sampleSize  = sample_bits;
    header.sampleRate  = spec->freq << 16;
    header.numFrames   = spec->samples;
    header.encode      = cmpSH;
    
    /* Note that we install the 16bitLittleEndian Converter if needed. */
    if ( spec->format == 0x8010 ) {
        header.compressionID = fixedCompression;
        header.format = k16BitLittleEndianFormat;
    }
    
    /* allocate 2 buffers */
    for (i=0; i<2; i++) {
       buffer[i] = (UInt8*)malloc (sizeof(UInt8) * spec->size);
      if (buffer[i] == NULL) {
         SDL_OutOfMemory();
         return (-1);
      }
     memset (buffer[i], 0, spec->size);
   }
   
   /* Create the sound manager channel */
    channel = (SndChannelPtr)malloc(sizeof(*channel));
    if ( channel == NULL ) {
        SDL_OutOfMemory();
        return(-1);
    }
    if ( spec->channels >= 2 ) {
        initOptions = initStereo;
    } else {
        initOptions = initMono;
    }
    channel->userInfo = (long)this;
    channel->qLength = 128;
    if ( SndNewChannel(&channel, sampledSynth, initOptions, callback) !=
noErr ) {
        SDL_SetError("Unable to create audio channel");
        free(channel);
        channel = NULL;
        return(-1);
    }
   
   /* start playback */
   {
      SndCommand cmd;
      cmd.cmd = callBackCmd;
      cmd.param2 = 0;
      running = 1;
      SndDoCommand (channel, &cmd, 0);
   }
   
   return 1;
}

static void Mac_CloseAudio(_THIS) {
   
   int i;
   
   running = 0;
   
   if (channel) {
      SndDisposeChannel (channel, true);
      channel = NULL;
   }
   
    for ( i=0; i<2; ++i ) {
        if ( buffer[i] ) {
            free(buffer[i]);
            buffer[i] = NULL;
        }
    }
}

#else /* !TARGET_API_MAC_CARBON */

/* This function is called by Sound Manager when it has exhausted one of
   the buffers, so we'll zero it to silence and fill it with audio if
   we're not paused.
*/
static pascal
void sndDoubleBackProc (SndChannelPtr chan, SndDoubleBufferPtr newbuf)
{
    SDL_AudioDevice *audio = (SDL_AudioDevice *)newbuf->dbUserInfo[0];

    /* If audio is quitting, don't do anything */
    if ( ! audio->enabled ) {
        return;
    }
    memset (newbuf->dbSoundData, 0, audio->spec.size);
    newbuf->dbNumFrames = audio->spec.samples;
    if ( ! audio->paused ) {
        if ( audio->convert.needed ) {
            audio->spec.callback(audio->spec.userdata,
                (Uint8 *)audio->convert.buf,audio->convert.len);
            SDL_ConvertAudio(&audio->convert);
#if 0
            if ( audio->convert.len_cvt != audio->spec.size ) {
                /* Uh oh... probably crashes here */;
            }
#endif
            memcpy(newbuf->dbSoundData, audio->convert.buf,
                            audio->convert.len_cvt);
        } else {
            audio->spec.callback(audio->spec.userdata,
                (Uint8 *)newbuf->dbSoundData, audio->spec.size);
        }
    }
    newbuf->dbFlags    |= dbBufferReady;
}

static int DoubleBufferAudio_Available(void)
{
    int available;
    NumVersion sndversion;
    long response;

    available = 0;
    sndversion = SndSoundManagerVersion();
    if ( sndversion.majorRev >= 3 ) {
        if ( Gestalt(gestaltSoundAttr, &response) == noErr ) {
            if ( (response & (1 << gestaltSndPlayDoubleBuffer)) ) {
                available = 1;
            }
        }
    } else {
        if ( Gestalt(gestaltSoundAttr, &response) == noErr ) {
            if ( (response & (1 << gestaltHasASC)) ) {
                available = 1;
            }
        }
    }
    return(available);
}

static void Mac_CloseAudio(_THIS)
{
    int i;

    if ( channel != NULL ) {
#if 0
        SCStatus status;

        /* Wait for audio to complete */
        do {
            SndChannelStatus(channel, sizeof(status), &status);
        } while ( status.scChannelBusy );
#endif
        /* Clean up the audio channel */
        SndDisposeChannel(channel, true);
        channel = NULL;
    }
    for ( i=0; i<2; ++i ) {
        if ( audio_buf[i] ) {
            free(audio_buf[i]);
            audio_buf[i] = NULL;
        }
    }
}

static int Mac_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
    SndDoubleBufferHeader2 audio_dbh;
    int i;
    long initOptions;
    int sample_bits;
    SndDoubleBackUPP doubleBackProc;

    /* Check to make sure double-buffered audio is available */
    if ( ! DoubleBufferAudio_Available() ) {
        SDL_SetError("Sound manager doesn't support double-buffering");
        return(-1);
    }

    /* Very few conversions are required, but... */
    switch (spec->format) {
        case AUDIO_S8:
        spec->format = AUDIO_U8;
        break;
        case AUDIO_U16LSB:
        spec->format = AUDIO_S16LSB;
        break;
        case AUDIO_U16MSB:
        spec->format = AUDIO_S16MSB;
        break;
    }
    SDL_CalculateAudioSpec(spec);

    /* initialize the double-back header */
    memset(&audio_dbh, 0, sizeof(audio_dbh));
    doubleBackProc = NewSndDoubleBackProc (sndDoubleBackProc);
    sample_bits = spec->size / spec->samples / spec->channels * 8;
    
    audio_dbh.dbhNumChannels = spec->channels;
    audio_dbh.dbhSampleSize    = sample_bits;
    audio_dbh.dbhCompressionID = 0;
    audio_dbh.dbhPacketSize    = 0;
    audio_dbh.dbhSampleRate    = spec->freq << 16;
    audio_dbh.dbhDoubleBack    = doubleBackProc;
    audio_dbh.dbhFormat    = 0;

    /* Note that we install the 16bitLittleEndian Converter if needed. */
    if ( spec->format == 0x8010 ) {
        audio_dbh.dbhCompressionID = fixedCompression;
        audio_dbh.dbhFormat = k16BitLittleEndianFormat;
    }

    /* allocate the 2 double-back buffers */
    for ( i=0; i<2; ++i ) {
        audio_buf[i] = calloc(1, sizeof(SndDoubleBuffer)+spec->size);
        if ( audio_buf[i] == NULL ) {
            SDL_OutOfMemory();
            return(-1);
        }
        audio_buf[i]->dbNumFrames = spec->samples;
        audio_buf[i]->dbFlags = dbBufferReady;
        audio_buf[i]->dbUserInfo[0] = (long)this;
        audio_dbh.dbhBufferPtr[i] = audio_buf[i];
    }

    /* Create the sound manager channel */
    channel = (SndChannelPtr)malloc(sizeof(*channel));
    if ( channel == NULL ) {
        SDL_OutOfMemory();
        return(-1);
    }
    if ( spec->channels >= 2 ) {
        initOptions = initStereo;
    } else {
        initOptions = initMono;
    }
    channel->userInfo = 0;
    channel->qLength = 128;
    if ( SndNewChannel(&channel, sampledSynth, initOptions, 0L) != noErr ) {
        SDL_SetError("Unable to create audio channel");
        free(channel);
        channel = NULL;
        return(-1);
    }
 
    /* Start playback */
    if ( SndPlayDoubleBuffer(channel, (SndDoubleBufferHeaderPtr)&audio_dbh)
                                != noErr ) {
        SDL_SetError("Unable to play double buffered audio");
        return(-1);
    }
    
    return 1;
}

#endif /* TARGET_API_MAC_CARBON */


