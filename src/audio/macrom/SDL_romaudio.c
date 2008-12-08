/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* This should work on PowerPC and Intel Mac OS X, and Carbonized Mac OS 9. */

#if defined(__APPLE__) && defined(__MACH__)
#  define SDL_MACOS_NAME "Mac OS X"
#  include <Carbon/Carbon.h>
#elif TARGET_API_MAC_CARBON && (UNIVERSAL_INTERFACES_VERSION > 0x0335)
#  define SDL_MACOS_NAME "Mac OS 9"
#  include <Carbon.h>
#else
#  define SDL_MACOS_NAME "Mac OS 9"
#  include <Sound.h>            /* SoundManager interface */
#  include <Gestalt.h>
#  include <DriverServices.h>
#endif

#if !defined(NewSndCallBackUPP) && (UNIVERSAL_INTERFACES_VERSION < 0x0335)
#if !defined(NewSndCallBackProc)        /* avoid circular redefinition... */
#define NewSndCallBackUPP NewSndCallBackProc
#endif
#if !defined(NewSndCallBackUPP)
#define NewSndCallBackUPP NewSndCallBackProc
#endif
#endif

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"
#include "SDL_romaudio.h"

#pragma options align=power

static volatile SInt32 audio_is_locked = 0;
static volatile SInt32 need_to_mix = 0;

static UInt8 *buffer[2];
static volatile UInt32 running = 0;
static CmpSoundHeader header;
static volatile Uint32 fill_me = 0;


static void
mix_buffer(SDL_AudioDevice * audio, UInt8 * buffer)
{
    if (!audio->paused) {
#ifdef __MACOSX__
        SDL_mutexP(audio->mixer_lock);
#endif
        if (audio->convert.needed) {
            audio->spec.callback(audio->spec.userdata,
                                 (Uint8 *) audio->convert.buf,
                                 audio->convert.len);
            SDL_ConvertAudio(&audio->convert);
            if (audio->convert.len_cvt != audio->spec.size) {
                /* Uh oh... probably crashes here */ ;
            }
            SDL_memcpy(buffer, audio->convert.buf, audio->convert.len_cvt);
        } else {
            audio->spec.callback(audio->spec.userdata, buffer,
                                 audio->spec.size);
        }
#ifdef __MACOSX__
        SDL_mutexV(audio->mixer_lock);
#endif
    }

    DecrementAtomic((SInt32 *) & need_to_mix);
}

#ifndef __MACOSX__
static void
SNDMGR_LockDevice(_THIS)
{
    IncrementAtomic((SInt32 *) & audio_is_locked);
}

static void
SNDMGR_UnlockDevice(_THIS)
{
    SInt32 oldval;

    oldval = DecrementAtomic((SInt32 *) & audio_is_locked);
    if (oldval != 1)            /* != 1 means audio is still locked. */
        return;

    /* Did we miss the chance to mix in an interrupt? Do it now. */
    if (BitAndAtomic(0xFFFFFFFF, (UInt32 *) & need_to_mix)) {
        /*
         * Note that this could be a problem if you missed an interrupt
         *  while the audio was locked, and get preempted by a second
         *  interrupt here, but that means you locked for way too long anyhow.
         */
        mix_buffer(this, buffer[fill_me]);
    }
}
#endif // __MACOSX__

static void
callBackProc(SndChannel * chan, SndCommand * cmd_passed)
{
    UInt32 play_me;
    SndCommand cmd;
    SDL_AudioDevice *audio = (SDL_AudioDevice *) chan->userInfo;

    IncrementAtomic((SInt32 *) & need_to_mix);

    fill_me = cmd_passed->param2;       /* buffer that has just finished playing, so fill it */
    play_me = !fill_me;         /* filled buffer to play _now_ */

    if (!audio->enabled) {
        return;
    }

    /* queue previously mixed buffer for playback. */
    header.samplePtr = (Ptr) buffer[play_me];
    cmd.cmd = bufferCmd;
    cmd.param1 = 0;
    cmd.param2 = (long) &header;
    SndDoCommand(chan, &cmd, 0);

    SDL_memset(buffer[fill_me], 0, audio->spec.size);

    /*
     * if audio device isn't locked, mix the next buffer to be queued in
     *  the memory block that just finished playing.
     */
    if (!BitAndAtomic(0xFFFFFFFF, (UInt32 *) & audio_is_locked)) {
        mix_buffer(audio, buffer[fill_me]);
    }

    /* set this callback to run again when current buffer drains. */
    if (running) {
        cmd.cmd = callBackCmd;
        cmd.param1 = 0;
        cmd.param2 = play_me;

        SndDoCommand(chan, &cmd, 0);
    }
}

