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
    slouken@devolution.com
*/


#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include <kernel.h>
#include "swis.h"

#include "SDL_endian.h"
#include "SDL_audio.h"
#include "SDL_audio_c.h"
#include "SDL_audiomem.h"
#include "SDL_sysaudio.h"
#include "SDL_drenderer.h"

#define DigitalRenderer_Activate    0x4F700
#define DigitalRenderer_Deactivate	0x4F701
#define DigitalRenderer_ReadState	0x4F705
#define DigitalRenderer_NewSample	0x4F706
#define DigitalRenderer_NumBuffers	0x4F709
#define DigitalRenderer_StreamSamples  0x4F70A
#define DigitalRenderer_Stream16BitSamples	0x4F70B
#define DigitalRenderer_StreamStatistics	0x4F70C
#define DigitalRenderer_StreamFlags     0x4F70D
#define DigitalRenderer_Activate16		0x4F70F
#define DigitalRenderer_GetFrequency	0x4F710

static int FillBuffer;
extern SDL_AudioDevice *current_audio;

extern int riscos_audiobuffer; /* Override for audio buffer size */

/* Audio driver functions */

static void DRenderer_CloseAudio(_THIS);
static int DRenderer_OpenAudio(_THIS, SDL_AudioSpec *spec);

/* Audio driver bootstrap functions */

/* Define following to dump stats to stdout */
/* #define DUMP_AUDIO */


static int Audio_Available(void)
{
	_kernel_swi_regs regs;
	int available = 0;

	/* Use call to set buffers to also check if Module is loaded */
	regs.r[0] = 0;
	if (_kernel_swi(DigitalRenderer_NumBuffers, &regs, &regs) == 0) available = 1;

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
    this->OpenAudio   = DRenderer_OpenAudio;
    this->CloseAudio  = DRenderer_CloseAudio;
    this->free        = Audio_DeleteDevice;

    return this;
}

AudioBootStrap DRENDERER_bootstrap = {
	"drenderer", "RiscOS Digital Renderer Module",
	Audio_Available, Audio_CreateDevice
};

/* Routine called to check and fill audio buffers if necessary */
static Uint8 *buffer = NULL;

void DRenderer_FillBuffers()
{
	SDL_AudioDevice *audio = current_audio;

   if ( !audio || ! audio->enabled )
   {
      return;
   }

   if ( ! audio->paused )
   {
	   _kernel_swi_regs regs;
	   /* Check filled buffers count */
	   _kernel_swi(DigitalRenderer_StreamStatistics, &regs, &regs);

#ifdef DUMP_AUDIO
    if (regs.r[0] <= FillBuffer)
    {
       printf("Buffers in use %d\n", regs.r[0]);
    }
#endif

	   while (regs.r[0] <= FillBuffer && !audio->paused)
	   {
	     if ( audio->convert.needed )
		   {
		       int silence;
                       if ( audio->convert.src_format == AUDIO_U8 )
                       {
                       	 silence = 0x80;
                       } else {
                       	 silence = 0;
                       }
                       memset(audio->convert.buf, silence, audio->convert.len);
		       audio->spec.callback(audio->spec.userdata,
				   (Uint8 *)audio->convert.buf,audio->convert.len);
                       SDL_ConvertAudio(&audio->convert);
#if 0
			   if ( audio->convert.len_cvt != audio->spec.size ) {
				   /* Uh oh... probably crashes here; */
			   }
#endif
		       regs.r[0] = (int)audio->convert.buf;
		       regs.r[1] = audio->spec.samples * audio->spec.channels;
		       _kernel_swi(DigitalRenderer_Stream16BitSamples, &regs, &regs);

		   } else
		   {
			/* Fill buffer with silence */
			memset (buffer, 0, audio->spec.size);

			audio->spec.callback(audio->spec.userdata,
				   (Uint8 *)buffer, audio->spec.size);

			regs.r[0] = (int)buffer;
			regs.r[1] = audio->spec.samples * audio->spec.channels;
			 _kernel_swi(DigitalRenderer_Stream16BitSamples, &regs, &regs);
		   }
  		   /* Check if we have enough buffers yet */
   		   _kernel_swi(DigitalRenderer_StreamStatistics, &regs, &regs);
	   }

   }
}

