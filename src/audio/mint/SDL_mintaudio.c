/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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
#include "SDL_config.h"

/*
	Audio interrupt variables and callback function

	Patrice Mandin
*/

#include <unistd.h>

#include <mint/osbind.h>
#include <mint/falcon.h>
#include <mint/mintbind.h>
#include <mint/cookie.h>

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"

#include "../../video/ataricommon/SDL_atarimxalloc_c.h"

#include "SDL_mintaudio.h"

/* The audio device */

#define MAX_DMA_BUF	8

SDL_AudioDevice *SDL_MintAudio_device;

static int SDL_MintAudio_num_upd;	/* Number of calls to update function */
static int SDL_MintAudio_max_buf;	/* Number of buffers to use */
static int SDL_MintAudio_numbuf;	/* Buffer to play */

static void SDL_MintAudio_Callback(void);

/* MiNT thread variables */
SDL_bool SDL_MintAudio_mint_present;
SDL_bool SDL_MintAudio_quit_thread;
SDL_bool SDL_MintAudio_thread_finished;
long SDL_MintAudio_thread_pid;

/* Debug print info */
#define DEBUG_NAME "audio:mint: "
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#else
#define DEBUG_PRINT(what)
#endif

/* Initialize DMA buffers */

int SDL_MintAudio_InitBuffers(SDL_AudioSpec *spec)
{
	int dmabuflen;
	SDL_AudioDevice *this = SDL_MintAudio_device;

	SDL_CalculateAudioSpec(spec);
	MINTAUDIO_audiosize = spec->size * MAX_DMA_BUF;

	/* Allocate audio buffer memory for application in FastRAM */
	MINTAUDIO_fastrambuf = Atari_SysMalloc(MINTAUDIO_audiosize, MX_TTRAM);
	if (MINTAUDIO_fastrambuf) {
		SDL_memset(MINTAUDIO_fastrambuf, spec->silence, MINTAUDIO_audiosize);
	}

	/* Allocate audio buffers memory for hardware in DMA-able RAM */
	dmabuflen = ((2 * MINTAUDIO_audiosize) | 3)+1;
	MINTAUDIO_audiobuf[0] = Atari_SysMalloc(dmabuflen, MX_STRAM);
	if (MINTAUDIO_audiobuf[0]==NULL) {
		SDL_SetError("SDL_MintAudio_OpenAudio: Not enough memory for audio buffer");
		return (0);
	}
	MINTAUDIO_audiobuf[1] = MINTAUDIO_audiobuf[0] + MINTAUDIO_audiosize;
	SDL_memset(MINTAUDIO_audiobuf[0], spec->silence, dmabuflen);

	DEBUG_PRINT((DEBUG_NAME "buffer 0 at 0x%p\n", MINTAUDIO_audiobuf[0]));
	DEBUG_PRINT((DEBUG_NAME "buffer 1 at 0x%p\n", MINTAUDIO_audiobuf[1]));

	SDL_MintAudio_numbuf = SDL_MintAudio_num_its = SDL_MintAudio_num_upd = 0;
	SDL_MintAudio_max_buf = MAX_DMA_BUF;

	/* For filling silence when too many interrupts per update */
	SDL_MintAudio_itbuffer = MINTAUDIO_audiobuf[0];
	SDL_MintAudio_itbuflen = (dmabuflen >> 2)-1;
	SDL_MintAudio_itsilence = (spec->silence << 24)|(spec->silence << 16)|
		(spec->silence << 8)|spec->silence;

	return (1);
}

/* Destroy DMA buffers */

void SDL_MintAudio_FreeBuffers(void)
{
	SDL_AudioDevice *this = SDL_MintAudio_device;

	if (MINTAUDIO_fastrambuf) {
		Mfree(MINTAUDIO_fastrambuf);
		MINTAUDIO_fastrambuf = NULL;
	}
	if (MINTAUDIO_audiobuf[0]) {
		Mfree(MINTAUDIO_audiobuf[0]);
		SDL_MintAudio_itbuffer = MINTAUDIO_audiobuf[0] = MINTAUDIO_audiobuf[1] = NULL;
	}
}

/* Update buffers */

void SDL_AtariMint_UpdateAudio(void)
{
	SDL_AudioDevice *this = SDL_MintAudio_device;

	++SDL_MintAudio_num_upd;

	/* No interrupt triggered? still playing current buffer */
	if (SDL_MintAudio_num_its==0) {
		return;
	}

	if (SDL_MintAudio_num_upd < (SDL_MintAudio_num_its<<2)) {
		/* Too many interrupts per update, increase latency */
		if (SDL_MintAudio_max_buf < MAX_DMA_BUF) {
			SDL_MintAudio_max_buf <<= 1;
		}
	} else if (SDL_MintAudio_num_its < (SDL_MintAudio_num_upd<<2)) {
		/* Too many updates per interrupt, decrease latency */
		if (SDL_MintAudio_max_buf > 1) {
			SDL_MintAudio_max_buf >>= 1;
		}
	}
	MINTAUDIO_audiosize = this->spec.size * SDL_MintAudio_max_buf;

	SDL_MintAudio_num_its = 0;
	SDL_MintAudio_num_upd = 0;

	SDL_MintAudio_numbuf ^= 1;

	/* Fill new buffer */
	SDL_MintAudio_Callback();

	/* And swap to it */
	(*MINTAUDIO_swapbuf)(MINTAUDIO_audiobuf[SDL_MintAudio_numbuf], MINTAUDIO_audiosize);
}

