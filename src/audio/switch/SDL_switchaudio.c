/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_AUDIO_DRIVER_SWITCH

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"

#include "SDL_switchaudio.h"

static int
SWITCHAUDIO_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    Result res = audoutInitialize();
    if (res != 0) {
        return SDL_SetError("audoutInitialize failed (0x%x)", res);
    }

    res = audoutStartAudioOut();
    if (res != 0) {
        audoutExit();
        return SDL_SetError("audoutStartAudioOut failed (0x%x)", res);
    }

    this->hidden = (struct SDL_PrivateAudioData *) SDL_malloc(sizeof(*this->hidden));
    if (this->hidden == NULL) {
        return SDL_OutOfMemory();
    }
    SDL_zerop(this->hidden);

    switch (this->spec.format & 0xff) {
        case 8:
        case 16:
            this->spec.format = AUDIO_S16LSB;
            break;
        default:
            return SDL_SetError("Unsupported audio format");
    }

    this->spec.freq = 48000;
    this->spec.channels = 2;

    SDL_CalculateAudioSpec(&this->spec);

    for (int i = 0; i < 2; i++) {
        u32 size = (u32) (this->spec.size + 0xfff) & ~0xfff;
        this->hidden->buffer[i] = memalign(0x1000, size);
        memset(this->hidden->buffer[i], 0, size);
        this->hidden->source_buffer[i].next = NULL;
        this->hidden->source_buffer[i].buffer = this->hidden->buffer[i];
        this->hidden->source_buffer[i].buffer_size =
            (u64) this->spec.size / this->spec.channels / 4;
        this->hidden->source_buffer[i].data_size = (u64) this->spec.size;
        this->hidden->source_buffer[i].data_offset = (u64) 0;
        audoutAppendAudioOutBuffer(&this->hidden->source_buffer[i]);
    }

    return 0;
}

static void
SWITCHAUDIO_PlayDevice(_THIS)
{
    audoutAppendAudioOutBuffer(this->hidden->released_buffer);
}

static void
SWITCHAUDIO_WaitDevice(_THIS)
{

}

static Uint8
*SWITCHAUDIO_GetDeviceBuf(_THIS)
{
    audoutWaitPlayFinish(&this->hidden->released_buffer,
                         &this->hidden->released_count, U64_MAX);

    return this->hidden->released_buffer->buffer;
}

static void
SWITCHAUDIO_CloseDevice(_THIS)
{
    if (this->hidden->buffer[0]) {
        free(this->hidden->buffer[0]);
    }
    if (this->hidden->buffer[1]) {
        free(this->hidden->buffer[1]);
    }

    audoutStopAudioOut();
    audoutExit();

    SDL_free(this->hidden);
}

static void
SWITCHAUDIO_ThreadInit(_THIS)
{

}

static int
SWITCHAUDIO_Init(SDL_AudioDriverImpl *impl)
{
    impl->OpenDevice = SWITCHAUDIO_OpenDevice;
    impl->PlayDevice = SWITCHAUDIO_PlayDevice;
    impl->WaitDevice = SWITCHAUDIO_WaitDevice;
    impl->GetDeviceBuf = SWITCHAUDIO_GetDeviceBuf;
    impl->CloseDevice = SWITCHAUDIO_CloseDevice;
    impl->ThreadInit = SWITCHAUDIO_ThreadInit;

    impl->OnlyHasDefaultOutputDevice = 1;

    return 1;
}

AudioBootStrap SWITCHAUDIO_bootstrap = {
    "switch", "Nintendo Switch audio driver", SWITCHAUDIO_Init, 0
};

#endif /* SDL_AUDIO_DRIVER_SWITCH */
