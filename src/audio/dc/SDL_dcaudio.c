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

/* Output dreamcast aica */

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "SDL_dcaudio.h"

#include "aica.h"
#include <dc/spu.h>

#define	SPU_RAM_BASE	0xa0800000

static void
spu_memload_stereo8(int leftpos, int rightpos, void *src0, size_t size)
{
    uint8 *src = src0;
    uint32 *left = (uint32 *) (leftpos + SPU_RAM_BASE);
    uint32 *right = (uint32 *) (rightpos + SPU_RAM_BASE);
    size = (size + 7) / 8;
    while (size--) {
        unsigned lval, rval;
        lval = *src++;
        rval = *src++;
        lval |= (*src++) << 8;
        rval |= (*src++) << 8;
        lval |= (*src++) << 16;
        rval |= (*src++) << 16;
        lval |= (*src++) << 24;
        rval |= (*src++) << 24;
        g2_write_32(left++, lval);
        g2_write_32(right++, rval);
        g2_fifo_wait();
    }
}

static void
spu_memload_stereo16(int leftpos, int rightpos, void *src0, size_t size)
{
    uint16 *src = src0;
    uint32 *left = (uint32 *) (leftpos + SPU_RAM_BASE);
    uint32 *right = (uint32 *) (rightpos + SPU_RAM_BASE);
    size = (size + 7) / 8;
    while (size--) {
        unsigned lval, rval;
        lval = *src++;
        rval = *src++;
        lval |= (*src++) << 16;
        rval |= (*src++) << 16;
        g2_write_32(left++, lval);
        g2_write_32(right++, rval);
        g2_fifo_wait();
    }
}

static void
DCAUD_PlayDevice(_THIS)
{
    SDL_AudioSpec *spec = &this->spec;
    unsigned int offset;

    if (this->hidden->playing) {
        /* wait */
        while (aica_get_pos(0) / spec->samples == this->hidden->nextbuf) {
            thd_pass();
        }
    }

    offset = this->hidden->nextbuf * spec->size;
    this->hidden->nextbuf ^= 1;
    /* Write the audio data, checking for EAGAIN on broken audio drivers */
    if (spec->channels == 1) {
        spu_memload(this->hidden->leftpos + offset, this->hidden->mixbuf,
                    this->hidden->mixlen);
    } else {
        offset /= 2;
        if ((this->spec.format & 255) == 8) {
            spu_memload_stereo8(this->hidden->leftpos + offset,
                                this->hidden->rightpos + offset,
                                this->hidden->mixbuf, this->hidden->mixlen);
        } else {
            spu_memload_stereo16(this->hidden->leftpos + offset,
                                 this->hidden->rightpos + offset,
                                 this->hidden->mixbuf, this->hidden->mixlen);
        }
    }

    if (!this->hidden->playing) {
        int mode;
        this->hidden->playing = 1;
        mode = (spec->format == AUDIO_S8) ? SM_8BIT : SM_16BIT;
        if (spec->channels == 1) {
            aica_play(0, mode, this->hidden->leftpos, 0,
                      spec->samples * 2, spec->freq, 255, 128, 1);
        } else {
            aica_play(0, mode, this->hidden->leftpos, 0,
                      spec->samples * 2, spec->freq, 255, 0, 1);
            aica_play(1, mode, this->hidden->rightpos, 0,
                      spec->samples * 2, spec->freq, 255, 255, 1);
        }
    }
}

static Uint8 *
DCAUD_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

/* This function waits until it is possible to write a full sound buffer */
static void
DCAUD_WaitDevice(_THIS)
{
    if (this->hidden->playing) {
        /* wait */
        while (aica_get_pos(0) / this->spec.samples == this->hidden->nextbuf) {
            thd_pass();
        }
    }
}

static void
DCAUD_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        aica_stop(0);
        if (this->spec.channels == 2) {
            aica_stop(1);
        }
        if (this->hidden->mixbuf != NULL) {
            SDL_FreeAudioMem(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }
        SDL_free(this->hidden);
        this->hidden = NULL;

        /* !!! FIXME: is there a reverse of spu_init()? */
    }
}

static int
DCAUD_OpenDevice(_THIS, SDL_AudioSpec * spec)
{
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(spec->format);
    int valid_datatype = 0;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    spu_init();

    while ((!valid_datatype) && (test_format)) {
        spec->format = test_format;
        switch (test_format) {
            /* only formats Dreamcast accepts... */
        case AUDIO_S8:
        case AUDIO_S16LSB:
            valid_datatype = 1;
            break;

        default:
            test_format = SDL_NextAudioFormat();
            break;
        }
    }

    if (!valid_datatype) {      /* shouldn't happen, but just in case... */
        DCAUD_CloseDevice(this);
        SDL_SetError("Unsupported audio format");
        return 0;
    }

    if (spec->channels > 2)
        spec->channels = 2;     /* no more than stereo on the Dreamcast. */

    /* Update the fragment size as size in bytes */
    SDL_CalculateAudioSpec(spec);

    /* Allocate mixing buffer */
    this->hidden->mixlen = spec->size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if (this->hidden->mixbuf == NULL) {
        DCAUD_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);
    this->hidden->leftpos = 0x11000;
    this->hidden->rightpos = 0x11000 + spec->size;
    this->hidden->playing = 0;
    this->hidden->nextbuf = 0;

    /* We're ready to rock and roll. :-) */
    return 1;
}

static int
DCAUD_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = DCAUD_OpenDevice;
    impl->PlayDevice = DCAUD_PlayDevice;
    impl->WaitDevice = DCAUD_WaitDevice;
    impl->GetDeviceBuf = DCAUD_GetDeviceBuf;
    impl->CloseDevice = DCAUD_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = 1;

    return 1;
}

AudioBootStrap DCAUD_bootstrap = {
    "dcaudio", "Dreamcast AICA audio", DCAUD_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
