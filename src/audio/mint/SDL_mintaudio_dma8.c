/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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
	MiNT audio driver
	using DMA 8bits (hardware access)

	Patrice Mandin
*/

/* Mint includes */
#include <mint/osbind.h>
#include <mint/falcon.h>
#include <mint/cookie.h>

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"

#include "../../video/ataricommon/SDL_atarimxalloc_c.h"

#include "SDL_mintaudio.h"
#include "SDL_mintaudio_dma8.h"

/*--- Defines ---*/

#define MINT_AUDIO_DRIVER_NAME "mint_dma8"

/* Debug print info */
#define DEBUG_NAME "audio:dma8: "
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#else
#define DEBUG_PRINT(what)
#endif

/*--- Static variables ---*/

static unsigned long cookie_snd, cookie_mch;

static void
MINTDMA8_LockDevice(_THIS)
{
    void *oldpile;

    /* Stop replay */
    oldpile = (void *) Super(0);
    DMAAUDIO_IO.control = 0;
    Super(oldpile);
}

static void
MINTDMA8_UnlockDevice(_THIS)
{
    void *oldpile;

    /* Restart replay */
    oldpile = (void *) Super(0);
    DMAAUDIO_IO.control = 3;
    Super(oldpile);
}

static void
MINTDMA8_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        /* Stop replay */
        void *oldpile = (void *) Super(0);

        DMAAUDIO_IO.control = 0;
        Super(oldpile);

        DEBUG_PRINT((DEBUG_NAME "closeaudio: replay stopped\n"));

        /* Disable interrupt */
        Jdisint(MFP_DMASOUND);

        DEBUG_PRINT((DEBUG_NAME "closeaudio: interrupt disabled\n"));

        /* Wait if currently playing sound */
        while (SDL_MintAudio_mutex != 0) {
        }

        DEBUG_PRINT((DEBUG_NAME "closeaudio: no more interrupt running\n"));

        /* Clear buffers */
        if (SDL_MintAudio_audiobuf[0]) {
            Mfree(SDL_MintAudio_audiobuf[0]);
            SDL_MintAudio_audiobuf[0] = SDL_MintAudio_audiobuf[1] = NULL;
        }

        DEBUG_PRINT((DEBUG_NAME "closeaudio: buffers freed\n"));
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
MINTDMA8_CheckAudio(_THIS)
{
    int i, masterprediv, sfreq;
    unsigned long masterclock;

    DEBUG_PRINT((DEBUG_NAME "asked: %d bits, ",
                 SDL_AUDIO_BITSIZE(this->spec.format)));
    DEBUG_PRINT(("float=%d, ", SDL_AUDIO_ISFLOAT(this->spec.format)));
    DEBUG_PRINT(("signed=%d, ", SDL_AUDIO_ISSIGNED(this->spec.format)));
    DEBUG_PRINT(("big endian=%d, ",
                 SDL_AUDIO_ISBIGENDIAN(this->spec.format)));
    DEBUG_PRINT(("channels=%d, ", this->spec.channels));
    DEBUG_PRINT(("freq=%d\n", this->spec.freq));

    if (this->spec.channels > 2) {
        this->spec.channels = 2;        /* no more than stereo! */
    }

    /* Check formats available */
    this->spec.format = AUDIO_S8;

    /* Calculate and select the closest frequency */
    sfreq = 0;
    masterclock = MASTERCLOCK_STE;
    masterprediv = MASTERPREDIV_STE;
    switch (cookie_mch >> 16) {
/*
		case MCH_STE:
			masterclock=MASTERCLOCK_STE;
			masterprediv=MASTERPREDIV_STE;
			break;
*/
    case MCH_TT:
        masterclock = MASTERCLOCK_TT;
        masterprediv = MASTERPREDIV_TT;
        break;
    case MCH_F30:
    case MCH_ARANYM:
        masterclock = MASTERCLOCK_FALCON1;
        masterprediv = MASTERPREDIV_FALCON;
        sfreq = 1;
        break;
    }

    MINTAUDIO_freqcount = 0;
    for (i = sfreq; i < 4; i++) {
        SDL_MintAudio_AddFrequency(this,
                                   masterclock / (masterprediv * (1 << i)),
                                   masterclock, i - sfreq, -1);
    }

#if 1
    for (i = 0; i < MINTAUDIO_freqcount; i++) {
        DEBUG_PRINT((DEBUG_NAME "freq %d: %lu Hz, clock %lu, prediv %d\n",
                     i, MINTAUDIO_frequencies[i].frequency,
                     MINTAUDIO_frequencies[i].masterclock,
                     MINTAUDIO_frequencies[i].predivisor));
    }
#endif

    MINTAUDIO_numfreq = SDL_MintAudio_SearchFrequency(this, this->spec.freq);
    this->spec.freq = MINTAUDIO_frequencies[MINTAUDIO_numfreq].frequency;

    DEBUG_PRINT((DEBUG_NAME "obtained: %d bits, ",
                 SDL_AUDIO_BITSIZE(this->spec.format)));
    DEBUG_PRINT(("float=%d, ", SDL_AUDIO_ISFLOAT(this->spec.format)));
    DEBUG_PRINT(("signed=%d, ", SDL_AUDIO_ISSIGNED(this->spec.format)));
    DEBUG_PRINT(("big endian=%d, ",
                 SDL_AUDIO_ISBIGENDIAN(this->spec.format)));
    DEBUG_PRINT(("channels=%d, ", this->spec.channels));
    DEBUG_PRINT(("freq=%d\n", this->spec.freq));

    return 0;
}

