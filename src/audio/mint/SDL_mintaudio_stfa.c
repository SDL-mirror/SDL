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
#include "SDL_config.h"

/*
	MiNT audio driver
	using XBIOS functions (STFA driver)

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
#include "SDL_mintaudio_stfa.h"

/*--- Defines ---*/

#define MINT_AUDIO_DRIVER_NAME "mint_stfa"

/* Debug print info */
#define DEBUG_NAME "audio:stfa: "
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
static cookie_stfa_t *cookie_stfa = NULL;

static const int freqs[16] = {
    4995, 6269, 7493, 8192,
    9830, 10971, 12538, 14985,
    16384, 19819, 21943, 24576,
    30720, 32336, 43885, 49152
};

static void
MINTSTFA_LockDevice(_THIS)
{
    /* Stop replay */
    void *oldpile = (void *) Super(0);
    cookie_stfa->sound_enable = STFA_PLAY_DISABLE;
    Super(oldpile);
}

static void
MINTSTFA_UnlockDevice(_THIS)
{
    /* Restart replay */
    void *oldpile = (void *) Super(0);
    cookie_stfa->sound_enable = STFA_PLAY_ENABLE | STFA_PLAY_REPEAT;
    Super(oldpile);
}

static void
MINTSTFA_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        /* Stop replay */
        void *oldpile = (void *) Super(0);
        cookie_stfa->sound_enable = STFA_PLAY_DISABLE;
        Super(oldpile);

        /* Wait if currently playing sound */
        while (SDL_MintAudio_mutex != 0) {
        }

        /* Clear buffers */
        if (SDL_MintAudio_audiobuf[0]) {
            Mfree(SDL_MintAudio_audiobuf[0]);
            SDL_MintAudio_audiobuf[0] = SDL_MintAudio_audiobuf[1] = NULL;
        }

        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
MINTSTFA_CheckAudio(_THIS)
{
    int i;

    DEBUG_PRINT((DEBUG_NAME "asked: %d bits, ",
                 SDL_AUDIO_BITSIZE(this->spec.format)));
    DEBUG_PRINT(("float=%d, ", SDL_AUDIO_ISFLOAT(this->spec.format)));
    DEBUG_PRINT(("signed=%d, ", SDL_AUDIO_ISSIGNED(this->spec.format)));
    DEBUG_PRINT(("big endian=%d, ",
                 SDL_AUDIO_ISBIGENDIAN(this->spec.format)));
    DEBUG_PRINT(("channels=%d, ", this->spec.channels));
    DEBUG_PRINT(("freq=%d\n", this->spec.freq));

    if (SDL_AUDIO_BITSIZE(this->spec.format) > 16) {
        this->spec.format = AUDIO_S16SYS;       /* clamp out int32/float32 ... */
    }

    if (this->spec.channels > 2) {
        this->spec.channels = 2;        /* no more than stereo! */
    }

    /* Check formats available */
    MINTAUDIO_freqcount = 0;
    for (i = 0; i < 16; i++) {
        SDL_MintAudio_AddFrequency(this, freqs[i], 0, i, -1);
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
MINTSTFA_InitAudio(_THIS)
{
    void *buffer = SDL_MintAudio_audiobuf[SDL_MintAudio_numbuf];
    void *oldpile = (void *) Super(0);

    /* Stop replay */
    cookie_stfa->sound_enable = STFA_PLAY_DISABLE;

    /* Select replay format */
    cookie_stfa->sound_control =
        MINTAUDIO_frequencies[MINTAUDIO_numfreq].predivisor;
    if (SDL_AUDIO_BITSIZE(this->spec.format) == 8) {
        cookie_stfa->sound_control |= STFA_FORMAT_8BIT;
    } else {
        cookie_stfa->sound_control |= STFA_FORMAT_16BIT;
    }
    if (this->spec.channels == 2) {
        cookie_stfa->sound_control |= STFA_FORMAT_STEREO;
    } else {
        cookie_stfa->sound_control |= STFA_FORMAT_MONO;
    }
    if (SDL_AUDIO_ISSIGNED(this->spec.format) != 0) {
        cookie_stfa->sound_control |= STFA_FORMAT_SIGNED;
    } else {
        cookie_stfa->sound_control |= STFA_FORMAT_UNSIGNED;
    }
    if (SDL_AUDIO_ISBIGENDIAN(this->spec.format) != 0) {
        cookie_stfa->sound_control |= STFA_FORMAT_BIGENDIAN;
    } else {
        cookie_stfa->sound_control |= STFA_FORMAT_LITENDIAN;
    }

    /* Set buffer */
    cookie_stfa->sound_start = (unsigned long) buffer;
    cookie_stfa->sound_end = (unsigned long) (buffer + this->spec.size);

    /* Set interrupt */
    cookie_stfa->stfa_it = SDL_MintAudio_StfaInterrupt;

    /* Restart replay */
    cookie_stfa->sound_enable = STFA_PLAY_ENABLE | STFA_PLAY_REPEAT;

    Super(oldpile);

    DEBUG_PRINT((DEBUG_NAME "hardware initialized\n"));
}

static int
MINTSTFA_OpenDevice(_THIS, const char *devname, int iscapture)
{
    SDL_MintAudio_device = this;

    /* Check audio capabilities */
    if (MINTSTFA_CheckAudio(this) == -1) {
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
        SDL_OutOfMemory();
        SDL_free(this->hidden);
        this->hidden = NULL;
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
    MINTSTFA_InitAudio(this);

    return 1;                   /* good to go. */
}


static int
MINTSTFA_Init(SDL_AudioDriverImpl * impl)
{
    /* Cookie _MCH present ? if not, assume ST machine */
    if (Getcookie(C__MCH, &cookie_mch) == C_NOTFOUND) {
        cookie_mch = MCH_ST;
    }

    /* Cookie _SND present ? if not, assume ST machine */
    if (Getcookie(C__SND, &cookie_snd) == C_NOTFOUND) {
        cookie_snd = SND_PSG;
    }

    /* Cookie STFA present ? */
    if (Getcookie(C_STFA, (long *) &cookie_stfa) != C_FOUND) {
        SDL_SetError(DEBUG_NAME "no STFA audio");
        return (0);
    }

    SDL_MintAudio_stfa = cookie_stfa;

    DEBUG_PRINT((DEBUG_NAME "STFA audio available!\n"));

    /* Set the function pointers */
    impl->OpenDevice = MINTSTFA_OpenDevice;
    impl->CloseDevice = MINTSTFA_CloseDevice;
    impl->LockDevice = MINTSTFA_LockDevice;
    impl->UnlockDevice = MINTSTFA_UnlockDevice;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->ProvidesOwnCallbackThread = 1;
    impl->SkipMixerLock = 1;

    return 1;
}

AudioBootStrap MINTAUDIO_STFA_bootstrap = {
    MINT_AUDIO_DRIVER_NAME, "MiNT STFA audio driver", MINTSTFA_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
