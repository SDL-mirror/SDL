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

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sched.h>
#include <sys/select.h>
#include <sys/neutrino.h>
#include <sys/asoundlib.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "SDL_nto_audio.h"

/* The tag name used by NTO audio */
#define DRIVER_NAME "qsa-nto"

/* default channel communication parameters */
#define DEFAULT_CPARAMS_RATE 22050
#define DEFAULT_CPARAMS_VOICES 1
/* FIXME: need to add in the near future flexible logic with frag_size and frags count */
#define DEFAULT_CPARAMS_FRAG_SIZE 4096
#define DEFAULT_CPARAMS_FRAGS_MIN 1
#define DEFAULT_CPARAMS_FRAGS_MAX 1

/* Open the audio device for playback, and don't block if busy */
#define OPEN_FLAGS SND_PCM_OPEN_PLAYBACK

#define QSA_NO_WORKAROUNDS  0x00000000
#define QSA_MMAP_WORKAROUND 0x00000001

struct BuggyCards
{
    char *cardname;
    unsigned long bugtype;
};

#define QSA_WA_CARDS 3

struct BuggyCards buggycards[QSA_WA_CARDS] = {
    {"Sound Blaster Live!", QSA_MMAP_WORKAROUND},
    {"Vortex 8820", QSA_MMAP_WORKAROUND},
    {"Vortex 8830", QSA_MMAP_WORKAROUND},
};


static inline void
NTO_SetError(const char *fn, int rval)
{
    SDL_SetError("NTO: %s failed: %s", fn, snd_strerror(rval));
}


/* card names check to apply the workarounds */
static int
NTO_CheckBuggyCards(_THIS, unsigned long checkfor)
{
    char scardname[33];
    int it;

    if (snd_card_get_name(this->hidden->cardno, scardname, 32) < 0) {
        return 0;
    }

    for (it = 0; it < QSA_WA_CARDS; it++) {
        if (SDL_strcmp(buggycards[it].cardname, scardname) == 0) {
            if (buggycards[it].bugtype == checkfor) {
                return 1;
            }
        }
    }

    return 0;
}

static void
NTO_ThreadInit(_THIS)
{
    struct sched_param param;
    int status = SchedGet(0, 0, &param);

    /* increasing default 10 priority to 25 to avoid jerky sound */
    param.sched_priority = param.sched_curpriority + 15;
    status = SchedSet(0, 0, SCHED_NOCHANGE, &param);
}

/* PCM transfer channel parameters initialize function */
static void
NTO_InitAudioParams(snd_pcm_channel_params_t * cpars)
{
    SDL_memset(cpars, 0, sizeof(snd_pcm_channel_params_t));

    cpars->channel = SND_PCM_CHANNEL_PLAYBACK;
    cpars->mode = SND_PCM_MODE_BLOCK;
    cpars->start_mode = SND_PCM_START_DATA;
    cpars->stop_mode = SND_PCM_STOP_STOP;
    cpars->format.format = SND_PCM_SFMT_S16_LE;
    cpars->format.interleave = 1;
    cpars->format.rate = DEFAULT_CPARAMS_RATE;
    cpars->format.voices = DEFAULT_CPARAMS_VOICES;
    cpars->buf.block.frag_size = DEFAULT_CPARAMS_FRAG_SIZE;
    cpars->buf.block.frags_min = DEFAULT_CPARAMS_FRAGS_MIN;
    cpars->buf.block.frags_max = DEFAULT_CPARAMS_FRAGS_MAX;
}


/* This function waits until it is possible to write a full sound buffer */
static void
NTO_WaitDevice(_THIS)
{
    fd_set wfds;
    int selectret;

    FD_ZERO(&wfds);
    FD_SET(this->hidden->audio_fd, &wfds);

    do {
        selectret =
            select(this->hidden->audio_fd + 1, NULL, &wfds, NULL, NULL);
        switch (selectret) {
        case -1:
        case 0:
            SDL_SetError("NTO: select() failed: %s\n", strerror(errno));
            return;
        default:
            if (FD_ISSET(this->hidden->audio_fd, &wfds)) {
                return;
            }
            break;
        }
    } while (1);
}