static void
MINTDMA8_InitAudio(_THIS)
{
    void *oldpile;
    unsigned long buffer;
    unsigned char mode;

    /* Set replay tracks */
    if (cookie_snd & SND_16BIT) {
        Settracks(0, 0);
        Setmontracks(0);
    }

    oldpile = (void *) Super(0);

    /* Stop currently playing sound */
    DMAAUDIO_IO.control = 0;

    /* Set buffer */
    buffer = (unsigned long) SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
    DMAAUDIO_IO.start_high = (buffer >> 16) & 255;
    DMAAUDIO_IO.start_mid = (buffer >> 8) & 255;
    DMAAUDIO_IO.start_low = buffer & 255;

    buffer += SDL_MintAudio_audiosize;
    DMAAUDIO_IO.end_high = (buffer >> 16) & 255;
    DMAAUDIO_IO.end_mid = (buffer >> 8) & 255;
    DMAAUDIO_IO.end_low = buffer & 255;

    mode = 3 - MINTAUDIO_frequencies[MINTAUDIO_numfreq].predivisor;
    if (this->spec.channels == 1) {
        mode |= 1 << 7;
    }
    DMAAUDIO_IO.sound_ctrl = mode;

    /* Set interrupt */
    Jdisint(MFP_DMASOUND);
    Xbtimer(XB_TIMERA, 8, 1, SDL_MintAudio_Dma8Interrupt);
    Jenabint(MFP_DMASOUND);

    if (cookie_snd & SND_16BIT) {
        if (Setinterrupt(SI_TIMERA, SI_PLAY) < 0) {
            DEBUG_PRINT((DEBUG_NAME "Setinterrupt() failed\n"));
        }
    }

    /* Go */
    DMAAUDIO_IO.control = 3;    /* playback + repeat */

    Super(oldpile);
}

static int
MINTDMA8_OpenDevice(_THIS, const char *devname, int iscapture)
{
    SDL_MintAudio_device = this;

    /* Check audio capabilities */
    if (MINTDMA8_CheckAudio(this) == -1) {
        return 0;
    }

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    SDL_CalculateAudioSpec(&this->spec);

    /* Allocate memory for audio buffers in DMA-able RAM */
    DEBUG_PRINT((DEBUG_NAME "buffer size=%d\n", this->spec.size));

    SDL_MintAudio_audiobuf[0] =
        Atari_SysMalloc(this->spec.size * 2, MX_STRAM);
    if (SDL_MintAudio_audiobuf[0] == NULL) {
        SDL_free(this->hidden);
        this->hidden = NULL;
        SDL_OutOfMemory();
        return 0;
    }
    SDL_MintAudio_audiobuf[1] = SDL_MintAudio_audiobuf[0] + this->spec.size;
    SDL_MintAudio_numbuf = 0;
    SDL_memset(SDL_MintAudio_audiobuf[0], this->spec.silence,
               this->spec.size * 2);
    SDL_MintAudio_audiosize = this->spec.size;
    SDL_MintAudio_mutex = 0;

    DEBUG_PRINT((DEBUG_NAME "buffer 0 at 0x%08x\n",
                 SDL_MintAudio_audiobuf[0]));
    DEBUG_PRINT((DEBUG_NAME "buffer 1 at 0x%08x\n",
                 SDL_MintAudio_audiobuf[1]));

    SDL_MintAudio_CheckFpu();

    /* Setup audio hardware */
    MINTDMA8_InitAudio(this);

    return 1;                   /* good to go. */
}

static int
MINTDMA8_Init(SDL_AudioDriverImpl * impl)
{
    /* Cookie _MCH present ? if not, assume ST machine */
    if (Getcookie(C__MCH, &cookie_mch) == C_NOTFOUND) {
        cookie_mch = MCH_ST;
    }

    /* Cookie _SND present ? if not, assume ST machine */
    if (Getcookie(C__SND, &cookie_snd) == C_NOTFOUND) {
        cookie_snd = SND_PSG;
    }

    /* Check if we have 8 bits audio */
    if ((cookie_snd & SND_8BIT) == 0) {
        SDL_SetError(DEBUG_NAME "no 8 bits sound");
        return 0;
    }

    /* Check if audio is lockable */
    if (cookie_snd & SND_16BIT) {
        if (Locksnd() != 1) {
            SDL_SetError(DEBUG_NAME "audio locked by other application");
            return 0;
        }

        Unlocksnd();
    }

    DEBUG_PRINT((DEBUG_NAME "8 bits audio available!\n"));

    /* Set the function pointers */
    impl->OpenDevice = MINTDMA8_OpenDevice;
    impl->CloseDevice = MINTDMA8_CloseDevice;
    impl->LockDevice = MINTDMA8_LockDevice;
    impl->UnlockDevice = MINTDMA8_UnlockDevice;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->ProvidesOwnCallbackThread = 1;
    impl->SkipMixerLock = 1;

    return 2;                   /* 2 == definitely has an audio device. */
}

AudioBootStrap MINTAUDIO_DMA8_bootstrap = {
    MINT_AUDIO_DRIVER_NAME, "MiNT DMA 8 bits audio driver", MINTDMA8_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
