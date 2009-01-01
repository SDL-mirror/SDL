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
	using XBIOS functions (MacSound compatible driver)

	Patrice Mandin
*/

#include <support.h>

/* Mint includes */
#include <mint/osbind.h>
#include <mint/falcon.h>
#include <mint/cookie.h>

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"

#include "../../video/ataricommon/SDL_atarimxalloc_c.h"

#include "SDL_mintaudio.h"
#include "SDL_mintaudio_mcsn.h"

/*--- Defines ---*/

#define MINT_AUDIO_DRIVER_NAME "mint_mcsn"

/* Debug print info */
#define DEBUG_NAME "audio:mcsn: "
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#else
#define DEBUG_PRINT(what)
#endif

/*--- Static variables ---*/

static unsigned long cookie_snd = 0;
static unsigned long cookie_mch = 0;
static cookie_mcsn_t *cookie_mcsn = NULL;

static void
MINTMCSN_LockDevice(_THIS)
{
    /* Stop replay */
    Buffoper(0);
}

static void
MINTMCSN_UnlockDevice(_THIS)
{
    /* Restart replay */
    Buffoper(SB_PLA_ENA | SB_PLA_RPT);
}

static void
MINTMCSN_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        /* Stop replay */
        SDL_MintAudio_WaitThread();
        Buffoper(0);

        if (!SDL_MintAudio_mint_present) {
            /* Uninstall interrupt */
            Jdisint(MFP_DMASOUND);
        }

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

        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
