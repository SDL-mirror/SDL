/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

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

#if SDL_AUDIO_DRIVER_AMIGAOS4

// This optimisation assumes that allocated audio buffers
// are sufficiently aligned to treat as arrays of longwords.
// Which they should be, as far as I can tell.
#define POSSIBLY_DANGEROUS_OPTIMISATION 1

#include "SDL_audio.h"
#include "SDL_timer.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"
#include "SDL_os4audio.h"

#include <proto/exec.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

/* The tag name used by the AmigaOS4 audio driver */
#define DRIVER_NAME         "amigaos4"

static SDL_bool
OS4_OpenAhiDevice(OS4AudioData * os4data)
{
    if (os4data->deviceOpen) {
        dprintf("Device already open\n");
    }

    os4data->deviceOpen = SDL_FALSE;

    os4data->ahiReplyPort = (struct MsgPort *)IExec->AllocSysObjectTags(ASOT_PORT, TAG_DONE);

    if (os4data->ahiReplyPort) {

        /* create a iorequest for the device */
        os4data->ahiRequest[0] = (struct AHIRequest *)
            IExec->AllocSysObjectTags(
                ASOT_IOREQUEST,
                ASOIOR_ReplyPort, os4data->ahiReplyPort,
                ASOIOR_Size,      sizeof(struct AHIRequest),
                TAG_DONE);

        if (os4data->ahiRequest[0]) {

            if (!IExec->OpenDevice(AHINAME, 0, (struct IORequest *)os4data->ahiRequest[0], 0)) {

                dprintf("%s opened\n", AHINAME);

                /* Create a copy */
                os4data->ahiRequest[1] = (struct AHIRequest *)
                    IExec->AllocSysObjectTags(
                        ASOT_IOREQUEST,
                        ASOIOR_Duplicate, os4data->ahiRequest[0],
                        TAG_DONE);

                if (os4data->ahiRequest[1]) {

                    dprintf("IO requests created\n");

                    os4data->deviceOpen = SDL_TRUE;
                    os4data->currentBuffer = 0;
                    os4data->link = NULL;
                } else {
                    dprintf("Failed to duplicate IO request\n");
                }
            } else {
                dprintf("Failed to open %s\n", AHINAME);
            }
        } else {
            dprintf("Failed to create IO request\n");
        }
    } else {
        dprintf("Failed to create reply port\n");
    }

    dprintf("deviceOpen = %d\n", os4data->deviceOpen);
    return os4data->deviceOpen;
}

static void
OS4_CloseAhiDevice(OS4AudioData * os4data)
{
    if (os4data->ahiRequest[0]) {
        if (os4data->link) {
            dprintf("Aborting I/O...\n");

            IExec->AbortIO((struct IORequest *)os4data->link);
            IExec->WaitIO((struct IORequest *)os4data->link);
        }

        dprintf("Closing device\n");
        IExec->CloseDevice((struct IORequest *)os4data->ahiRequest[0]);

        dprintf("Freeing I/O requests\n");
        IExec->FreeSysObject(ASOT_IOREQUEST, os4data->ahiRequest[0]);
        os4data->ahiRequest[0] = NULL;

        if (os4data->ahiRequest[1]) {
            IExec->FreeSysObject(ASOT_IOREQUEST, os4data->ahiRequest[1]);
            os4data->ahiRequest[1] = NULL;
        }
    }

    if (os4data->ahiReplyPort) {
        dprintf("Deleting message port\n");
        IExec->FreeSysObject(ASOT_PORT, os4data->ahiReplyPort);
        os4data->ahiReplyPort = NULL;
    }

    os4data->deviceOpen = SDL_FALSE;

    dprintf("Device closed\n");
}

static SDL_bool
OS4_AudioAvailable(void)
{
    SDL_bool isAvailable = SDL_FALSE;

    OS4AudioData *tempData = SDL_calloc(1, sizeof(OS4AudioData));

    if (!tempData) {
        dprintf("Failed to allocate temp data\n");
    } else {
        isAvailable = OS4_OpenAhiDevice(tempData);

        OS4_CloseAhiDevice(tempData);

        SDL_free(tempData);
    }

    dprintf("AHI is %savailable\n", isAvailable ? "" : "not ");
    return isAvailable;
}

static int
OS4_SwapBuffer(int current)
{
    return (1 - current);
}

static void
OS4_FillCaptureRequest(struct AHIRequest * request, void * buffer, int length, int frequency, int type)
{
    request->ahir_Std.io_Message.mn_Node.ln_Pri = 60;
    request->ahir_Std.io_Data    = buffer,
    request->ahir_Std.io_Length  = length;
    request->ahir_Std.io_Command = CMD_READ;
    request->ahir_Volume         = 0x10000;
    request->ahir_Position       = 0x8000;
    request->ahir_Link           = NULL;
    request->ahir_Frequency      = frequency;
    request->ahir_Type           = type;
}

