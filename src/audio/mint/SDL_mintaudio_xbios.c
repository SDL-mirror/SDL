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

/*
	MiNT audio driver
	using XBIOS functions (Falcon)

	Patrice Mandin, Didier Méquignon
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Mint includes */
#include <mint/osbind.h>
#include <mint/falcon.h>
#include <mint/cookie.h>

#include "SDL_endian.h"
#include "SDL_audio.h"
#include "SDL_audio_c.h"
#include "SDL_audiomem.h"
#include "SDL_sysaudio.h"

#include "SDL_atarimxalloc_c.h"

#include "SDL_mintaudio.h"
#include "SDL_mintaudio_dma8.h"

/*--- Defines ---*/

#define MINT_AUDIO_DRIVER_NAME "mint_xbios"

/* Debug print info */
#define DEBUG_NAME "audio:xbios: "
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#else
#define DEBUG_PRINT(what)
#endif

/*--- Static variables ---*/

static unsigned long cookie_snd;

/*--- Audio driver functions ---*/

static void Mint_CloseAudio(_THIS);
static int Mint_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void Mint_LockAudio(_THIS);
static void Mint_UnlockAudio(_THIS);

/* To check/init hardware audio */
static int Mint_CheckAudio(_THIS, SDL_AudioSpec *spec);
static void Mint_InitAudio(_THIS, SDL_AudioSpec *spec);

/*--- Audio driver bootstrap functions ---*/

static int Audio_Available(void)
{
	const char *envr = getenv("SDL_AUDIODRIVER");

	/* Check if user asked a different audio driver */
	if ((envr) && (strcmp(envr, MINT_AUDIO_DRIVER_NAME)!=0)) {
		DEBUG_PRINT((DEBUG_NAME "user asked a different audio driver\n"));
		return(0);
	}

	/* Cookie _SND present ? if not, assume ST machine */
	if (Getcookie(C__SND, &cookie_snd) == C_NOTFOUND) {
		cookie_snd = SND_PSG;
	}

	/* Check if we have 16 bits audio */
	if ((cookie_snd & SND_16BIT)==0) {
		DEBUG_PRINT((DEBUG_NAME "no 16 bits sound\n"));
	    return(0);
	}

	/* Check if audio is lockable */
	if (Locksnd()!=1) {
		DEBUG_PRINT((DEBUG_NAME "audio locked by other application\n"));
		return(0);
	}

	Unlocksnd();

	DEBUG_PRINT((DEBUG_NAME "XBIOS audio available!\n"));
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
    this->OpenAudio   = Mint_OpenAudio;
    this->CloseAudio  = Mint_CloseAudio;
    this->LockAudio   = Mint_LockAudio;
    this->UnlockAudio = Mint_UnlockAudio;
    this->free        = Audio_DeleteDevice;

    return this;
}

AudioBootStrap MINTAUDIO_XBIOS_bootstrap = {
	MINT_AUDIO_DRIVER_NAME, "MiNT XBIOS audio driver",
	Audio_Available, Audio_CreateDevice
};

static void Mint_LockAudio(_THIS)
{
	/* Stop replay */
	Buffoper(0);
}

static void Mint_UnlockAudio(_THIS)
{
	/* Restart replay */
	Buffoper(SB_PLA_ENA|SB_PLA_RPT);
}

static void Mint_CloseAudio(_THIS)
{
	/* Stop replay */
	Buffoper(0);

	/* Uninstall interrupt */
	Jdisint(MFP_DMASOUND);

	/* Wait if currently playing sound */
	while (SDL_MintAudio_mutex != 0) {
	}

	/* Clear buffers */
	if (SDL_MintAudio_audiobuf[0]) {
		Mfree(SDL_MintAudio_audiobuf[0]);
		SDL_MintAudio_audiobuf[0] = SDL_MintAudio_audiobuf[1] = NULL;
	}

	/* Unlock sound system */
	Unlocksnd();
}