static void
NTO_PlayDevice(_THIS)
{
    snd_pcm_channel_status_t cstatus;
    int written, rval;
    int towrite;
    void *pcmbuffer;

    if ((!this->enabled) || (!this->hidden)) {
        return;
    }

    towrite = this->spec.size;
    pcmbuffer = this->hidden->pcm_buf;

    /* Write the audio data, checking for EAGAIN (buffer full) and underrun */
    do {
        written = snd_pcm_plugin_write(this->hidden->audio_handle,
                                       pcmbuffer, towrite);
        if (written != towrite) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                /* Let a little CPU time go by and try to write again */
                SDL_Delay(1);
                /* if we wrote some data */
                towrite -= written;
                pcmbuffer += written * this->spec.channels;
                continue;
            } else if ((errno == EINVAL) || (errno == EIO)) {
                SDL_memset(&cstatus, 0, sizeof(cstatus));
                cstatus.channel = SND_PCM_CHANNEL_PLAYBACK;
                rval = snd_pcm_plugin_status(this->hidden->audio_handle,
                                             &cstatus);
                if (rval < 0) {
                    NTO_SetError("snd_pcm_plugin_status", rval);
                    return;
                }

                if ((cstatus.status == SND_PCM_STATUS_UNDERRUN) ||
                    (cstatus.status == SND_PCM_STATUS_READY)) {
                    rval = snd_pcm_plugin_prepare(this->hidden->audio_handle,
                                                  SND_PCM_CHANNEL_PLAYBACK);
                    if (rval < 0) {
                        NTO_SetError("snd_pcm_plugin_prepare", rval);
                        return;
                    }
                }
                continue;
            } else {
                return;
            }
        } else {
            /* we wrote all remaining data */
            towrite -= written;
            pcmbuffer += written * this->spec.channels;
        }
    } while ((towrite > 0) && (this->enabled));

    /* If we couldn't write, assume fatal error for now */
    if (towrite != 0) {
        this->enabled = 0;
    }
}

static Uint8 *
NTO_GetDeviceBuf(_THIS)
{
    return this->hidden->pcm_buf;
}

