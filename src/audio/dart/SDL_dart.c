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

/* Allow access to a raw mixing buffer */

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_dart.h"

// Buffer states:
#define BUFFER_EMPTY       0
#define BUFFER_USED        1

typedef struct _tMixBufferDesc
{
    int iBufferUsage;           // BUFFER_EMPTY or BUFFER_USED
    SDL_AudioDevice *pSDLAudioDevice;
} tMixBufferDesc, *pMixBufferDesc;


//---------------------------------------------------------------------
// DARTEventFunc
//
// This function is called by DART, when an event occurs, like end of
// playback of a buffer, etc...
//---------------------------------------------------------------------
static LONG APIENTRY
DARTEventFunc(ULONG ulStatus, PMCI_MIX_BUFFER pBuffer, ULONG ulFlags)
{
    if (ulFlags && MIX_WRITE_COMPLETE) {        // Playback of buffer completed!

        // Get pointer to buffer description
        pMixBufferDesc pBufDesc;

        if (pBuffer) {
            pBufDesc = (pMixBufferDesc) (*pBuffer).ulUserParm;

            if (pBufDesc) {
                SDL_AudioDevice *pSDLAudioDevice = pBufDesc->pSDLAudioDevice;
                // Set the buffer to be empty
                pBufDesc->iBufferUsage = BUFFER_EMPTY;
                // And notify DART feeder thread that it will have to work a bit.
                if (pSDLAudioDevice)
                    DosPostEventSem(pSDLAudioDevice->hidden->
                                    hevAudioBufferPlayed);
            }
        }
    }
    return TRUE;
}