/* Size of DMA buffer to use */
#define DRENDERER_BUFFER_SIZE 512

/* Number of centiseconds of sound to buffer.
   Hopefully more than the maximum time between calls to the
   FillBuffers routine above
*/
#define DRENDERER_CSEC_TO_BUFFER 10

static int DRenderer_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	_kernel_swi_regs regs;
	int buffers_per_sample;

#ifdef DUMP_AUDIO
    printf("Request format %d\n", spec->format);
    printf("Request freq   %d\n", spec->freq);
    printf("Samples        %d\n", spec->samples);
#endif

	/* Only support signed 16bit format */
	spec->format = AUDIO_S16LSB;

    if (spec->samples < DRENDERER_BUFFER_SIZE) spec->samples = DRENDERER_BUFFER_SIZE;

    SDL_CalculateAudioSpec(spec);


	buffers_per_sample = spec->samples / DRENDERER_BUFFER_SIZE;

	if ((spec->samples % DRENDERER_BUFFER_SIZE) != 0)
	{
		buffers_per_sample++;
		spec->samples = buffers_per_sample * DRENDERER_BUFFER_SIZE;
	}

	/* Set number of buffers to use - the following should give enough
	   data between calls to the sound polling.
	*/

    if (riscos_audiobuffer == 0)
    {
    	FillBuffer = (int)((double)DRENDERER_CSEC_TO_BUFFER / ((double)DRENDERER_BUFFER_SIZE * 100.0 / (double)spec->freq)) + 1;
    } else FillBuffer = riscos_audiobuffer/DRENDERER_BUFFER_SIZE - buffers_per_sample;

	if (FillBuffer < buffers_per_sample) FillBuffer = buffers_per_sample;
	regs.r[0] = FillBuffer + buffers_per_sample;

#ifdef DUMP_AUDIO
    printf("Buffers per sample %d\n", buffers_per_sample);
    printf("Fill buffer        %d\n", FillBuffer);
    printf("Time buffered (ms) %d\n",(int)((1000.0 * regs.r[0] * DRENDERER_BUFFER_SIZE)/(double)spec->freq));
#endif

	if (_kernel_swi(DigitalRenderer_NumBuffers, &regs, &regs) != 0)
    {
       SDL_SetError("Can't set number of streaming sound buffers\n");
       return -1;
    }

	/* Now initialise sound system */
	regs.r[0] = spec->channels;   /* Number of channels */
	regs.r[1] = DRENDERER_BUFFER_SIZE; /* Samples length */
	regs.r[2] = spec->freq; /* frequency */
	regs.r[3] = 1;   /* Restore previous handler on exit */

	if (_kernel_swi(DigitalRenderer_Activate16, &regs, &regs) != 0)
	{
		SDL_SetError("Unable to activate digital renderer in 16 bit mode\n");
		return -1;
	}

	if (_kernel_swi(DigitalRenderer_GetFrequency, &regs, &regs) == 0)
	{
		spec->freq = regs.r[0];
	}

#ifdef DUMP_AUDIO
    printf("Got format %d\n", spec->format);
    printf("Frequency  %d\n", spec->freq);
    printf("Samples    %d\n", spec->samples);
#endif                 

    /* Set to fill buffer with zero if we don't get data to it fast enough */
    regs.r[0] = 1;
    regs.r[1] = ~1;
    _kernel_swi(DigitalRenderer_StreamFlags, &regs, &regs);

	buffer = (Uint8 *)malloc(sizeof(Uint8) * spec->size);
	if (buffer == NULL)
	{
		SDL_OutOfMemory();
		return -1;
	}

   /* Hopefully returning 2 will show success, but not start up an audio thread */
   return 2;
}

static void DRenderer_CloseAudio(_THIS)
{
	_kernel_swi_regs regs;

	/* Close down the digital renderer */
	_kernel_swi(DigitalRenderer_Deactivate, &regs, &regs);

	if (buffer != NULL) free(buffer);
}