/* The callback function, called by each driver whenever needed */

static void SDL_MintAudio_Callback(void)
{
	SDL_AudioDevice *this = SDL_MintAudio_device;
	Uint8 *buffer;
	int i;

 	buffer = (MINTAUDIO_fastrambuf ?
		MINTAUDIO_fastrambuf :
		MINTAUDIO_audiobuf[SDL_MintAudio_numbuf]);
	SDL_memset(buffer, this->spec.silence, this->spec.size * SDL_MintAudio_max_buf);

	if (!this->paused) {
		for (i=0; i<SDL_MintAudio_max_buf; i++) {
			if (this->convert.needed) {
				int silence;

				if ( this->convert.src_format == AUDIO_U8 ) {
					silence = 0x80;
				} else {
					silence = 0;
				}
				SDL_memset(this->convert.buf, silence, this->convert.len);
				this->spec.callback(this->spec.userdata,
					(Uint8 *)this->convert.buf,this->convert.len);
				SDL_ConvertAudio(&this->convert);
				SDL_memcpy(buffer, this->convert.buf, this->convert.len_cvt);

				buffer += this->convert.len_cvt;
			} else {
				this->spec.callback(this->spec.userdata, buffer,
					this->spec.size);

				buffer += this->spec.size;
			}
		}
	}

	if (MINTAUDIO_fastrambuf) {
		SDL_memcpy(MINTAUDIO_audiobuf[SDL_MintAudio_numbuf], MINTAUDIO_fastrambuf,
			this->spec.size * SDL_MintAudio_max_buf);
	}
}

/* Add a new frequency/clock/predivisor to the current list */
void SDL_MintAudio_AddFrequency(_THIS, Uint32 frequency, Uint32 clock,
	Uint32 prediv, int gpio_bits)
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
			SDL_memcpy(&MINTAUDIO_frequencies[i], &MINTAUDIO_frequencies[i-1], sizeof(mint_frequency_t));
		}
	}

	/* And insert new one */
	MINTAUDIO_frequencies[p].frequency = frequency;
	MINTAUDIO_frequencies[p].masterclock = clock;
	MINTAUDIO_frequencies[p].predivisor = prediv;
	MINTAUDIO_frequencies[p].gpio_bits = gpio_bits;

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

/* The thread function, used under MiNT with xbios */
int SDL_MintAudio_Thread(long param)
{
	SndBufPtr	pointers;
	SDL_bool	buffers_filled[2] = {SDL_FALSE, SDL_FALSE};
	SDL_AudioDevice *this = SDL_MintAudio_device;

	SDL_MintAudio_thread_finished = SDL_FALSE;
	while (!SDL_MintAudio_quit_thread) {
		if (Buffptr(&pointers)!=0)
			continue;

		if (( (unsigned long)pointers.play>=(unsigned long)MINTAUDIO_audiobuf[0])
			&& ( (unsigned long)pointers.play<=(unsigned long)MINTAUDIO_audiobuf[1])) 
		{
			/* DMA is reading buffer #0, setup buffer #1 if not already done */
			if (!buffers_filled[1]) {
				SDL_MintAudio_numbuf = 1;
				SDL_MintAudio_Callback();
				Setbuffer(0, MINTAUDIO_audiobuf[1], MINTAUDIO_audiobuf[1] + MINTAUDIO_audiosize);
				buffers_filled[1]=SDL_TRUE;
				buffers_filled[0]=SDL_FALSE;
			}
		} else {
			/* DMA is reading buffer #1, setup buffer #0 if not already done */
			if (!buffers_filled[0]) {
				SDL_MintAudio_numbuf = 0;
				SDL_MintAudio_Callback();
				Setbuffer(0, MINTAUDIO_audiobuf[0], MINTAUDIO_audiobuf[0] + MINTAUDIO_audiosize);
				buffers_filled[0]=SDL_TRUE;
				buffers_filled[1]=SDL_FALSE;
			}
		}

		usleep(100);
	}
	SDL_MintAudio_thread_finished = SDL_TRUE;
	return 0;
}

void SDL_MintAudio_WaitThread(void)
{
	if (!SDL_MintAudio_mint_present)
		return;

	if (SDL_MintAudio_thread_finished)
		return;

	SDL_MintAudio_quit_thread = SDL_TRUE;
	while (!SDL_MintAudio_thread_finished) {
		Syield();
	}
}