static void
SNDMGR_CloseDevice(_THIS)
{
    running = 0;

    if (this->hidden != NULL) {
        if (this->hidden->channel) {
            SndDisposeChannel(this->hidden->channel, true);
            this->hidden->channel = NULL;
        }

        SDL_free(buffer[0]);
        SDL_free(buffer[1]);
        buffer[0] = buffer[1] = NULL;

        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
SNDMGR_OpenDevice(_THIS, const char *devname, int iscapture)
{
    SDL_AudioSpec *spec = &this->spec;
    SndChannelPtr channel = NULL;
    SndCallBackUPP callback;
    int sample_bits;
    int i;
    long initOptions;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* !!! FIXME: iterate through format matrix... */
    /* Very few conversions are required, but... */
    switch (spec->format) {
    case AUDIO_S8:
        spec->format = AUDIO_U8;
        break;
    case AUDIO_U16LSB:
        spec->format = AUDIO_S16LSB;
        break;
    case AUDIO_U16MSB:
        spec->format = AUDIO_S16MSB;
        break;
    case AUDIO_F32LSB:
        spec->format = AUDIO_F32MSB;
        break;
    }
    SDL_CalculateAudioSpec(&this->spec);

    /* initialize bufferCmd header */
    SDL_memset(&header, 0, sizeof(header));
    callback = (SndCallBackUPP) NewSndCallBackUPP(callBackProc);
    sample_bits = spec->size / spec->samples / spec->channels * 8;

#ifdef DEBUG_AUDIO
    fprintf(stderr,
            "Audio format 0x%x, channels = %d, sample_bits = %d, frequency = %d\n",
            spec->format, spec->channels, sample_bits, spec->freq);
#endif /* DEBUG_AUDIO */

    header.numChannels = spec->channels;
    header.sampleSize = sample_bits;
    header.sampleRate = spec->freq << 16;
    header.numFrames = spec->samples;
    header.encode = cmpSH;

    /* Note that we install the 16bitLittleEndian Converter if needed. */
    if (spec->format == AUDIO_S16LSB) {
        header.compressionID = fixedCompression;
        header.format = k16BitLittleEndianFormat;
    } else if (spec->format == AUDIO_S32MSB) {
        header.compressionID = fixedCompression;
        header.format = k32BitFormat;
    } else if (spec->format == AUDIO_S32LSB) {
        header.compressionID = fixedCompression;
        header.format = k32BitLittleEndianFormat;
    } else if (spec->format == AUDIO_F32MSB) {
        header.compressionID = fixedCompression;
        header.format = kFloat32Format;
    }

    /* allocate 2 buffers */
    for (i = 0; i < 2; i++) {
        buffer[i] = (UInt8 *) SDL_malloc(sizeof(UInt8) * spec->size);
        if (buffer[i] == NULL) {
            SNDMGR_CloseDevice(this);
            SDL_OutOfMemory();
            return 0;
        }
        SDL_memset(buffer[i], 0, spec->size);
    }

    /* Create the sound manager channel */
    channel = (SndChannelPtr) SDL_malloc(sizeof(*channel));
    if (channel == NULL) {
        SNDMGR_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    this->hidden->channel = channel;
    if (spec->channels >= 2) {
        initOptions = initStereo;
    } else {
        initOptions = initMono;
    }
    channel->userInfo = (long) this;
    channel->qLength = 128;
    if (SndNewChannel(&channel, sampledSynth, initOptions, callback) != noErr) {
        SNDMGR_CloseDevice(this);
        SDL_SetError("Unable to create audio channel");
        return 0;
    }

    /* start playback */
    {
        SndCommand cmd;
        cmd.cmd = callBackCmd;
        cmd.param2 = 0;
        running = 1;
        SndDoCommand(channel, &cmd, 0);
    }

    return 1;
}

static int
SNDMGR_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = SNDMGR_OpenDevice;
    impl->CloseDevice = SNDMGR_CloseDevice;
    impl->ProvidesOwnCallbackThread = 1;
    impl->OnlyHasDefaultOutputDevice = 1;

/* Mac OS X uses threaded audio, so normal thread code is okay */
#ifndef __MACOSX__
    impl->LockDevice = SNDMGR_LockDevice;
    impl->UnlockDevice = SNDMGR_UnlockDevice;
    impl->SkipMixerLock = 1;
#endif

    return 1;
}

AudioBootStrap SNDMGR_bootstrap = {
    "sndmgr", SDL_MACOS_NAME " SoundManager", SNDMGR_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
