/*
 * MiNT audio driver
 * 
 * Patrice Mandin
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Mint includes */
#include <mint/osbind.h>
#include <mint/falcon.h>
#include <sys/cookie.h>

#include "SDL_endian.h"
#include "SDL_audio.h"
#include "SDL_audio_c.h"
#include "SDL_audiomem.h"
#include "SDL_sysaudio.h"

#include "SDL_mintaudio.h"
#include "SDL_mintaudiodma.h"
#include "SDL_mintaudiogsxb.h"
#include "SDL_mintaudiointerrupt_s.h"

#include "SDL_atarimxalloc_c.h"

/*--- Defines ---*/

#define MINT_AUDIO_DRIVER_NAME "mint"

/* Master clocks for replay frequencies */
#define MASTERCLOCK_STE		8010666		/* Not sure of this one */
#define MASTERCLOCK_TT		16107953	/* Not sure of this one */
#define MASTERCLOCK_FALCON1	25175000
#define MASTERCLOCK_FALCON2	32000000	/* Only usable for DSP56K */
#define MASTERCLOCK_FALCONEXT	-1		/* Clock on DSP56K port, unknown */
#define MASTERCLOCK_MILAN1	22579200	/* Standard clock for 44.1 Khz */
#define MASTERCLOCK_MILAN2	24576000	/* Standard clock for 48 Khz */

/* Master clock predivisors */
#define MASTERPREDIV_STE	160
#define MASTERPREDIV_TT		320
#define MASTERPREDIV_FALCON	256
#define MASTERPREDIV_MILAN	256

/* Values>>16 in _MCH cookie */
enum {
	MCH_ST=0,
	MCH_STE,
	MCH_TT,
	MCH_F30
};

/* MFP 68901 interrupt sources */
enum {
	MFP_PARALLEL=0,
	MFP_DCD,
	MFP_CTS,
	MFP_BITBLT,
	MFP_TIMERD,
	MFP_BAUDRATE=MFP_TIMERD,
	MFP_TIMERC,
	MFP_200HZ=MFP_TIMERC,
	MFP_ACIA,
	MFP_DISK,
	MFP_TIMERB,
	MFP_HBLANK=MFP_TIMERB,
	MFP_TERR,
	MFP_TBE,
	MFP_RERR,
	MFP_RBF,
	MFP_TIMERA,
	MFP_DMASOUND=MFP_TIMERA,
	MFP_RING,
	MFP_MONODETECT
};

/* Xbtimer() timers */
enum {
	XB_TIMERA=0,
	XB_TIMERB,
	XB_TIMERC,
	XB_TIMERD
};

/*--- Static variables ---*/

static unsigned long cookie_snd, cookie_mch, cookie_gsxb;
static Uint16 hardfreq[16];
static Uint16 numfreq;
static SDL_AudioDevice *SDL_MintAudio_device;

/*--- Audio driver functions ---*/

static void Mint_CloseAudio(_THIS);
static int Mint_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void Mint_LockAudio(_THIS);
static void Mint_UnlockAudio(_THIS);

/*--- Audio driver bootstrap functions ---*/

static int Audio_Available(void)
{
	const char *envr = getenv("SDL_AUDIODRIVER");

	/* Check if user asked a different audio driver */
	if ((envr) && (strcmp(envr, MINT_AUDIO_DRIVER_NAME)!=0)) {
		return 0;
	}

	/* Cookie _SND present ? if not, assume ST machine */
	if (Getcookie(C__SND, &cookie_snd) == C_NOTFOUND) {
		cookie_snd = SND_PSG;
	}

	/* Cookie _MCH present ? if not, assume ST machine */
	if (Getcookie(C__MCH, &cookie_mch) == C_NOTFOUND) {
		cookie_mch = MCH_ST << 16;
	}

	/* Cookie GSXB present ? */
	cookie_gsxb = (Getcookie(C_GSXB, &cookie_gsxb) == C_FOUND);

	/* Check if we have xbios functions (Falcon, clones) */
	if ((cookie_snd & SND_16BIT)!=0) {
		/* Check if audio is lockable */
		if (Locksnd()==1) {
			Unlocksnd();
		} else {
			/* Already in use */
			return(0);
		}

		return(1);
	}

	/* Check if we have 8 bits DMA audio (STE, TT) */
	if ((cookie_snd & SND_8BIT)!=0) {
		return(1);
	}

    return(0);
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
    }
    if ( this == NULL ) {
        SDL_OutOfMemory();
        if ( this ) {
            free(this);
        }
        return(0);
    }

    /* Set the function pointers */
    this->OpenAudio   = Mint_OpenAudio;
    this->CloseAudio  = Mint_CloseAudio;
    this->LockAudio   = Mint_LockAudio;
    this->UnlockAudio = Mint_UnlockAudio;
    this->free        = Audio_DeleteDevice;

    return this;
}