static int
DART_OpenDevice(_THIS, const char *devname, int iscapture)
{
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(_this->spec.format);
    int valid_datatype = 0;
    MCI_AMP_OPEN_PARMS AmpOpenParms;
    int iDeviceOrd = 0;         // Default device to be used
    int bOpenShared = 1;        // Try opening it shared
    int iBits = 16;             // Default is 16 bits signed
    int iFreq = 44100;          // Default is 44KHz
    int iChannels = 2;          // Default is 2 channels (Stereo)
    int iNumBufs = 2;           // Number of audio buffers: 2
    int iBufSize;
    int iOpenMode;
    int iSilence;
    int rc;

    /* Initialize all variables that we clean on shutdown */
    _this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *_this->hidden));
    if (_this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(_this->hidden, 0, (sizeof *_this->hidden));

    // First thing is to try to open a given DART device!
    SDL_memset(&AmpOpenParms, 0, sizeof(MCI_AMP_OPEN_PARMS));
    // pszDeviceType should contain the device type in low word, and device ordinal in high word!
    AmpOpenParms.pszDeviceType =
        (PSZ) (MCI_DEVTYPE_AUDIO_AMPMIX | (iDeviceOrd << 16));

    iOpenMode = MCI_WAIT | MCI_OPEN_TYPE_ID;
    if (bOpenShared)
        iOpenMode |= MCI_OPEN_SHAREABLE;

    rc = mciSendCommand(0, MCI_OPEN, iOpenMode, (PVOID) & AmpOpenParms, 0);
    if (rc != MCIERR_SUCCESS) { // No audio available??
        DART_CloseDevice(_this);
        SDL_SetError("DART: Couldn't open audio device.");
        return 0;
    }
    // Save the device ID we got from DART!
    // We will use this in the next calls!
    _this->hidden->iCurrDeviceOrd = iDeviceOrd = AmpOpenParms.usDeviceID;

    // Determine the audio parameters from the AudioSpec
    if (_this->spec.channels > 4)
        _this->spec.channels = 4;

    while ((!valid_datatype) && (test_format)) {
        _this->spec.format = test_format;
        valid_datatype = 1;
        switch (test_format) {
        case AUDIO_U8:
            // Unsigned 8 bit audio data
            iSilence = 0x80;
            _this->hidden->iCurrBits = iBits = 8;
            break;

        case AUDIO_S16LSB:
            // Signed 16 bit audio data
            iSilence = 0x00;
            _this->hidden->iCurrBits = iBits = 16;
            break;

            // !!! FIXME: int32?

        default:
            valid_datatype = 0;
            test_format = SDL_NextAudioFormat();
            break;
        }
    }

    if (!valid_datatype) {      // shouldn't happen, but just in case...
        // Close DART, and exit with error code!
        DART_CloseDevice(_this);
        SDL_SetError("Unsupported audio format");
        return 0;
    }

    _this->hidden->iCurrFreq = iFreq = _this->spec.freq;
    _this->hidden->iCurrChannels = iChannels = _this->spec.channels;
    /* Update the fragment size as size in bytes */
    SDL_CalculateAudioSpec(&_this->spec);
    _this->hidden->iCurrBufSize = iBufSize = _this->spec.size;

    // Now query this device if it supports the given freq/bits/channels!
    SDL_memset(&(_this->hidden->MixSetupParms), 0,
               sizeof(MCI_MIXSETUP_PARMS));
    _this->hidden->MixSetupParms.ulBitsPerSample = iBits;
    _this->hidden->MixSetupParms.ulFormatTag = MCI_WAVE_FORMAT_PCM;
    _this->hidden->MixSetupParms.ulSamplesPerSec = iFreq;
    _this->hidden->MixSetupParms.ulChannels = iChannels;
    _this->hidden->MixSetupParms.ulFormatMode = MCI_PLAY;
    _this->hidden->MixSetupParms.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
    _this->hidden->MixSetupParms.pmixEvent = DARTEventFunc;
    rc = mciSendCommand(iDeviceOrd, MCI_MIXSETUP,
                        MCI_WAIT | MCI_MIXSETUP_QUERYMODE,
                        &(_this->hidden->MixSetupParms), 0);
    if (rc != MCIERR_SUCCESS) { // The device cannot handle this format!
        // Close DART, and exit with error code!
        DART_CloseDevice(_this);
        SDL_SetError("Audio device doesn't support requested audio format");
        return 0;
    }
    // The device can handle this format, so initialize!
    rc = mciSendCommand(iDeviceOrd, MCI_MIXSETUP,
                        MCI_WAIT | MCI_MIXSETUP_INIT,
                        &(_this->hidden->MixSetupParms), 0);
    if (rc != MCIERR_SUCCESS) { // The device could not be opened!
        // Close DART, and exit with error code!
        DART_CloseDevice(_this);
        SDL_SetError("Audio device could not be set up");
        return 0;
    }
    // Ok, the device is initialized.
    // Now we should allocate buffers. For this, we need a place where
    // the buffer descriptors will be:
    _this->hidden->pMixBuffers =
        (MCI_MIX_BUFFER *) SDL_malloc(sizeof(MCI_MIX_BUFFER) * iNumBufs);
    if (!(_this->hidden->pMixBuffers)) {        // Not enough memory!
        // Close DART, and exit with error code!
        DART_CloseDevice(_this);
        SDL_OutOfMemory();
        return 0;
    }
    // Now that we have the place for buffer list, we can ask DART for the
    // buffers!
    _this->hidden->BufferParms.ulNumBuffers = iNumBufs; // Number of buffers
    _this->hidden->BufferParms.ulBufferSize = iBufSize; // each with this size
    _this->hidden->BufferParms.pBufList = _this->hidden->pMixBuffers;   // getting descriptorts into this list
    // Allocate buffers!
    rc = mciSendCommand(iDeviceOrd, MCI_BUFFER,
                        MCI_WAIT | MCI_ALLOCATE_MEMORY,
                        &(_this->hidden->BufferParms), 0);
    if ((rc != MCIERR_SUCCESS)
        || (iNumBufs != _this->hidden->BufferParms.ulNumBuffers)
        || (_this->hidden->BufferParms.ulBufferSize == 0)) {    // Could not allocate memory!
        // Close DART, and exit with error code!
        DART_CloseDevice(_this);
        SDL_SetError("DART could not allocate buffers");
        return 0;
    }
    _this->hidden->iCurrNumBufs = iNumBufs;

    // Ok, we have all the buffers allocated, let's mark them!
    {
        int i;
        for (i = 0; i < iNumBufs; i++) {
            pMixBufferDesc pBufferDesc =
                (pMixBufferDesc) SDL_malloc(sizeof(tMixBufferDesc));;
            // Check if this buffer was really allocated by DART
            if ((!(_this->hidden->pMixBuffers[i].pBuffer))
                || (!pBufferDesc)) {    // Wrong buffer!
                DART_CloseDevice(_this);
                SDL_SetError("Error at internal buffer check");
                return 0;
            }
            pBufferDesc->iBufferUsage = BUFFER_EMPTY;
            pBufferDesc->pSDLAudioDevice = _this;

            _this->hidden->pMixBuffers[i].ulBufferLength =
                _this->hidden->BufferParms.ulBufferSize;
            _this->hidden->pMixBuffers[i].ulUserParm = (ULONG) pBufferDesc;     // User parameter: Description of buffer
            _this->hidden->pMixBuffers[i].ulFlags = 0;  // Some stuff should be flagged here for DART, like end of
            // audio data, but as we will continously send
            // audio data, there will be no end.:)
            SDL_memset(_this->hidden->pMixBuffers[i].pBuffer, iSilence,
                       iBufSize);
        }
    }
    _this->hidden->iNextFreeBuffer = 0;
    _this->hidden->iLastPlayedBuf = -1;
    // Create event semaphore
    if (DosCreateEventSem
        (NULL, &(_this->hidden->hevAudioBufferPlayed), 0, FALSE) != NO_ERROR)
    {
        DART_CloseDevice(_this);
        SDL_SetError("Could not create event semaphore");
        return 0;
    }

    return 1;
}