/* ---------------------------------------------- */
/* Audio driver exported functions implementation */
/* ---------------------------------------------- */
static void
OS4_CloseDevice(_THIS)
{
    OS4AudioData *os4data = _this->hidden;

    dprintf("Called\n");

    OS4_CloseAhiDevice(os4data);

    if (os4data->audioBuffer[0]) {
        SDL_free(os4data->audioBuffer[0]);
        os4data->audioBuffer[0] = NULL;
    }

    if (os4data->audioBuffer[1]) {
        SDL_free(os4data->audioBuffer[1]);
        os4data->audioBuffer[1] = NULL;
    }

    SDL_free(os4data);
}

static int
OS4_OpenDevice(_THIS, void * handle, const char * devname, int iscapture)
{
    int result = 0;
    OS4AudioData *os4data = NULL;

    dprintf("handle %p, devname %s, iscapture %d\n", handle, devname, iscapture);

    _this->hidden = (OS4AudioData *) SDL_malloc(sizeof(OS4AudioData));

    if (!_this->hidden) {
        dprintf("Failed to allocate private data\n");
        return SDL_OutOfMemory();
    }

    SDL_memset(_this->hidden, 0, sizeof(OS4AudioData));
    os4data = _this->hidden;

    if ((_this->spec.format & 0xff) != 8) {
        _this->spec.format = AUDIO_S16MSB;
    }

    dprintf("New format = 0x%x\n", _this->spec.format);
    dprintf("Buffer size = %d\n", _this->spec.size);

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&_this->spec);

    os4data->audioBufferSize = _this->spec.size;
    os4data->audioBuffer[0] = (Uint8 *) SDL_malloc(_this->spec.size);
    os4data->audioBuffer[1] = (Uint8 *) SDL_malloc(_this->spec.size);

    if (os4data->audioBuffer[0] == NULL || os4data->audioBuffer[1] == NULL) {
        OS4_CloseDevice(_this);
        dprintf("No memory for audio buffer\n");
        SDL_SetError("No memory for audio buffer");
        return -1;
    }

    SDL_memset(os4data->audioBuffer[0], _this->spec.silence, _this->spec.size);
    SDL_memset(os4data->audioBuffer[1], _this->spec.silence, _this->spec.size);

    switch(_this->spec.format) {
        case AUDIO_S8:
        case AUDIO_U8:
            os4data->ahiType = (_this->spec.channels < 2) ? AHIST_M8S : AHIST_S8S;
            break;

        default:
            os4data->ahiType = (_this->spec.channels < 2) ? AHIST_M16S : AHIST_S16S;
            break;
    }

    return result;
}

static void
OS4_ThreadInit(_THIS)
{
    OS4AudioData *os4data = _this->hidden;

    dprintf("Called\n");

    /* Signal must be opened in the task which is using it (player) */
    if (!OS4_OpenAhiDevice(os4data)) {
        // FIXME: this is bad. We have failed and SDL core doesn't know about that.
        dprintf("Failed to open AHI\n");
    }

    /* This will cause a lot of problems.. and should be removed.

    One possibility: create a configuration GUI or ENV variable that allows
    user to select priority, if there is no silver bullet value */
    IExec->SetTaskPri(IExec->FindTask(NULL), 5);
}

static void
OS4_WaitDevice(_THIS)
{
    /* Dummy - OS4_PlayDevice handles the waiting */
    //dprintf("Called\n");
}

static void
OS4_PlayDevice(_THIS)
{
    struct AHIRequest  *ahiRequest;
    SDL_AudioSpec      *spec    = &_this->spec;
    OS4AudioData       *os4data = _this->hidden;
    int                 current = os4data->currentBuffer;

    //dprintf("Called\n");

    if (!os4data->deviceOpen) {
        dprintf("Device is not open\n");
        return;
    }

    ahiRequest = os4data->ahiRequest[current];

    ahiRequest->ahir_Std.io_Message.mn_Node.ln_Pri = 60;
    ahiRequest->ahir_Std.io_Data    = os4data->audioBuffer[current];
    ahiRequest->ahir_Std.io_Length  = os4data->audioBufferSize;
    ahiRequest->ahir_Std.io_Offset  = 0;
    ahiRequest->ahir_Std.io_Command = CMD_WRITE;
    ahiRequest->ahir_Volume         = 0x10000;
    ahiRequest->ahir_Position       = 0x8000;
    ahiRequest->ahir_Link           = os4data->link;
    ahiRequest->ahir_Frequency      = spec->freq;
    ahiRequest->ahir_Type           = os4data->ahiType;

    // Convert to signed?
    if (spec->format == AUDIO_U8) {
#if POSSIBLY_DANGEROUS_OPTIMISATION
        int i, n;
        uint32 *mixbuf = (uint32 *)os4data->audioBuffer[current];
        n = os4data->audioBufferSize / 4; // let the gcc optimiser decide the best way to divide by 4
        for (i = 0; i < n; i++) {
            *(mixbuf++) ^= 0x80808080;
        }

        if (0 != (n = os4data->audioBufferSize & 3)) {
            uint8 *mixbuf8 = (uint8 *)mixbuf;
            for (i = 0; i < n; i++) {
                *(mixbuf8++) -= 128;
            }
        }
#else
        int i;
        for (i = 0; i < os4data->audioBufferSize; i++) {
            os4data->audioBuffer[current][i] -= 128;
        }
#endif
    }

    IExec->SendIO((struct IORequest *)ahiRequest);

    if (os4data->link) {
        IExec->WaitIO((struct IORequest *)os4data->link);
    }

    os4data->link = ahiRequest;
    os4data->currentBuffer = OS4_SwapBuffer(current);
}