AudioBootStrap MINTAUDIO_bootstrap = {
	MINT_AUDIO_DRIVER_NAME, "MiNT audio driver",
	Audio_Available, Audio_CreateDevice
};

static void Mint_LockAudio(_THIS)
{
	void *oldpile;

	/* Stop replay */
	if ((cookie_snd & SND_16BIT)!=0) {
		Buffoper(0);
	} else if ((cookie_snd & SND_8BIT)!=0) {
		oldpile=(void *)Super(0);
		DMAAUDIO_IO.control=0;
		Super(oldpile);
	}
}

static void Mint_UnlockAudio(_THIS)
{
	void *oldpile;

	/* Restart replay */
	if ((cookie_snd & SND_16BIT)!=0) {
		Buffoper(SB_PLA_ENA|SB_PLA_RPT);
	} else if ((cookie_snd & SND_8BIT)!=0) {
		oldpile=(void *)Super(0);
		DMAAUDIO_IO.control=3;
		Super(oldpile);
	}
}

/* This is called from the interrupt routine */
void SDL_MintAudio_Callback(void)
{
	SDL_AudioDevice *audio;
	Uint8 *buffer;

	audio = SDL_MintAudio_device;
 	buffer = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];

	if ( ! audio->paused ) {
		if ( audio->convert.needed ) {
			audio->spec.callback(audio->spec.userdata,
				(Uint8 *)audio->convert.buf,audio->convert.len);
			SDL_ConvertAudio(&audio->convert);
			memcpy(buffer, audio->convert.buf, audio->convert.len_cvt);
		} else {
			audio->spec.callback(audio->spec.userdata, buffer, audio->spec.size);
		}
	}
}

static void Mint_StopAudio_Dma8(void)
{
	void *oldpile;
	
	oldpile=(void *)Super(0);
	DMAAUDIO_IO.control=0;
	Super(oldpile);

	Jdisint(MFP_DMASOUND);
}

static void Mint_StopAudio_Xbios(void)
{
	Buffoper(0);
	Jdisint(MFP_DMASOUND);
}

static void Mint_StopAudio_Gsxb(void)
{
	Buffoper(0);
}

static void Mint_CloseAudio(_THIS)
{
	if (cookie_gsxb && ((cookie_snd & (SND_GSXB|SND_16BIT))==(SND_GSXB|SND_16BIT)) ) {
		Mint_StopAudio_Gsxb();
	} else if ((cookie_snd & SND_16BIT)!=0) {
		Mint_StopAudio_Xbios();
	} else if ((cookie_snd & SND_8BIT)!=0) {
		Mint_StopAudio_Dma8();
	}

	/* Clear buffers */
	if (SDL_MintAudio_audiobuf[0]) {
		Mfree(SDL_MintAudio_audiobuf[0]);
		SDL_MintAudio_audiobuf[0] = SDL_MintAudio_audiobuf[1] = NULL;
	}

	/* Unlock sound system */
	if ((cookie_snd & SND_16BIT)!=0) {
		Unlocksnd();
	}
}

