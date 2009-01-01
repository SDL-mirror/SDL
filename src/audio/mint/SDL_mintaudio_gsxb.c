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
	using XBIOS functions (GSXB compatible driver)

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
#include "SDL_mintaudio_gsxb.h"

/*--- Defines ---*/

#define MINT_AUDIO_DRIVER_NAME "mint_gsxb"

/* Debug print info */
#define DEBUG_NAME "audio:gsxb: "
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#else
#define DEBUG_PRINT(what)
#endif

/*--- Static variables ---*/

static unsigned long cookie_snd, cookie_gsxb;

/*--- Audio driver functions ---*/

/* GSXB callbacks */
static void MINTGSXB_GsxbInterrupt(void);
static void MINTGSXB_GsxbNullInterrupt(void);

static void
MINTGSXB_LockDevice(_THIS)
{
    /* Stop replay */
    Buffoper(0);
}

static void
MINTGSXB_UnlockDevice(_THIS)
{
    /* Restart replay */
    Buffoper(SB_PLA_ENA | SB_PLA_RPT);
}

static void
MINTGSXB_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        /* Stop replay */
        Buffoper(0);

        /* Uninstall interrupt */
        if (NSetinterrupt(2, SI_NONE, MINTGSXB_GsxbNullInterrupt) < 0) {
            DEBUG_PRINT((DEBUG_NAME "NSetinterrupt() failed in close\n"));
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
MINTGSXB_CheckAudio(_THIS)
{
    long snd_format;
    int i, resolution, format_signed, format_bigendian;
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(this->spec.format);
    int valid_datatype = 0;

    resolution = SDL_AUDIO_BITSIZE(this->spec.format);
    format_signed = SDL_AUDIO_ISSIGNED(this->spec.format);
    format_bigendian = SDL_AUDIO_ISBIGENDIAN(this->spec.format);

    DEBUG_PRINT((DEBUG_NAME "asked: %d bits, ", resolution));
    DEBUG_PRINT(("float=%d, ", SDL_AUDIO_ISFLOAT(this->spec.format)));
    DEBUG_PRINT(("signed=%d, ", format_signed));
    DEBUG_PRINT(("big endian=%d, ", format_bigendian));
    DEBUG_PRINT(("channels=%d, ", this->spec.channels));
    DEBUG_PRINT(("freq=%d\n", this->spec.freq));

    if (this->spec.channels > 2) {
        this->spec.channels = 2;        /* no more than stereo! */
    }

    while ((!valid_datatype) && (test_format)) {
        /* Check formats available */
        snd_format = Sndstatus(SND_QUERYFORMATS);
        this->spec.format = test_format;
        resolution = SDL_AUDIO_BITSIZE(this->spec.format);
        format_signed = SDL_AUDIO_ISSIGNED(this->spec.format);
        format_bigendian = SDL_AUDIO_ISBIGENDIAN(this->spec.format);
        switch (test_format) {
        case AUDIO_U8:
        case AUDIO_S8:
            if (snd_format & SND_FORMAT8) {
                valid_datatype = 1;
                snd_format = Sndstatus(SND_QUERY8BIT);
            }
            break;

        case AUDIO_U16LSB:
        case AUDIO_S16LSB:
        case AUDIO_U16MSB:
        case AUDIO_S16MSB:
            if (snd_format & SND_FORMAT16) {
                valid_datatype = 1;
                snd_format = Sndstatus(SND_QUERY16BIT);
            }
            break;

        case AUDIO_S32LSB:
        case AUDIO_S32MSB:
            if (snd_format & SND_FORMAT32) {
                valid_datatype = 1;
                snd_format = Sndstatus(SND_QUERY32BIT);
            }
            break;

            /* no float support... */

        default:
            test_format = SDL_NextAudioFormat();
            break;
        }
    }

    if (!valid_datatype) {
        SDL_SetError("Unsupported audio format");
        return (-1);
    }

    /* Check signed/unsigned format */
    if (format_signed) {
        if (snd_format & SND_FORMATSIGNED) {
            /* Ok */
        } else if (snd_format & SND_FORMATUNSIGNED) {
            /* Give unsigned format */
            this->spec.format = this->spec.format & (~SDL_AUDIO_MASK_SIGNED);
        }
    } else {
        if (snd_format & SND_FORMATUNSIGNED) {
            /* Ok */
        } else if (snd_format & SND_FORMATSIGNED) {
            /* Give signed format */
            this->spec.format |= SDL_AUDIO_MASK_SIGNED;
        }
    }

    if (format_bigendian) {
        if (snd_format & SND_FORMATBIGENDIAN) {
            /* Ok */
        } else if (snd_format & SND_FORMATLITTLEENDIAN) {
            /* Give little endian format */
            this->spec.format = this->spec.format & (~SDL_AUDIO_MASK_ENDIAN);
        }
    } else {
        if (snd_format & SND_FORMATLITTLEENDIAN) {
            /* Ok */
        } else if (snd_format & SND_FORMATBIGENDIAN) {
            /* Give big endian format */
            this->spec.format |= SDL_AUDIO_MASK_ENDIAN;
        }
    }

    /* Calculate and select the closest frequency */
    MINTAUDIO_freqcount = 0;
    for (i = 1; i < 4; i++) {
        SDL_MintAudio_AddFrequency(this,
                                   MASTERCLOCK_44K / (MASTERPREDIV_MILAN *
                                                      (1 << i)),
                                   MASTERCLOCK_44K, (1 << i) - 1, -1);
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
MINTGSXB_InitAudio(_THIS)
{
    int channels_mode, prediv;
    void *buffer;

    /* Stop currently playing sound */
    Buffoper(0);

    /* Set replay tracks */
    Settracks(0, 0);
    Setmontracks(0);

    /* Select replay format */
    switch (SDL_AUDIO_BITSIZE(this->spec.format)) {
    case 8:
        if (this->spec.channels == 2) {
            channels_mode = STEREO8;
        } else {
            channels_mode = MONO8;
        }
        break;
    case 16:
        if (this->spec.channels == 2) {
            channels_mode = STEREO16;
        } else {
            channels_mode = MONO16;
        }
        break;
    case 32:
        if (this->spec.channels == 2) {
            channels_mode = STEREO32;
        } else {
            channels_mode = MONO32;
        }
        break;
    default:
        channels_mode = STEREO16;
        break;
    }
    if (Setmode(channels_mode) < 0) {
        DEBUG_PRINT((DEBUG_NAME "Setmode() failed\n"));
    }

    prediv = MINTAUDIO_frequencies[MINTAUDIO_numfreq].predivisor;
    Devconnect(DMAPLAY, DAC, CLKEXT, prediv, 1);

    /* Set buffer */
    buffer = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
    if (Setbuffer(0, buffer, buffer + this->spec.size) < 0) {
        DEBUG_PRINT((DEBUG_NAME "Setbuffer() failed\n"));
    }

    /* Install interrupt */
    if (NSetinterrupt(2, SI_PLAY, MINTGSXB_GsxbInterrupt) < 0) {
        DEBUG_PRINT((DEBUG_NAME "NSetinterrupt() failed\n"));
    }

    /* Go */
    Buffoper(SB_PLA_ENA | SB_PLA_RPT);
    DEBUG_PRINT((DEBUG_NAME "hardware initialized\n"));
}

static int
MINTGSXB_OpenDevice(_THIS, const char *devname, int iscapture)
{
    /* Lock sound system */
    if (Locksnd() != 1) {
        SDL_SetError("MINTGSXB_OpenDevice: Audio system already in use");
        return 0;
    }

    SDL_MintAudio_device = this;

    /* Check audio capabilities */
    if (MINTGSXB_CheckAudio(this) == -1) {
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
    MINTGSXB_InitAudio(this);

    return 1;                   /* good to go. */
}

static void
MINTGSXB_GsxbInterrupt(void)
{
    Uint8 *newbuf;

    if (SDL_MintAudio_mutex)
        return;

    SDL_MintAudio_mutex = 1;

    SDL_MintAudio_numbuf ^= 1;
    SDL_MintAudio_Callback();
    newbuf = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
    Setbuffer(0, newbuf, newbuf + SDL_MintAudio_audiosize);

    SDL_MintAudio_mutex = 0;
}

static void
MINTGSXB_GsxbNullInterrupt(void)
{
}

static int
MINTGSXB_Init(SDL_AudioDriverImpl * impl)
{
    /* Cookie _SND present ? if not, assume ST machine */
    if (Getcookie(C__SND, &cookie_snd) == C_NOTFOUND) {
        cookie_snd = SND_PSG;
    }

    /* Check if we have 16 bits audio */
    if ((cookie_snd & SND_16BIT) == 0) {
        SDL_SetError(DEBUG_NAME "no 16-bit sound");
        return 0;
    }

    /* Cookie GSXB present ? */
    cookie_gsxb = (Getcookie(C_GSXB, &cookie_gsxb) == C_FOUND);

    /* Is it GSXB ? */
    if (((cookie_snd & SND_GSXB) == 0) || (cookie_gsxb == 0)) {
        SDL_SetError(DEBUG_NAME "no GSXB audio");
        return 0;
    }

    /* Check if audio is lockable */
    if (Locksnd() != 1) {
        SDL_SetError(DEBUG_NAME "audio locked by other application");
        return 0;
    }

    Unlocksnd();

    DEBUG_PRINT((DEBUG_NAME "GSXB audio available!\n"));

    /* Set the function pointers */
    impl->OpenDevice = MINTGSXB_OpenDevice;
    impl->CloseDevice = MINTGSXB_CloseDevice;
    impl->LockDevice = MINTGSXB_LockDevice;
    impl->UnlockDevice = MINTGSXB_UnlockDevice;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->ProvidesOwnCallbackThread = 1;
    impl->SkipMixerLock = 1;

    return 2;  /* 2 == definitely has an audio device. */
}

AudioBootStrap MINTAUDIO_GSXB_bootstrap = {
    MINT_AUDIO_DRIVER_NAME, "MiNT GSXB audio driver", MINTGSXB_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