MINTMCSN_CheckAudio(_THIS)
{
    int i;
    unsigned long masterclock, masterprediv;

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
    MINTAUDIO_freqcount = 0;
    switch (cookie_mcsn->play) {
    case MCSN_ST:
        this->spec.channels = 1;
        this->spec.format = AUDIO_S8;   /* FIXME: is it signed or unsigned ? */
        SDL_MintAudio_AddFrequency(this, 12500, 0, 0, -1);
        break;
    case MCSN_TT:              /* Also STE, Mega STE */
        this->spec.format = AUDIO_S8;
        masterclock = MASTERCLOCK_STE;
        masterprediv = MASTERPREDIV_STE;
        if ((cookie_mch >> 16) == MCH_TT) {
            masterclock = MASTERCLOCK_TT;
            masterprediv = MASTERPREDIV_TT;
        }
        for (i = 0; i < 4; i++) {
            SDL_MintAudio_AddFrequency(this,
                                       masterclock / (masterprediv *
                                                      (1 << i)),
                                       masterclock, 3 - i, -1);
        }
        break;
    case MCSN_FALCON:          /* Also Mac */
        for (i = 1; i < 12; i++) {
            /* Remove unusable Falcon codec predivisors */
            if ((i == 6) || (i == 8) || (i == 10)) {
                continue;
            }
            SDL_MintAudio_AddFrequency(this,
                                       MASTERCLOCK_FALCON1 /
                                       (MASTERPREDIV_FALCON * (i + 1)),
                                       CLK25M, i + 1, -1);
        }
        if (cookie_mcsn->res1 != 0) {
            for (i = 1; i < 4; i++) {
                SDL_MintAudio_AddFrequency(this,
                                           (cookie_mcsn->res1) /
                                           (MASTERPREDIV_FALCON *
                                            (1 << i)), CLKEXT,
                                           (1 << i) - 1, -1);
            }
        }
        this->spec.format |= SDL_AUDIO_MASK_SIGNED;     /* Audio is always signed */
        if ((SDL_AUDIO_BITSIZE(this->spec.format)) == 16) {
            this->spec.format |= SDL_AUDIO_MASK_ENDIAN; /* Audio is always big endian */
            this->spec.channels = 2;    /* 16 bits always stereo */
        }
        break;
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
MINTMCSN_InitAudio(_THIS)
{
    int channels_mode, prediv, dmaclock;
    void *buffer;

    /* Stop currently playing sound */
    SDL_MintAudio_quit_thread = SDL_FALSE;
    SDL_MintAudio_thread_finished = SDL_TRUE;
    SDL_MintAudio_WaitThread();
    Buffoper(0);

    /* Set replay tracks */
    Settracks(0, 0);
    Setmontracks(0);

    /* Select replay format */
    channels_mode = STEREO16;
    switch (SDL_AUDIO_BITSIZE(this->spec.format)) {
    case 8:
        if (this->spec.channels == 2) {
            channels_mode = STEREO8;
        } else {
            channels_mode = MONO8;
        }
        break;
    }
    if (Setmode(channels_mode) < 0) {
        DEBUG_PRINT((DEBUG_NAME "Setmode() failed\n"));
    }

    dmaclock = MINTAUDIO_frequencies[MINTAUDIO_numfreq].masterclock;
    prediv = MINTAUDIO_frequencies[MINTAUDIO_numfreq].predivisor;
    switch (cookie_mcsn->play) {
    case MCSN_TT:
        Devconnect(DMAPLAY, DAC, CLK25M, CLKOLD, 1);
        Soundcmd(SETPRESCALE, prediv);
        DEBUG_PRINT((DEBUG_NAME "STE/TT prescaler selected\n"));
        break;
    case MCSN_FALCON:
        Devconnect(DMAPLAY, DAC, dmaclock, prediv, 1);
        DEBUG_PRINT((DEBUG_NAME "Falcon prescaler selected\n"));
        break;
    }

    /* Set buffer */
    buffer = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
    if (Setbuffer(0, buffer, buffer + this->spec.size) < 0) {
        DEBUG_PRINT((DEBUG_NAME "Setbuffer() failed\n"));
    }

    if (SDL_MintAudio_mint_present) {
        SDL_MintAudio_thread_pid = tfork(SDL_MintAudio_Thread, 0);
    } else {
        /* Install interrupt */
        Jdisint(MFP_DMASOUND);
        Xbtimer(XB_TIMERA, 8, 1, SDL_MintAudio_XbiosInterrupt);
        Jenabint(MFP_DMASOUND);

        if (Setinterrupt(SI_TIMERA, SI_PLAY) < 0) {
            DEBUG_PRINT((DEBUG_NAME "Setinterrupt() failed\n"));
        }
    }

    /* Go */
    Buffoper(SB_PLA_ENA | SB_PLA_RPT);
    DEBUG_PRINT((DEBUG_NAME "hardware initialized\n"));
}

static int
MINTMCSN_OpenDevice(_THIS, const char *devname, int iscapture)
{
    /* Lock sound system */
    if (Locksnd() != 1) {
        SDL_SetError("MINTMCSN_OpenDevice: Audio system already in use");
        return 0;
    }

    SDL_MintAudio_device = this;

    /* Check audio capabilities */
    if (MINTMCSN_CheckAudio(this) == -1) {
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
    MINTMCSN_InitAudio(this);

    return 1;                   /* good to go. */
}

static int
MINTMCSN_Init(SDL_AudioDriverImpl * impl)
{
    unsigned long dummy = 0;

    SDL_MintAudio_mint_present = (Getcookie(C_MiNT, &dummy) == C_FOUND);

    /* We can't use XBIOS in interrupt with Magic, don't know about thread */
    if (Getcookie(C_MagX, &dummy) == C_FOUND) {
        return 0;
    }

    /* Cookie _MCH present ? if not, assume ST machine */
    if (Getcookie(C__MCH, &cookie_mch) == C_NOTFOUND) {
        cookie_mch = MCH_ST;
    }

    /* Cookie _SND present ? if not, assume ST machine */
    if (Getcookie(C__SND, &cookie_snd) == C_NOTFOUND) {
        cookie_snd = SND_PSG;
    }

    /* Check if we have 16 bits audio */
    if ((cookie_snd & SND_16BIT) == 0) {
        SDL_SetError(DEBUG_NAME "no 16-bit sound");
        return 0;
    }

    /* Cookie MCSN present ? */
    if (Getcookie(C_McSn, (long *) &cookie_mcsn) != C_FOUND) {
        SDL_SetError(DEBUG_NAME "no MCSN audio");
        return 0;
    }

    /* Check if interrupt at end of replay */
    if (cookie_mcsn->pint == 0) {
        SDL_SetError(DEBUG_NAME "no interrupt at end of replay");
        return 0;
    }

    /* Check if audio is lockable */
    if (Locksnd() != 1) {
        SDL_SetError(DEBUG_NAME "audio locked by other application");
        return 0;
    }

    Unlocksnd();

    DEBUG_PRINT((DEBUG_NAME "MCSN audio available!\n"));

    /* Set the function pointers */
    impl->OpenDevice = MINTMCSN_OpenDevice;
    impl->CloseDevice = MINTMCSN_CloseDevice;
    impl->LockDevice = MINTMCSN_LockDevice;
    impl->UnlockDevice = MINTMCSN_UnlockDevice;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->ProvidesOwnCallbackThread = 1;
    impl->SkipMixerLock = 1;

    return 2;  /* 2 == definitely has an audio device. */
}

AudioBootStrap MINTAUDIO_MCSN_bootstrap = {
    MINT_AUDIO_DRIVER_NAME, "MiNT MCSN audio driver", MINTMCSN_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
