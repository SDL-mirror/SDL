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

	Patrice Mandin
*/

#ifndef _SDL_mintaudio_h
#define _SDL_mintaudio_h

#include "SDL_sysaudio.h"
#include "SDL_mintaudio_stfa.h"

/* Hidden "this" pointer for the audio functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData {
	Uint32	hardfreq[16];	/* Array of replay freqs of the hardware */
	int		sfreq;			/* First number of freq to use in the array */
	int 	nfreq;			/* Number of freqs to use in the array */
	int		numfreq;		/* Number of selected frequency */
};

/* Old variable names */
#define MINTAUDIO_hardfreq		(this->hidden->hardfreq)
#define MINTAUDIO_sfreq			(this->hidden->sfreq)
#define MINTAUDIO_nfreq			(this->hidden->nfreq)
#define MINTAUDIO_numfreq		(this->hidden->numfreq)

/* _MCH cookie (values>>16) */
enum {
	MCH_ST=0,
	MCH_STE,
	MCH_TT,
	MCH_F30
};

/* Master clocks for replay frequencies */
#define MASTERCLOCK_STE		8010666		/* Not sure of this one */
#define MASTERCLOCK_TT		16107953	/* Not sure of this one */
#define MASTERCLOCK_FALCON1	25175000
#define MASTERCLOCK_FALCON2	32000000	/* Only usable for DSP56K */
#define MASTERCLOCK_FALCONEXT	-1		/* Clock on DSP56K port, unknown */
#define MASTERCLOCK_44K		22579200	/* Standard clock for 44.1 Khz */
#define MASTERCLOCK_48K		24576000	/* Standard clock for 48 Khz */

/* Master clock predivisors */
#define MASTERPREDIV_STE	160
#define MASTERPREDIV_TT		320
#define MASTERPREDIV_FALCON	256
#define MASTERPREDIV_MILAN	256

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

/* Variables */
extern SDL_AudioDevice *SDL_MintAudio_device;
extern Uint8 *SDL_MintAudio_audiobuf[2];	/* Pointers to buffers */
extern unsigned long SDL_MintAudio_audiosize;		/* Length of audio buffer=spec->size */
extern unsigned short SDL_MintAudio_numbuf;		/* Buffer to play */
extern unsigned short SDL_MintAudio_mutex;
extern cookie_stfa_t *SDL_MintAudio_stfa;

/* Functions */
void SDL_MintAudio_Callback(void);
int SDL_MintAudio_SearchFrequency(_THIS, int falcon_codec, int desired_freq);

/* ASM interrupt functions */
void SDL_MintAudio_GsxbInterrupt(void);
void SDL_MintAudio_EmptyGsxbInterrupt(void);
void SDL_MintAudio_XbiosInterrupt(void);
void SDL_MintAudio_Dma8Interrupt(void);
void SDL_MintAudio_StfaInterrupt(void);

#endif /* _SDL_mintaudio_h */