/* Falcon XBIOS implementation of Devconnect() is buggy with external clock */
static void Devconnect2(int src, int dst, int sclk, int pre)
{		
	static const unsigned short MASK1[3] = { 0, 0x6000, 0 };
	static const unsigned short MASK2[4] = { 0xFFF0, 0xFF8F, 0xF0FF, 0x0FFF };
	static const unsigned short INDEX1[4] = {  1, 3, 5, 7 };
	static const unsigned short INDEX2[4] = {  0, 2, 4, 6 };
	unsigned short sync_div,dev_ctrl,dest_ctrl;
	void *oldstack;

	if (dst==0) {
		return;
	}

	oldstack=(void *)Super(0);

	dev_ctrl = DMAAUDIO_IO.dev_ctrl;
	dest_ctrl = DMAAUDIO_IO.dest_ctrl;
	dev_ctrl &= MASK2[src];

	if (src==ADC) {
		dev_ctrl |= MASK1[sclk];
	} else {
		dev_ctrl |= (INDEX1[sclk] << (src<<4));
	}

	if (dst & DMAREC) {		
		dest_ctrl &= 0xFFF0;
		dest_ctrl |= INDEX1[src];
	}

	if (dst & DSPRECV) {		
		dest_ctrl &= 0xFF8F;
		dest_ctrl |= (INDEX1[src]<<4); 
	}

	if (dst & EXTOUT) {		
		dest_ctrl &= 0xF0FF;
		dest_ctrl |= (INDEX1[src]<<8); 
	}

	if (dst & DAC) {		
		dev_ctrl &= 0x0FFF;
		dev_ctrl |= MASK1[sclk]; 
		dest_ctrl &=  0x0FFF;
		dest_ctrl |= (INDEX2[src]<<12); 
	}

	sync_div = DMAAUDIO_IO.sync_div;
	if (sclk==CLKEXT) {
		pre<<=8;
		sync_div &= 0xF0FF;
	} else {
		sync_div &= 0xFFF0;
	}
	sync_div |= pre;

	DMAAUDIO_IO.dev_ctrl = dev_ctrl;
	DMAAUDIO_IO.dest_ctrl = dest_ctrl;
	DMAAUDIO_IO.sync_div = sync_div;

	Super(oldstack);
}

static Uint32 Mint_CheckExternalClock(void)
{
#define SIZE_BUF_CLOCK_MEASURE (44100/10)

	unsigned long cookie_snd;
	Uint32 masterclock;
	char *buffer;
	int i;

	/* DSP present with its GPIO port ? */
	if (Getcookie(C__SND, &cookie_snd) == C_NOTFOUND) {
		return 0;
	}
	if ((cookie_snd & SND_DSP)==0) {
		return 0;
	}

	buffer = Atari_SysMalloc(SIZE_BUF_CLOCK_MEASURE, MX_STRAM);
	if (buffer==NULL) {
		DEBUG_PRINT((DEBUG_NAME "Not enough memory for the measure\n"));
		return 0;
	}
	memset(buffer, 0, SIZE_BUF_CLOCK_MEASURE);

	masterclock=0;

	Buffoper(0);
	Settracks(0,0);
	Setmontracks(0);
	Setmode(MONO8);
	Jdisint(MFP_TIMERA);

	for (i=0; i<2; i++) {
		Gpio(GPIO_SET,7);      /* DSP port gpio outputs */
		Gpio(GPIO_WRITE,2+i);  /* 22.5792/24.576 MHz for 44.1/48KHz */
		Devconnect2(DMAPLAY, DAC, CLKEXT, CLK50K);  /* Matrix and clock source */
		Setbuffer(0, buffer, buffer + SIZE_BUF_CLOCK_MEASURE);		           /* Set buffer */
		Xbtimer(XB_TIMERA, 5, 38, SDL_MintAudio_XbiosInterruptMeasureClock); /* delay mode timer A, prediv /64, 1KHz */
		Jenabint(MFP_TIMERA);
		SDL_MintAudio_clocktics = 0;
		Buffoper(SB_PLA_ENA);
		usleep(110000);

		if((Buffoper(-1) & 1)==0) {
			if (SDL_MintAudio_clocktics) {
				unsigned long khz;

				khz = ((SIZE_BUF_CLOCK_MEASURE/SDL_MintAudio_clocktics) +1) & 0xFFFFFFFE;
				DEBUG_PRINT((DEBUG_NAME "measure %d: freq=%lu KHz\n", i+1, khz));

				if (i==0) {
					if(khz==44) {
						masterclock = MASTERCLOCK_44K;
					}
				} else {
					if(khz==48) {
						masterclock = MASTERCLOCK_48K;
					}
				}
			} else {
				DEBUG_PRINT((DEBUG_NAME "No measure\n"));
			}
		} else {
			DEBUG_PRINT((DEBUG_NAME "No SDMA clock\n"));
		}

		Buffoper(0);             /* stop */
		Jdisint(MFP_TIMERA);     /* Uninstall interrupt */
		if (masterclock == 0)
			break;
	}

	Mfree(buffer);
	return masterclock;
}