static void Mint_CheckAudio_Dma8(SDL_AudioSpec *spec)
{
	int i;

	spec->format = AUDIO_S8;

	switch(cookie_mch>>16) {
		case MCH_STE:
			/* STE replay frequencies */
			for (i=0;i<4;i++) {
				hardfreq[i]=MASTERCLOCK_STE/(MASTERPREDIV_STE*(i+1));
			}

			if (spec->freq>=(hardfreq[0]+hardfreq[1])>>1) {
				numfreq=3;		/* 50066 */
			} else if (spec->freq>=(hardfreq[1]+hardfreq[2])>>1) {
				numfreq=2;		/* 25033 */
			} else if (spec->freq>=(hardfreq[2]+hardfreq[3])>>1) {
				numfreq=1;		/* 12517 */
			} else {
				numfreq=0;		/* 6258 */
			}

			spec->freq=hardfreq[numfreq];
			break;
		case MCH_TT:
			/* TT replay frequencies */
			for (i=0;i<4;i++) {
				hardfreq[i]=MASTERCLOCK_TT/(MASTERPREDIV_TT*(i+1));
			}

			if (spec->freq>=(hardfreq[0]+hardfreq[1])>>1) {
				numfreq=3;		/* 50337 */
			} else if (spec->freq>=(hardfreq[1]+hardfreq[2])>>1) {
				numfreq=2;		/* 25169 */
			} else if (spec->freq>=(hardfreq[2]+hardfreq[3])>>1) {
				numfreq=1;		/* 12584 */
			} else {
				numfreq=0;		/* 6292 */
			}
			spec->freq=hardfreq[numfreq];
			break;
	}
}

static void Mint_CheckAudio_Xbios(SDL_AudioSpec *spec)
{
	int i;

	/* Check conversions needed */
	switch (spec->format & 0xff) {
		case 8:
			spec->format = AUDIO_S8;
			break;
		case 16:
			spec->format = AUDIO_S16MSB;
			break;
	}
	
	/* Check hardware channels */
	if ((spec->channels==1) && ((spec->format & 0xff)==16)) {
		spec->channels=2;
	}

	/* Falcon replay frequencies */
	for (i=0;i<16;i++) {
		hardfreq[i]=MASTERCLOCK_FALCON1/(MASTERPREDIV_FALCON*(i+1));
	}

	/* The Falcon CODEC only support some frequencies */
	if (spec->freq>=(hardfreq[CLK50K]+hardfreq[CLK33K])>>1) {
		numfreq=CLK50K;		/* 49170 */
	} else if (spec->freq>=(hardfreq[CLK33K]+hardfreq[CLK25K])>>1) {
		numfreq=CLK33K;		/* 32780 */
	} else if (spec->freq>=(hardfreq[CLK25K]+hardfreq[CLK20K])>>1) {
		numfreq=CLK25K;		/* 24585 */
	} else if (spec->freq>=(hardfreq[CLK20K]+hardfreq[CLK16K])>>1) {
		numfreq=CLK20K;		/* 19668 */
	} else if (spec->freq>=(hardfreq[CLK16K]+hardfreq[CLK12K])>>1) {
		numfreq=CLK16K;		/* 16390 */
	} else if (spec->freq>=(hardfreq[CLK12K]+hardfreq[CLK10K])>>1) {
		numfreq=CLK12K;		/* 12292 */
	} else if (spec->freq>=(hardfreq[CLK10K]+hardfreq[CLK8K])>>1) {
		numfreq=CLK10K;		/* 9834 */
	} else {
		numfreq=CLK8K;		/* 8195 */
	}				

	spec->freq=hardfreq[numfreq];
}