static void
DART_ThreadInit(_THIS)
{
    /* Increase the priority of this thread to make sure that
       the audio will be continuous all the time! */
#ifdef USE_DOSSETPRIORITY
    if (SDL_getenv("SDL_USE_TIMECRITICAL_AUDIO")) {
#ifdef DEBUG_BUILD
        printf
            ("[DART_ThreadInit] : Setting priority to TimeCritical+0! (TID%d)\n",
             SDL_ThreadID());
#endif
        DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0);
    } else {
#ifdef DEBUG_BUILD
        printf
            ("[DART_ThreadInit] : Setting priority to ForegroundServer+0! (TID%d)\n",
             SDL_ThreadID());
#endif
        DosSetPriority(PRTYS_THREAD, PRTYC_FOREGROUNDSERVER, 0, 0);
    }
#endif
}

/* This function waits until it is possible to write a full sound buffer */
static void
DART_WaitDevice(_THIS)
{
    int i;
    pMixBufferDesc pBufDesc;
    ULONG ulPostCount;

    DosResetEventSem(_this->hidden->hevAudioBufferPlayed, &ulPostCount);
    // If there is already an empty buffer, then return now!
    for (i = 0; i < _this->hidden->iCurrNumBufs; i++) {
        pBufDesc = (pMixBufferDesc) _this->hidden->pMixBuffers[i].ulUserParm;
        if (pBufDesc->iBufferUsage == BUFFER_EMPTY)
            return;
    }
    // If there is no empty buffer, wait for one to be empty!
    DosWaitEventSem(_this->hidden->hevAudioBufferPlayed, 1000); // Wait max 1 sec!!! Important!
    return;
}

static void
DART_PlayDevice(_THIS)
{
    int iFreeBuf = _this->hidden->iNextFreeBuffer;
    pMixBufferDesc pBufDesc;

    pBufDesc =
        (pMixBufferDesc) _this->hidden->pMixBuffers[iFreeBuf].ulUserParm;
    pBufDesc->iBufferUsage = BUFFER_USED;
    // Send it to DART to be queued
    _this->hidden->MixSetupParms.pmixWrite(_this->hidden->MixSetupParms.
                                           ulMixHandle,
                                           &(_this->hidden->
                                             pMixBuffers[iFreeBuf]), 1);

    _this->hidden->iLastPlayedBuf = iFreeBuf;
    iFreeBuf = (iFreeBuf + 1) % _this->hidden->iCurrNumBufs;
    _this->hidden->iNextFreeBuffer = iFreeBuf;
}