static int Mint_CheckAudio(_THIS, SDL_AudioSpec *spec)
{
	int i;
	Uint32 extclock;

	DEBUG_PRINT((DEBUG_NAME "asked: %d bits, ",spec->format & 0x00ff));
	DEBUG_PRINT(("signed=%d, ", ((spec->format & 0x8000)!=0)));
	DEBUG_PRINT(("big endian=%d, ", ((spec->format & 0x1000)!=0)));
	DEBUG_PRINT(("channels=%d, ", spec->channels));
	DEBUG_PRINT(("freq=%d\n", spec->freq));

	spec->format |= 0x8000;	/* Audio is always signed */
	if ((spec->format & 0x00ff)==16) {
		spec->format |= 0x1000;	/* Audio is always big endian */
		spec->channels=2;	/* 16 bits always stereo */
	}

	extclock=Mint_CheckExternalClock();

	/* Standard clocks */
	MINTAUDIO_freqcount=0;
	for (i=1;i<12;i++) {
		/* Remove unusable Falcon codec predivisors */
		if ((i==6) || (i==8) || (i==10)) {
			continue;
		}
		SDL_MintAudio_AddFrequency(this, MASTERCLOCK_FALCON1/(MASTERPREDIV_FALCON*(i+1)), MASTERCLOCK_FALCON1, i);
	}

	if (extclock>0) {
		for (i=1; i<4; i++) {
			SDL_MintAudio_AddFrequency(this, extclock/(MASTERPREDIV_FALCON*(1<<i)), extclock, (1<<i)-1);
		}
	}

#if 1
	for (i=0; i<MINTAUDIO_freqcount; i++) {
		DEBUG_PRINT((DEBUG_NAME "freq %d: %lu Hz, clock %lu, prediv %d\n",
			i, MINTAUDIO_frequencies[i].frequency, MINTAUDIO_frequencies[i].masterclock,
			MINTAUDIO_frequencies[i].predivisor
		));
	}
#endif

	MINTAUDIO_numfreq=SDL_MintAudio_SearchFrequency(this, spec->freq);
	spec->freq=MINTAUDIO_frequencies[MINTAUDIO_numfreq].frequency;

	DEBUG_PRINT((DEBUG_NAME "obtained: %d bits, ",spec->format & 0x00ff));
	DEBUG_PRINT(("signed=%d, ", ((spec->format & 0x8000)!=0)));
	DEBUG_PRINT(("big endian=%d, ", ((spec->format & 0x1000)!=0)));
	DEBUG_PRINT(("channels=%d, ", spec->channels));
	DEBUG_PRINT(("freq=%d\n", spec->freq));

	return 0;
}