static int Mint_CheckAudio_Gsxb(SDL_AudioSpec *spec)
{
	long snd_format;
	int i, resolution, format_signed, format_bigendian;

	resolution = spec->format & 0x00ff;
	format_signed = ((spec->format & 0x8000)!=0);
	format_bigendian = ((spec->format & 0x1000)!=0);

	/* Check formats available */
	snd_format = Sndstatus(SND_QUERYFORMATS);
	switch (resolution) {
		case 8:
			if ((snd_format & SND_FORMAT8)==0) {
				SDL_SetError("Mint_CheckAudio: 8 bits samples not supported");
				return -1;
			}
			snd_format = Sndstatus(SND_QUERY8BIT);
			break;
		case 16:
			if ((snd_format & SND_FORMAT16)==0) {
				SDL_SetError("Mint_CheckAudio: 16 bits samples not supported");
				return -1;
			}
			snd_format = Sndstatus(SND_QUERY16BIT);
			break;
		default:
			SDL_SetError("Mint_CheckAudio: Unsupported sample resolution");
			return -1;
			break;
	}

	/* Check signed/unsigned format */
	if (format_signed) {
		if (snd_format & SND_FORMATSIGNED) {
			/* Ok */
		} else if (snd_format & SND_FORMATUNSIGNED) {
			/* Give unsigned format */
			spec->format = spec->format & (~0x8000);
		}
	} else {
		if (snd_format & SND_FORMATUNSIGNED) {
			/* Ok */
		} else if (snd_format & SND_FORMATSIGNED) {
			/* Give signed format */
			spec->format |= 0x8000;
		}
	}

	if (format_bigendian) {
		if (snd_format & SND_FORMATBIGENDIAN) {
			/* Ok */
		} else if (snd_format & SND_FORMATLITTLEENDIAN) {
			/* Give little endian format */
			spec->format = spec->format & (~0x1000);
		}
	} else {
		if (snd_format & SND_FORMATBIGENDIAN) {
			/* Ok */
		} else if (snd_format & SND_FORMATLITTLEENDIAN) {
			/* Give big endian format */
			spec->format |= 0x1000;
		}
	}
	
	/* Only xbios functions available = clone with PC board */
	for (i=0;i<8;i++) {
		hardfreq[i]=MASTERCLOCK_MILAN1/(MASTERPREDIV_MILAN*(i+1));
	}

	if (spec->freq>=(hardfreq[CLK_44K]+hardfreq[CLK_22K])>>1) {
		numfreq = CLK_44K;	/* 44100 */
	} else if (spec->freq>=(hardfreq[CLK_22K]+hardfreq[CLK_11K])>>1) {
		numfreq = CLK_22K;	/* 22050 */
	} else {
		numfreq = CLK_11K;	/* 11025 */
	}				

	spec->freq=hardfreq[numfreq];

	return 0;
}

static void Mint_InitAudio_Dma8(SDL_AudioSpec *spec)
{
	void *oldpile;
	unsigned long buffer;
	unsigned char mode;
	
	oldpile=(void *)Super(0);

	/* Stop currently playing sound */
	DMAAUDIO_IO.control=0;

	/* Set buffer */
	buffer = (unsigned long) SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
	DMAAUDIO_IO.start_high = (buffer>>16) & 255;
	DMAAUDIO_IO.start_mid = (buffer>>8) & 255;
	DMAAUDIO_IO.start_low = buffer & 255;

	buffer += SDL_MintAudio_audiosize;
	DMAAUDIO_IO.end_high = (buffer>>16) & 255;
	DMAAUDIO_IO.end_mid = (buffer>>8) & 255;
	DMAAUDIO_IO.end_low = buffer & 255;

	mode = numfreq;
	if (spec->channels==1) {
		mode |= 1<<7;
	}
	DMAAUDIO_IO.mode = mode;	

	/* Set interrupt */
	Jdisint(MFP_DMASOUND);
	Xbtimer(XB_TIMERA, 8, 1, SDL_MintAudio_IntDma);
	Jenabint(MFP_DMASOUND);

	/* Go */
	DMAAUDIO_IO.control = 3;	/* playback + repeat */

	Super(oldpile);
}