static void
NTO_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->audio_handle != NULL) {
            snd_pcm_plugin_flush(this->hidden->audio_handle,
                                 SND_PCM_CHANNEL_PLAYBACK);
            snd_pcm_close(this->hidden->audio_handle);
            this->hidden->audio_handle = NULL;
        }
        if (this->hidden->pcm_buf != NULL) {
            SDL_FreeAudioMem(this->hidden->pcm_buf);
            this->hidden->pcm_buf = NULL;
        }
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
NTO_OpenDevice(_THIS, const char *devname, int iscapture)
{
    int rval = 0;
    int format = 0;
    SDL_AudioFormat test_format = 0;
    int found = 0;
    snd_pcm_channel_setup_t csetup;
    snd_pcm_channel_params_t cparams;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* initialize channel transfer parameters to default */
    NTO_InitAudioParams(&cparams);

    /* Open the audio device */
    rval = snd_pcm_open_preferred(&this->hidden->audio_handle,
                                  &this->hidden->cardno,
                                  &this->hidden->deviceno, OPEN_FLAGS);

    if (rval < 0) {
        NTO_CloseDevice(this);
        NTO_SetError("snd_pcm_open", rval);
        return 0;
    }

    if (!NTO_CheckBuggyCards(this, QSA_MMAP_WORKAROUND)) {
        /* enable count status parameter */
        rval = snd_pcm_plugin_set_disable(this->hidden->audio_handle,
                                          PLUGIN_DISABLE_MMAP);
        if (rval < 0) {
            NTO_CloseDevice(this);
            NTO_SetError("snd_pcm_plugin_set_disable", rval);
            return 0;
        }
    }

    /* Try for a closest match on audio format */
    format = 0;
    /* can't use format as SND_PCM_SFMT_U8 = 0 in nto */
    found = 0;

    for (test_format = SDL_FirstAudioFormat(this->spec.format); !found;) {
        /* if match found set format to equivalent ALSA format */
        switch (test_format) {
        case AUDIO_U8:
            format = SND_PCM_SFMT_U8;
            found = 1;
            break;
        case AUDIO_S8:
            format = SND_PCM_SFMT_S8;
            found = 1;
            break;
        case AUDIO_S16LSB:
            format = SND_PCM_SFMT_S16_LE;
            found = 1;
            break;
        case AUDIO_S16MSB:
            format = SND_PCM_SFMT_S16_BE;
            found = 1;
            break;
        case AUDIO_U16LSB:
            format = SND_PCM_SFMT_U16_LE;
            found = 1;
            break;
        case AUDIO_U16MSB:
            format = SND_PCM_SFMT_U16_BE;
            found = 1;
            break;
        case AUDIO_S32LSB:
            format = SND_PCM_SFMT_S32_LE;
            found = 1;
            break;
        case AUDIO_S32MSB:
            format = SND_PCM_SFMT_S32_BE;
            found = 1;
            break;
        case AUDIO_F32LSB:
            format = SND_PCM_SFMT_FLOAT_LE;
            found = 1;
            break;
        case AUDIO_F32MSB:
            format = SND_PCM_SFMT_FLOAT_BE;
            found = 1;
            break;
        default:
            break;
        }

        if (!found) {
            test_format = SDL_NextAudioFormat();
        }
    }

    /* assumes test_format not 0 on success */
    if (test_format == 0) {
        NTO_CloseDevice(this);
        SDL_SetError("NTO: Couldn't find any hardware audio formats");
        return 0;
    }

    this->spec.format = test_format;

    /* Set the audio format */
    cparams.format.format = format;

    /* Set mono or stereo audio (currently only two channels supported) */
    cparams.format.voices = this->spec.channels;

    /* Set rate */
    cparams.format.rate = this->spec.freq;

    /* Setup the transfer parameters according to cparams */
    rval = snd_pcm_plugin_params(this->hidden->audio_handle, &cparams);
    if (rval < 0) {
        NTO_CloseDevice(this);
        NTO_SetError("snd_pcm_channel_params", rval);
        return 0;
    }

    /* Make sure channel is setup right one last time */
    SDL_memset(&csetup, '\0', sizeof(csetup));
    csetup.channel = SND_PCM_CHANNEL_PLAYBACK;
    if (snd_pcm_plugin_setup(this->hidden->audio_handle, &csetup) < 0) {
        NTO_CloseDevice(this);
        SDL_SetError("NTO: Unable to setup playback channel\n");
        return 0;
    }

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    this->hidden->pcm_len = this->spec.size;

    if (this->hidden->pcm_len == 0) {
        this->hidden->pcm_len =
            csetup.buf.block.frag_size * this->spec.channels *
            (snd_pcm_format_width(format) / 8);
    }

    /*
     * Allocate memory to the audio buffer and initialize with silence
     *  (Note that buffer size must be a multiple of fragment size, so find
     *  closest multiple)
     */
    this->hidden->pcm_buf =
        (Uint8 *) SDL_AllocAudioMem(this->hidden->pcm_len);
    if (this->hidden->pcm_buf == NULL) {
        NTO_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->pcm_buf, this->spec.silence,
               this->hidden->pcm_len);

    /* get the file descriptor */
    this->hidden->audio_fd =
        snd_pcm_file_descriptor(this->hidden->audio_handle,
                                SND_PCM_CHANNEL_PLAYBACK);
    if (this->hidden->audio_fd < 0) {
        NTO_CloseDevice(this);
        NTO_SetError("snd_pcm_file_descriptor", rval);
        return 0;
    }

    /* Trigger audio playback */
    rval = snd_pcm_plugin_prepare(this->hidden->audio_handle,
                                  SND_PCM_CHANNEL_PLAYBACK);
    if (rval < 0) {
        NTO_CloseDevice(this);
        NTO_SetError("snd_pcm_plugin_prepare", rval);
        return 0;
    }

    /* We're really ready to rock and roll. :-) */
    return 1;
}


static int
NTO_Init(SDL_AudioDriverImpl * impl)
{
    /*  See if we can open a nonblocking channel. */
    snd_pcm_t *handle = NULL;
    int rval = snd_pcm_open_preferred(&handle, NULL, NULL, OPEN_FLAGS);
    if (rval < 0) {
        SDL_SetError("NTO: couldn't open preferred audio device");
        return 0;
    }
    if ((rval = snd_pcm_close(handle)) < 0) {
        SDL_SetError("NTO: couldn't close test audio device");
        return 0;
    }

    /* Set the function pointers */
    impl->OpenDevice = NTO_OpenDevice;
    impl->ThreadInit = NTO_ThreadInit;
    impl->WaitDevice = NTO_WaitDevice;
    impl->PlayDevice = NTO_PlayDevice;
    impl->GetDeviceBuf = NTO_GetDeviceBuf;
    impl->CloseDevice = NTO_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = 1;       /* !!! FIXME: add device enum! */

    return 1;
}

AudioBootStrap QNXNTOAUDIO_bootstrap = {
    DRIVER_NAME, "QNX6 QSA-NTO Audio", NTO_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
