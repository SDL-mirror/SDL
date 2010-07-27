/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output audio to Android */

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_androidaudio.h"

#include <android/log.h>

static int
AndroidAUD_OpenDevice(_THIS, const char *devname, int iscapture)
{
    //TODO: Sample rates etc
    __android_log_print(ANDROID_LOG_INFO, "SDL", "AndroidAudio Open\n");
    
    return 1;
}

static void
AndroidAUD_PlayDevice(_THIS)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "AndroidAudio Play\n");
    
    

    //playGenericSound(this->hidden->mixbuf, this->hidden->mixlen);
    
#if 0
//    sound->data = this->hidden->mixbuf;/* pointer to raw audio data */
//    sound->len = this->hidden->mixlen; /* size of raw data pointed to above */
//    sound->rate = 22050; /* sample rate = 22050Hz */
//    sound->vol = 127;    /* volume [0..127] for [min..max] */
//    sound->pan = 64;     /* balance [0..127] for [left..right] */
//    sound->format = 0;   /* 0 for 16-bit, 1 for 8-bit */
//    playSound(sound);
#endif
}


static Uint8 *
AndroidAUD_GetDeviceBuf(_THIS)
{
    return this->hidden->mixbuf;        /* is this right? */
}

static void
AndroidAUD_WaitDevice(_THIS)
{
    /* stub */
}

static void
AndroidAUD_CloseDevice(_THIS)
{
    /* stub */
}

static int
AndroidAUD_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = AndroidAUD_OpenDevice;
    impl->PlayDevice = AndroidAUD_PlayDevice;
    impl->WaitDevice = AndroidAUD_WaitDevice;
    impl->GetDeviceBuf = AndroidAUD_GetDeviceBuf;
    impl->CloseDevice = AndroidAUD_CloseDevice;

    /* and the capabilities */
    impl->HasCaptureSupport = 0; //TODO
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->OnlyHasDefaultInputDevice = 1;

    __android_log_print(ANDROID_LOG_INFO, "SDL","Audio init\n");

    return 1;   /* this audio target is available. */
}

AudioBootStrap ANDROIDAUD_bootstrap = {
    "android", "SDL Android audio driver", AndroidAUD_Init, 0       /*1? */
};

/* vi: set ts=4 sw=4 expandtab: */