static void Mint_InitAudio_Xbios(SDL_AudioSpec *spec)
{
	int channels_mode;
	void *buffer;

	/* Stop currently playing sound */
	Buffoper(0);

	Settracks(0,0);
	Setmontracks(0);

	switch (spec->format & 0xff) {
		case 8:
			if (spec->channels==2) {
				channels_mode=STEREO8;
			} else {
				channels_mode=MONO8;
			}
			break;
		case 16:
		default:
			channels_mode=STEREO16;
			break;
	}
	Setmode(channels_mode);

	Devconnect(DMAPLAY, DAC, CLK25M, numfreq, 1);

	/* Set buffer */
	buffer = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
	Setbuffer(0, buffer, buffer+SDL_MintAudio_audiosize);
	
	/* Install interrupt */
	Setinterrupt(SI_TIMERA, SI_PLAY);

	Jdisint(MFP_DMASOUND);
	Xbtimer(XB_TIMERA, 8, 1, SDL_MintAudio_IntXbios);
	Jenabint(MFP_DMASOUND);

	/* Go */
	Buffoper(SB_PLA_ENA|SB_PLA_RPT);
}

static void Mint_InitAudio_Gsxb(SDL_AudioSpec *spec)
{
	int channels_mode;
	void *buffer;

	/* Stop currently playing sound */
	Buffoper(0);

	switch (spec->format & 0xff) {
		case 8:
			if (spec->channels==2) {
				channels_mode=STEREO8;
			} else {
				channels_mode=MONO8;
			}
			break;
		case 16:
			if (spec->channels==2) {
				channels_mode=STEREO16;
			} else {
				channels_mode=MONO16;
			}
			break;
		default:
			channels_mode=STEREO16;
			break;
	}
	Setmode(channels_mode);

	Devconnect(0, 0, CLKEXT, numfreq, 1);

	/* Set buffer */
	buffer = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
	Setbuffer(0, buffer, buffer+SDL_MintAudio_audiosize);
	
	/* Install interrupt */
	NSetinterrupt(2, SI_PLAY, SDL_MintAudio_IntGsxb);
		
	/* Go */
	Buffoper(SB_PLA_ENA|SB_PLA_RPT);
}

static int Mint_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	/* Lock sound system */
	if ((cookie_snd & SND_16BIT)!=0) {
		if (Locksnd()!=1) {
    	    SDL_SetError("Mint_OpenAudio: Audio system already in use");
	        return(-1);
		}
	}

	/* Check audio capabilities */
	if (cookie_gsxb && ((cookie_snd & (SND_GSXB|SND_16BIT))==(SND_GSXB|SND_16BIT)) ) {
		if (Mint_CheckAudio_Gsxb(spec)==-1) {
			return -1;
		}
	} else if ((cookie_snd & SND_16BIT)!=0) {
		Mint_CheckAudio_Xbios(spec);
	} else if ((cookie_snd & SND_8BIT)!=0) {
		Mint_CheckAudio_Dma8(spec);
	}

	SDL_CalculateAudioSpec(spec);

	/* Allocate memory for audio buffers in DMA-able RAM */
	spec->size = spec->samples;
	spec->size *= spec->channels;
	spec->size *= (spec->format & 0xFF)/8;

	SDL_MintAudio_audiosize = spec->size;

	SDL_MintAudio_audiobuf[0] = Atari_SysMalloc(SDL_MintAudio_audiosize *2, MX_STRAM);
	if (SDL_MintAudio_audiobuf[0]==NULL) {
		SDL_SetError("MINT_OpenAudio: Not enough memory for audio buffer");
		return (-1);
	}
	SDL_MintAudio_audiobuf[1] = SDL_MintAudio_audiobuf[0] + SDL_MintAudio_audiosize;
	SDL_MintAudio_numbuf=0;
	memset(SDL_MintAudio_audiobuf[0], 0, SDL_MintAudio_audiosize * 2);
	SDL_MintAudio_mutex = 0;

	SDL_MintAudio_device = this;

	/* Setup audio hardware */
	if (cookie_gsxb && ((cookie_snd & (SND_GSXB|SND_16BIT))==(SND_GSXB|SND_16BIT)) ) {
		Mint_InitAudio_Gsxb(spec);
	} else if ((cookie_snd & SND_16BIT)!=0) {
		Mint_InitAudio_Xbios(spec);
	} else if ((cookie_snd & SND_8BIT)!=0) {
		Mint_InitAudio_Dma8(spec);
	}

    return 1;
}