static Uint8 *
DART_GetDeviceBuf(_THIS)
{
    int iFreeBuf;
    Uint8 *pResult;
    pMixBufferDesc pBufDesc;

    if (_this) {
        if (_this->hidden) {
            iFreeBuf = _this->hidden->iNextFreeBuffer;
            pBufDesc =
                (pMixBufferDesc) _this->hidden->pMixBuffers[iFreeBuf].
                ulUserParm;

            if (pBufDesc) {
                if (pBufDesc->iBufferUsage == BUFFER_EMPTY) {
                    pResult = _this->hidden->pMixBuffers[iFreeBuf].pBuffer;
                    return pResult;
                }
            } else
                printf("[DART_GetDeviceBuf] : ERROR! pBufDesc = %p\n",
                       pBufDesc);
        } else
            printf("[DART_GetDeviceBuf] : ERROR! _this->hidden = %p\n",
                   _this->hidden);
    } else
        printf("[DART_GetDeviceBuf] : ERROR! _this = %p\n", _this);
    return NULL;
}

static void
DART_WaitDone(_THIS)
{
    pMixBufferDesc pBufDesc;
    ULONG ulPostCount = 0;
    APIRET rc = NO_ERROR;

    pBufDesc = (pMixBufferDesc)
        _this->hidden->pMixBuffers[_this->hidden->iLastPlayedBuf].ulUserParm;

    while ((pBufDesc->iBufferUsage != BUFFER_EMPTY) && (rc == NO_ERROR)) {
        DosResetEventSem(_this->hidden->hevAudioBufferPlayed, &ulPostCount);
        rc = DosWaitEventSem(_this->hidden->hevAudioBufferPlayed, 1000);        // 1 sec timeout! Important!
    }
}

static void
DART_CloseDevice(_THIS)
{
    MCI_GENERIC_PARMS GenericParms;
    int rc;
    int i;

    if (_this->hidden != NULL) {
        // Stop DART playback
        if (_this->hidden->iCurrDeviceOrd) {
            rc = mciSendCommand(_this->hidden->iCurrDeviceOrd, MCI_STOP,
                                MCI_WAIT, &GenericParms, 0);
#ifdef SFX_DEBUG_BUILD
            if (rc != MCIERR_SUCCESS) {
                printf("Could not stop DART playback!\n");
                fflush(stdout);
            }
#endif
        }
        // Close event semaphore
        if (_this->hidden->hevAudioBufferPlayed) {
            DosCloseEventSem(_this->hidden->hevAudioBufferPlayed);
            _this->hidden->hevAudioBufferPlayed = 0;
        }
        // Free memory of buffer descriptions
        for (i = 0; i < _this->hidden->iCurrNumBufs; i++) {
            SDL_free((void *) (_this->hidden->pMixBuffers[i].ulUserParm));
            _this->hidden->pMixBuffers[i].ulUserParm = 0;
        }
        _this->hidden->iCurrNumBufs = 0;

        // Deallocate buffers
        if (_this->hidden->iCurrDeviceOrd) {
            rc = mciSendCommand(_this->hidden->iCurrDeviceOrd, MCI_BUFFER,
                                MCI_WAIT | MCI_DEALLOCATE_MEMORY,
                                &(_this->hidden->BufferParms), 0);
        }
        // Free bufferlist
        if (_this->hidden->pMixBuffers != NULL) {
            SDL_free(_this->hidden->pMixBuffers);
            _this->hidden->pMixBuffers = NULL;
        }
        // Close dart
        if (_this->hidden->iCurrDeviceOrd) {
            rc = mciSendCommand(_this->hidden->iCurrDeviceOrd, MCI_CLOSE,
                                MCI_WAIT, &(GenericParms), 0);
        }
        _this->hidden->iCurrDeviceOrd = 0;

        SDL_free(_this->hidden);
        _this->hidden = NULL;
    }
}


static int
DART_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = DART_OpenDevice;
    impl->ThreadInit = DART_ThreadInit;
    impl->WaitDevice = DART_WaitDevice;
    impl->GetDeviceBuf = DART_GetDeviceBuf;
    impl->PlayDevice = DART_PlayDevice;
    impl->WaitDone = DART_WaitDone;
    impl->CloseDevice = DART_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = 1;       /* !!! FIXME: is this right? */

    return 1;
}


AudioBootStrap DART_bootstrap = {
    "dart", "OS/2 Direct Audio RouTines (DART)", DART_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