static void Mint_InitAudio(_THIS, SDL_AudioSpec *spec)
{
	int channels_mode, dmaclock, prediv;
	void *buffer;

	/* Stop currently playing sound */
	Buffoper(0);

	/* Set replay tracks */
	Settracks(0,0);
	Setmontracks(0);

	/* Select replay format */
	channels_mode=STEREO16;
	switch (spec->format & 0xff) {
		case 8:
			if (spec->channels==2) {
				channels_mode=STEREO8;
			} else {
				channels_mode=MONO8;
			}
			break;
	}
	if (Setmode(channels_mode)<0) {
		DEBUG_PRINT((DEBUG_NAME "Setmode() failed\n"));
	}

	dmaclock = MINTAUDIO_frequencies[MINTAUDIO_numfreq].masterclock;
	prediv = MINTAUDIO_frequencies[MINTAUDIO_numfreq].predivisor;
	if (dmaclock != MASTERCLOCK_FALCON1) {
		Gpio(GPIO_SET,7);		/* DSP port gpio outputs */
		if (dmaclock == MASTERCLOCK_44K) {
			Gpio(GPIO_WRITE,2);	/* 22.5792 MHz for 44.1KHz */
		} else {
			Gpio(GPIO_WRITE,3);	/* 24.576 MHz for 48KHz */
		}
		Devconnect2(DMAPLAY, DAC, CLKEXT, prediv);
	} else {
		Devconnect(DMAPLAY, DAC, CLK25M, prediv, 1);
	}

	/* Set buffer */
	buffer = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
	if (Setbuffer(0, buffer, buffer + spec->size)<0) {
		DEBUG_PRINT((DEBUG_NAME "Setbuffer() failed\n"));
	}
	
	/* Install interrupt */
	Jdisint(MFP_DMASOUND);
	Xbtimer(XB_TIMERA, 8, 1, SDL_MintAudio_XbiosInterrupt);
	Jenabint(MFP_DMASOUND);

	if (Setinterrupt(SI_TIMERA, SI_PLAY)<0) {
		DEBUG_PRINT((DEBUG_NAME "Setinterrupt() failed\n"));
	}

	/* Go */
	Buffoper(SB_PLA_ENA|SB_PLA_RPT);
	DEBUG_PRINT((DEBUG_NAME "hardware initialized\n"));
}

static int Mint_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	/* Lock sound system */
	if (Locksnd()!=1) {
   	    SDL_SetError("Mint_OpenAudio: Audio system already in use");
        return(-1);
	}

	SDL_MintAudio_device = this;

	/* Check audio capabilities */
	if (Mint_CheckAudio(this, spec)==-1) {
		return -1;
	}

	SDL_CalculateAudioSpec(spec);

	/* Allocate memory for audio buffers in DMA-able RAM */
	DEBUG_PRINT((DEBUG_NAME "buffer size=%d\n", spec->size));

	SDL_MintAudio_audiobuf[0] = Atari_SysMalloc(spec->size *2, MX_STRAM);
	if (SDL_MintAudio_audiobuf[0]==NULL) {
		SDL_SetError("MINT_OpenAudio: Not enough memory for audio buffer");
		return (-1);
	}
	SDL_MintAudio_audiobuf[1] = SDL_MintAudio_audiobuf[0] + spec->size ;
	SDL_MintAudio_numbuf=0;
	memset(SDL_MintAudio_audiobuf[0], spec->silence, spec->size *2);
	SDL_MintAudio_audiosize = spec->size;
	SDL_MintAudio_mutex = 0;

	DEBUG_PRINT((DEBUG_NAME "buffer 0 at 0x%08x\n", SDL_MintAudio_audiobuf[0]));
	DEBUG_PRINT((DEBUG_NAME "buffer 1 at 0x%08x\n", SDL_MintAudio_audiobuf[1]));

	/* Setup audio hardware */
	Mint_InitAudio(this, spec);

    return(1);	/* We don't use threaded audio */
}