static Uint8 *
OS4_GetDeviceBuf(_THIS)
{
    //dprintf("Called\n");

    return _this->hidden->audioBuffer[_this->hidden->currentBuffer];
}

#ifndef MIN
#define MIN(a, b) (a) < (b) ? (a) : (b)
#endif

#define RESTART_CAPTURE_THRESHOLD 500

static int
OS4_CaptureFromDevice(_THIS, void * buffer, int buflen)
{
    struct AHIRequest  *request;
    SDL_AudioSpec      *spec    = &_this->spec;
    OS4AudioData       *os4data = _this->hidden;
    Uint32 now;
    size_t copyLen;
    void *completedBuffer;
    int current;

    //dprintf("Called %p, %d\n", buffer, buflen);

    if (!os4data->deviceOpen) {
        dprintf("Device is not open\n");
        return 0;
    }

    now = SDL_GetTicks();
    current = os4data->currentBuffer;

    request = os4data->ahiRequest[0];

    if ((now - os4data->lastCaptureTicks) > RESTART_CAPTURE_THRESHOLD) {

        if (os4data->requestSent) {
            IExec->WaitIO((struct IORequest *)request);
        }

        /* Assume that we have to (re)start recording */
        OS4_FillCaptureRequest(
            request,
            os4data->audioBuffer[current],
            os4data->audioBufferSize,
            spec->freq,
            os4data->ahiType);

        request->ahir_Std.io_Offset = 0;

        dprintf("Start recording\n");

        IExec->DoIO((struct IORequest *)request);
        os4data->requestSent = SDL_FALSE;

        current = OS4_SwapBuffer(current);
    } else {
        /* Wait for the previous request completion */
        IExec->WaitIO((struct IORequest *)request);
    }

    OS4_FillCaptureRequest(
        request,
        os4data->audioBuffer[current],
        os4data->audioBufferSize,
        spec->freq,
        os4data->ahiType);

    IExec->SendIO((struct IORequest *)request);
    os4data->requestSent = SDL_TRUE;

    current = OS4_SwapBuffer(current);

    completedBuffer = os4data->audioBuffer[current];

    copyLen = MIN(buflen, os4data->audioBufferSize);

    SDL_memcpy(buffer, completedBuffer, copyLen);

    os4data->lastCaptureTicks = now;
    os4data->currentBuffer = current;

    //dprintf("%d bytes copied\n", copyLen);

    return copyLen;
}

/* ------------------------------------------ */
/* Audio driver init functions implementation */
/* ------------------------------------------ */
static int
OS4_Init(SDL_AudioDriverImpl * impl)
{
    if (!OS4_AudioAvailable()) {
        SDL_SetError("Failed to open AHI device");
        return 0;
    }

    // impl->DetectDevices?
    impl->OpenDevice = OS4_OpenDevice;
    impl->ThreadInit = OS4_ThreadInit;
    impl->WaitDevice = OS4_WaitDevice;
    impl->PlayDevice = OS4_PlayDevice;
    // impl->GetPendingBytes?
    impl->GetDeviceBuf = OS4_GetDeviceBuf;
    impl->CaptureFromDevice = OS4_CaptureFromDevice;
    // impl->FlushCapture
    // impl->PrepareToClose()
    impl->CloseDevice = OS4_CloseDevice;
    // impl->Lock+UnlockDevice
    // impl->FreeDeviceHandle
    // impl->Deinitialize

    impl->HasCaptureSupport = 1;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->OnlyHasDefaultCaptureDevice = 1;

    return 1;
}

AudioBootStrap AMIGAOS4AUDIO_bootstrap = {
   DRIVER_NAME, "AmigaOS4 AHI audio", OS4_Init, 0
};
#endif
