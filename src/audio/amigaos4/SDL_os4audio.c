/*
    AmigaOS4 support for the SDL - Simple DirectMedia Layer
    Copyright (C) 2004  Jrgen Schober

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

    Juergen Schober
    juergen.schober@pointdesign.com
*/
#if SDL_AUDIO_DRIVER_AMIGAOS4
// This optimisation assumes that allocated audio buffers
// are sufficiently aligned to treat as arrays of longwords.
// Which they should be, as far as I can tell.
#define POSSIBLY_DANGEROUS_OPTIMISATION 1

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiomem.h"
#include "../SDL_sysaudio.h"
#include "SDL_os4audio.h"

#include <dos/dos.h>
#include <proto/exec.h>

#undef DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"


/* The tag name used by the AmigaOS4 audio driver */
#define DRIVER_NAME         "amigaos4"

static int OS4_OpenAhiDevice(OS4AudioData *os4data)
{
    int ahi_open = 0;

    /* create our reply port */
    os4data->ahi_ReplyPort = (struct MsgPort*)IExec->AllocSysObjectTags( ASOT_PORT, TAG_DONE );
    if (os4data->ahi_ReplyPort)
    {
        /* create a iorequest for the device */
        os4data->ahi_IORequest[0] = (struct AHIRequest*)
            IExec->AllocSysObjectTags( ASOT_IOREQUEST,
                ASOIOR_ReplyPort, os4data->ahi_ReplyPort,
                ASOIOR_Size,      sizeof( struct AHIRequest ),
                TAG_DONE );
        if (os4data->ahi_IORequest[0])
        {
            /* open the device */
            if (!IExec->OpenDevice(AHINAME, 0, (struct IORequest*)os4data->ahi_IORequest[0], 0))
            {
                dprintf("Device open\n");

                /* Create a copy */
                os4data->ahi_IORequest[1] = (struct AHIRequest *)
                    IExec->AllocSysObjectTags( ASOT_IOREQUEST,
                        ASOIOR_Duplicate, os4data->ahi_IORequest[0],
                        TAG_DONE );
                if (os4data->ahi_IORequest[1])
                {
//                    IExec->CopyMem(os4data->ahi_IORequest[0], os4data->ahi_IORequest[1], sizeof(struct AHIRequest));

                    /* create our os4 interface */
                    os4data->ahi_IFace = (struct AHIIFace*)
                        IExec->GetInterface((struct Library*)os4data->ahi_IORequest[0]->ahir_Std.io_Device, "main", 1, NULL);
                    if (os4data->ahi_IFace)
                    {
                        dprintf("Got Interface\n");
                        ahi_open = 1;
                        os4data->currentBuffer = 0;
                        os4data->link = 0;
                    }
                }
            }
        }
    }

    dprintf("ahi_open = %d\n", ahi_open);
    return ahi_open;
}

static void OS4_CloseAhiDevice(OS4AudioData *os4data)
{

    if (os4data->ahi_IORequest[0])
    {
        dprintf("Aborting I/O...\n");
        if (os4data->link)
        {
            IExec->AbortIO((struct IORequest *)os4data->link);
            IExec->WaitIO((struct IORequest *)os4data->link);
        }

        dprintf("Closing device\n");
        IExec->CloseDevice((struct IORequest *)os4data->ahi_IORequest[0]);

        dprintf("Deleting I/O request\n");
        IExec->FreeSysObject( ASOT_IOREQUEST, os4data->ahi_IORequest[0] );
        os4data->ahi_IORequest[0] = NULL;
        IExec->FreeSysObject( ASOT_IOREQUEST, os4data->ahi_IORequest[1] );
        os4data->ahi_IORequest[1] = NULL;
    }

    if (os4data->ahi_ReplyPort)
    {
        dprintf("Deleting message port\n");
        IExec->FreeSysObject( ASOT_PORT, os4data->ahi_ReplyPort );
        os4data->ahi_ReplyPort = NULL;
    }

    dprintf("done closing\n");
}

static int OS4_AudioAvailable(void)
{
   OS4AudioData data;
   int is_available = OS4_OpenAhiDevice(&data);
   if (is_available)
   {
       OS4_CloseAhiDevice(&data);
   }
   dprintf("AHI is %savailable\n", is_available ? "" : "not ");
   return is_available;
}

/* ---------------------------------------------- */
/* Audio driver exported functions implementation */
/* ---------------------------------------------- */
static void OS4_CloseAudio(_THIS)
{
    OS4AudioData * os4data = _this->hidden;

    if (os4data->audio_MixBuffer[0])
    {
        SDL_FreeAudioMem(os4data->audio_MixBuffer[0]);
        os4data->audio_MixBuffer[0] = NULL;
    }

     if (os4data->audio_MixBuffer[1])
    {
        SDL_FreeAudioMem(os4data->audio_MixBuffer[1]);
        os4data->audio_MixBuffer[1] = NULL;
    }
    os4data->audio_IsOpen = 0;
}

static int OS4_OpenAudio(_THIS, const char *devname, int iscapture)
{
    int result = 0;
    OS4AudioData * os4data = _this->hidden;

	if ((_this->spec.format & 0xff) != 8)
		_this->spec.format = AUDIO_S16MSB;

    dprintf("New format = 0x%x\n", _this->spec.format);
    dprintf("Buffer size = %d\n", _this->spec.size);

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&_this->spec);

    /* Allocate mixing buffer */
    os4data->audio_MixBufferSize = _this->spec.size;
    os4data->audio_MixBuffer[0] = (Uint8 *)SDL_AllocAudioMem(_this->spec.size);
    os4data->audio_MixBuffer[1] = (Uint8 *)SDL_AllocAudioMem(_this->spec.size);
    if ( os4data->audio_MixBuffer[0] == NULL || os4data->audio_MixBuffer[1] == NULL )
    {
        OS4_CloseAudio(_this);
        dprintf("No memory for audio buffer\n");
        return -1;
    }
    SDL_memset(os4data->audio_MixBuffer[0], _this->spec.silence, _this->spec.size);
    SDL_memset(os4data->audio_MixBuffer[1], _this->spec.silence, _this->spec.size);

	switch( _this->spec.format )
	{
		case AUDIO_S8:
		case AUDIO_U8:
			os4data->ahi_Type = (_this->spec.channels<2) ? AHIST_M8S : AHIST_S8S;
			break;
	
		default:
			os4data->ahi_Type = (_this->spec.channels<2) ? AHIST_M16S : AHIST_S16S;
			break;
	}
	/* This will cause a lot of problems.. and should be removed */
	IExec->SetTaskPri(IExec->FindTask(0), 5);
	
    os4data->audio_IsOpen = 1;

    return result;
}

static void OS4_WaitDone(_THIS)
{
    OS4_CloseAhiDevice(_this->hidden);
}

static void OS4_WaitAudio(_THIS)
{
  /* Dummy - OS4_PlayAudio handles the waiting */
}

static void OS4_PlayAudio(_THIS)
{
    struct AHIRequest  *ahi_IORequest;
    SDL_AudioSpec      *spec    = &_this->spec;
    OS4AudioData       *os4data = _this->hidden;

    ahi_IORequest = os4data->ahi_IORequest[os4data->currentBuffer];

    ahi_IORequest->ahir_Std.io_Message.mn_Node.ln_Pri = 60;
    ahi_IORequest->ahir_Std.io_Data    = os4data->audio_MixBuffer[os4data->currentBuffer];
    ahi_IORequest->ahir_Std.io_Length  = os4data->audio_MixBufferSize;
    ahi_IORequest->ahir_Std.io_Offset  = 0;
    ahi_IORequest->ahir_Std.io_Command = CMD_WRITE;
    ahi_IORequest->ahir_Volume         = 0x10000;
    ahi_IORequest->ahir_Position       = 0x8000;
    ahi_IORequest->ahir_Link           = os4data->link;
    ahi_IORequest->ahir_Frequency      = spec->freq;
    ahi_IORequest->ahir_Type           = os4data->ahi_Type;

	// Convert to signed?
	if( spec->format == AUDIO_U8 )
	{
#if POSSIBLY_DANGEROUS_OPTIMISATION
		int i, n;
		uint32 *mixbuf = (uint32 *)os4data->audio_MixBuffer[os4data->currentBuffer];
		n = os4data->audio_MixBufferSize/4; // let the gcc optimiser decide the best way to divide by 4
		for( i=0; i<n; i++ )
			*(mixbuf++) ^= 0x80808080;
		if (0 != (n = os4data->audio_MixBufferSize & 3))
		{
			uint8  *mixbuf8 = (uint8*)mixbuf;
			for( i=0; i<n; i++ )
				*(mixbuf8++) -= 128;
		}
#else
		int i;
		for( i=0; i<os4data->audio_MixBufferSize; i++ )
			os4data->audio_MixBuffer[os4data->currentBuffer][i] -= 128;		
#endif
	}

    IExec->SendIO((struct IORequest*)ahi_IORequest);

    if (os4data->link)
        IExec->WaitIO((struct IORequest*)os4data->link);

    os4data->link = ahi_IORequest;

    os4data->currentBuffer = 1-os4data->currentBuffer;
}

static Uint8 *OS4_GetAudioBuf(_THIS)
{
    return _this->hidden->audio_MixBuffer[_this->hidden->currentBuffer];
}

/* ------------------------------------------ */
/* Audio driver init functions implementation */
/* ------------------------------------------ */
static int
OS4_Init(SDL_AudioDriverImpl * impl)
{
    if (OS4_AudioAvailable() != 1) {
        return 0;
    }

    /* Set the function pointers */
    impl->OpenDevice = OS4_OpenAudio;
    impl->WaitDevice = OS4_WaitAudio;
	impl->WaitDone = OS4_WaitDone;
    impl->GetDeviceBuf = OS4_GetAudioBuf;
    impl->PlayDevice = OS4_PlayAudio;
    impl->CloseDevice = OS4_CloseAudio;
    impl->OnlyHasDefaultOutputDevice = 1;

    return 1;   /* this audio target is available. */
}

AudioBootStrap AMIGAOS4_bootstrap = {
   DRIVER_NAME, "AmigaOS4 AHI audio", OS4_Init, 0
};
#endif
