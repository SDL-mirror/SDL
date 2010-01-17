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

/*
  The PulseAudio target for SDL 1.3 is based on the 1.3 arts target, with
   the appropriate parts replaced with the 1.2 PulseAudio target code. This
   was the cleanest way to move it to 1.3. The 1.2 target was written by
   Stéphan Kochen: stephan .a.t. kochen.nl
*/

#include "SDL_config.h"

/* Allow access to a raw mixing buffer */

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "SDL_pulseaudio.h"

#ifdef SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC
#include "SDL_name.h"
#include "SDL_loadso.h"
#else
#define SDL_NAME(X)	X
#endif

/* The tag name used by pulse audio */
#define PULSEAUDIO_DRIVER_NAME         "pulseaudio"

#ifdef SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC

#if (PA_API_VERSION < 12)
/** Return non-zero if the passed state is one of the connected states */
static inline int PA_CONTEXT_IS_GOOD(pa_context_state_t x) {
    return
        x == PA_CONTEXT_CONNECTING ||
        x == PA_CONTEXT_AUTHORIZING ||
        x == PA_CONTEXT_SETTING_NAME ||
        x == PA_CONTEXT_READY;
}
/** Return non-zero if the passed state is one of the connected states */
static inline int PA_STREAM_IS_GOOD(pa_stream_state_t x) {
    return
        x == PA_STREAM_CREATING ||
        x == PA_STREAM_READY;
}
#endif /* pulseaudio <= 0.9.10 */

static const char *pulse_library = SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC;
static void *pulse_handle = NULL;

/* !!! FIXME: I hate this SDL_NAME clutter...it makes everything so messy! */
static pa_simple *(*SDL_NAME(pa_simple_new)) (const char *server,
                                              const char *name,
                                              pa_stream_direction_t dir,
                                              const char *dev,
                                              const char *stream_name,
                                              const pa_sample_spec * ss,
                                              const pa_channel_map * map,
                                              const pa_buffer_attr * attr,
                                              int *error);
static void (*SDL_NAME(pa_simple_free)) (pa_simple * s);
static pa_channel_map *(*SDL_NAME(pa_channel_map_init_auto)) (pa_channel_map *
                                                              m,
                                                              unsigned
                                                              channels,
                                                              pa_channel_map_def_t
                                                              def);
static const char *(*SDL_NAME(pa_strerror)) (int error);
static pa_mainloop * (*SDL_NAME(pa_mainloop_new))(void);
static pa_mainloop_api * (*SDL_NAME(pa_mainloop_get_api))(pa_mainloop *m);
static int (*SDL_NAME(pa_mainloop_iterate))(pa_mainloop *m, int block, int *retval);
static void (*SDL_NAME(pa_mainloop_free))(pa_mainloop *m);

static pa_operation_state_t (*SDL_NAME(pa_operation_get_state))(pa_operation *o);
static void (*SDL_NAME(pa_operation_cancel))(pa_operation *o);
static void (*SDL_NAME(pa_operation_unref))(pa_operation *o);

static pa_context * (*SDL_NAME(pa_context_new))(
    pa_mainloop_api *m, const char *name);
static int (*SDL_NAME(pa_context_connect))(
    pa_context *c, const char *server,
    pa_context_flags_t flags, const pa_spawn_api *api);
static pa_context_state_t (*SDL_NAME(pa_context_get_state))(pa_context *c);
static void (*SDL_NAME(pa_context_disconnect))(pa_context *c);
static void (*SDL_NAME(pa_context_unref))(pa_context *c);

static pa_stream * (*SDL_NAME(pa_stream_new))(pa_context *c,
    const char *name, const pa_sample_spec *ss, const pa_channel_map *map);
static int (*SDL_NAME(pa_stream_connect_playback))(pa_stream *s, const char *dev,
    const pa_buffer_attr *attr, pa_stream_flags_t flags,
    pa_cvolume *volume, pa_stream *sync_stream);
static pa_stream_state_t (*SDL_NAME(pa_stream_get_state))(pa_stream *s);
static size_t (*SDL_NAME(pa_stream_writable_size))(pa_stream *s);
static int (*SDL_NAME(pa_stream_write))(pa_stream *s, const void *data, size_t nbytes,
    pa_free_cb_t free_cb, int64_t offset, pa_seek_mode_t seek);
static pa_operation * (*SDL_NAME(pa_stream_drain))(pa_stream *s,
    pa_stream_success_cb_t cb, void *userdata);
static int (*SDL_NAME(pa_stream_disconnect))(pa_stream *s);
static void (*SDL_NAME(pa_stream_unref))(pa_stream *s);


#define SDL_PULSEAUDIO_SYM(x) { #x, (void **) (char *) &SDL_NAME(x) }
static struct
{
    const char *name;
    void **func;
} pulse_functions[] = {
/* *INDENT-OFF* */
    SDL_PULSEAUDIO_SYM(pa_simple_new),
    SDL_PULSEAUDIO_SYM(pa_simple_free),
    SDL_PULSEAUDIO_SYM(pa_mainloop_new),
    SDL_PULSEAUDIO_SYM(pa_mainloop_get_api),
    SDL_PULSEAUDIO_SYM(pa_mainloop_iterate),
    SDL_PULSEAUDIO_SYM(pa_mainloop_free),
    SDL_PULSEAUDIO_SYM(pa_operation_get_state),
    SDL_PULSEAUDIO_SYM(pa_operation_cancel),
    SDL_PULSEAUDIO_SYM(pa_operation_unref),
    SDL_PULSEAUDIO_SYM(pa_context_new),
    SDL_PULSEAUDIO_SYM(pa_context_connect),
    SDL_PULSEAUDIO_SYM(pa_context_get_state),
    SDL_PULSEAUDIO_SYM(pa_context_disconnect),
    SDL_PULSEAUDIO_SYM(pa_context_unref),
    SDL_PULSEAUDIO_SYM(pa_stream_new),
    SDL_PULSEAUDIO_SYM(pa_stream_connect_playback),
    SDL_PULSEAUDIO_SYM(pa_stream_get_state),
    SDL_PULSEAUDIO_SYM(pa_stream_writable_size),
    SDL_PULSEAUDIO_SYM(pa_stream_write),
    SDL_PULSEAUDIO_SYM(pa_stream_drain),
    SDL_PULSEAUDIO_SYM(pa_stream_disconnect),
    SDL_PULSEAUDIO_SYM(pa_stream_unref),
    SDL_PULSEAUDIO_SYM(pa_channel_map_init_auto),
    SDL_PULSEAUDIO_SYM(pa_strerror),
/* *INDENT-ON* */
};

#undef SDL_PULSEAUDIO_SYM

static void
UnloadPulseLibrary()
{
    if (pulse_handle != NULL) {
        SDL_UnloadObject(pulse_handle);
        pulse_handle = NULL;
    }
}

static int
LoadPulseLibrary(void)
{
    int i, retval = -1;

    if (pulse_handle == NULL) {
        pulse_handle = SDL_LoadObject(pulse_library);
        if (pulse_handle != NULL) {
            retval = 0;
            for (i = 0; i < SDL_arraysize(pulse_functions); ++i) {
                *pulse_functions[i].func =
                    SDL_LoadFunction(pulse_handle, pulse_functions[i].name);
                if (!*pulse_functions[i].func) {
                    retval = -1;
                    UnloadPulseLibrary();
                    break;
                }
            }
        }
    }

    return retval;
}

#else

static void
UnloadPulseLibrary()
{
    return;
}

static int
LoadPulseLibrary(void)
{
    return 0;
}

#endif /* SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC */

/* This function waits until it is possible to write a full sound buffer */
static void
PULSEAUDIO_WaitDevice(_THIS)
{
    while(1) {
        if (SDL_NAME(pa_context_get_state)(this->hidden->context) != PA_CONTEXT_READY ||
            SDL_NAME(pa_stream_get_state)(this->hidden->stream) != PA_STREAM_READY ||
            SDL_NAME(pa_mainloop_iterate)(this->hidden->mainloop, 1, NULL) < 0) {
            this->enabled = 0;
            return;
        }
        if (SDL_NAME(pa_stream_writable_size)(this->hidden->stream) >= this->hidden->mixlen) {
            return;
        }
    }
}

static void
PULSEAUDIO_PlayDevice(_THIS)
{
    /* Write the audio data */
    if (SDL_NAME(pa_stream_write) (this->hidden->stream, this->hidden->mixbuf,
                                   this->hidden->mixlen, NULL, 0LL,
                                   PA_SEEK_RELATIVE) < 0) {
        this->enabled = 0;
    }
}

static void
stream_drain_complete(pa_stream *s, int success, void *userdata)
{
    /* no-op for pa_stream_drain() to use for callback. */
}

static void
PULSEAUDIO_WaitDone(_THIS)
{
    pa_operation *o;

    o = SDL_NAME(pa_stream_drain)(this->hidden->stream, stream_drain_complete, NULL);
    if (!o) {
        return;
    }

    while (SDL_NAME(pa_operation_get_state)(o) != PA_OPERATION_DONE) {
        if (SDL_NAME(pa_context_get_state)(this->hidden->context) != PA_CONTEXT_READY ||
            SDL_NAME(pa_stream_get_state)(this->hidden->stream) != PA_STREAM_READY ||
            SDL_NAME(pa_mainloop_iterate)(this->hidden->mainloop, 1, NULL) < 0) {
            SDL_NAME(pa_operation_cancel)(o);
            break;
        }
    }

    SDL_NAME(pa_operation_unref)(o);
}



static Uint8 *
PULSEAUDIO_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}


static void
PULSEAUDIO_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->mixbuf != NULL) {
            SDL_FreeAudioMem(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }
        if (this->hidden->stream) {
            SDL_NAME(pa_stream_disconnect)(this->hidden->stream);
            SDL_NAME(pa_stream_unref)(this->hidden->stream);
            this->hidden->stream = NULL;
        }
        if (this->hidden->context != NULL) {
            SDL_NAME(pa_context_disconnect)(this->hidden->context);
            SDL_NAME(pa_context_unref)(this->hidden->context);
            this->hidden->context = NULL;
        }
        if (this->hidden->mainloop != NULL) {
            SDL_NAME(pa_mainloop_free)(this->hidden->mainloop);
            this->hidden->mainloop = NULL;
        }
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}


/* !!! FIXME: this could probably be expanded. */
/* Try to get the name of the program */
static char *
get_progname(void)
{
#ifdef __LINUX__
    char *progname = NULL;
    FILE *fp;
    static char temp[BUFSIZ];

    SDL_snprintf(temp, SDL_arraysize(temp), "/proc/%d/cmdline", getpid());
    fp = fopen(temp, "r");
    if (fp != NULL) {
        if (fgets(temp, sizeof(temp) - 1, fp)) {
            progname = SDL_strrchr(temp, '/');
            if (progname == NULL) {
                progname = temp;
            } else {
                progname = progname + 1;
            }
        }
        fclose(fp);
    }
    return(progname);
#elif defined(__NetBSD__)
    return getprogname();
#else
    return("unknown");
#endif
}


static int
PULSEAUDIO_OpenDevice(_THIS, const char *devname, int iscapture)
{
    Uint16 test_format = 0;
    pa_sample_spec paspec;
    pa_buffer_attr paattr;
    pa_channel_map pacmap;
    pa_stream_flags_t flags = 0;
    int state = 0;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    paspec.format = PA_SAMPLE_INVALID;

    /* Try for a closest match on audio format */
    for (test_format = SDL_FirstAudioFormat(this->spec.format);
         (paspec.format == PA_SAMPLE_INVALID) && test_format;) {
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Trying format 0x%4.4x\n", test_format);
#endif
        switch (test_format) {
        case AUDIO_U8:
            paspec.format = PA_SAMPLE_U8;
            break;
        case AUDIO_S16LSB:
            paspec.format = PA_SAMPLE_S16LE;
            break;
        case AUDIO_S16MSB:
            paspec.format = PA_SAMPLE_S16BE;
            break;
        default:
            paspec.format = PA_SAMPLE_INVALID;
            break;
        }
        if (paspec.format == PA_SAMPLE_INVALID) {
            test_format = SDL_NextAudioFormat();
        }
    }
    if (paspec.format == PA_SAMPLE_INVALID) {
        PULSEAUDIO_CloseDevice(this);
        SDL_SetError("Couldn't find any hardware audio formats");
        return 0;
    }
    this->spec.format = test_format;

    /* Calculate the final parameters for this audio specification */
#ifdef PA_STREAM_ADJUST_LATENCY
    this->spec.samples /= 2; /* Mix in smaller chunck to avoid underruns */
#endif
    SDL_CalculateAudioSpec(&this->spec);

    /* Allocate mixing buffer */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if (this->hidden->mixbuf == NULL) {
        PULSEAUDIO_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

    paspec.channels = this->spec.channels;
    paspec.rate = this->spec.freq;

    /* Reduced prebuffering compared to the defaults. */
#ifdef PA_STREAM_ADJUST_LATENCY
    paattr.tlength = this->hidden->mixlen * 4; /* 2x original requested bufsize */
    paattr.prebuf = -1;
    paattr.maxlength = -1;
    /* -1 can lead to pa_stream_writable_size() >= this->hidden->mixlen never being true */
    paattr.minreq = this->hidden->mixlen;
    flags = PA_STREAM_ADJUST_LATENCY;
#else
    paattr.tlength = this->hidden->mixlen*2;
    paattr.prebuf = this->hidden->mixlen*2;
    paattr.maxlength = this->hidden->mixlen*2;
    paattr.minreq = this->hidden->mixlen;
#endif

    /* The SDL ALSA output hints us that we use Windows' channel mapping */
    /* http://bugzilla.libsdl.org/show_bug.cgi?id=110 */
    SDL_NAME(pa_channel_map_init_auto) (&pacmap, this->spec.channels,
                                        PA_CHANNEL_MAP_WAVEEX);

    /* Set up a new main loop */
    if (!(this->hidden->mainloop = SDL_NAME(pa_mainloop_new)())) {
        PULSEAUDIO_CloseDevice(this);
        SDL_SetError("pa_mainloop_new() failed");
        return 0;
    }

    this->hidden->mainloop_api = SDL_NAME(pa_mainloop_get_api)(this->hidden->mainloop);
    if (!(this->hidden->context = SDL_NAME(pa_context_new)(this->hidden->mainloop_api, get_progname()))) {
        PULSEAUDIO_CloseDevice(this);
        SDL_SetError("pa_context_new() failed");
        return 0;
    }

    /* Connect to the PulseAudio server */
    if (SDL_NAME(pa_context_connect)(this->hidden->context, NULL, 0, NULL) < 0) {
        PULSEAUDIO_CloseDevice(this);
        SDL_SetError("Could not setup connection to PulseAudio");
        return 0;
    }

    do {
        if (SDL_NAME(pa_mainloop_iterate)(this->hidden->mainloop, 1, NULL) < 0) {
            PULSEAUDIO_CloseDevice(this);
            SDL_SetError("pa_mainloop_iterate() failed");
            return 0;
        }
        state = SDL_NAME(pa_context_get_state)(this->hidden->context);
        if (!PA_CONTEXT_IS_GOOD(state)) {
            PULSEAUDIO_CloseDevice(this);
            SDL_SetError("Could not connect to PulseAudio");
            return 0;
        }
    } while (state != PA_CONTEXT_READY);

    this->hidden->stream = SDL_NAME(pa_stream_new)(
        this->hidden->context,
        "Simple DirectMedia Layer", /* stream description */
        &paspec,    /* sample format spec */
        &pacmap     /* channel map */
        );

    if (this->hidden->stream == NULL) {
        PULSEAUDIO_CloseDevice(this);
        SDL_SetError("Could not set up PulseAudio stream");
        return 0;
    }

    if (SDL_NAME(pa_stream_connect_playback)(this->hidden->stream, NULL, &paattr, flags,
            NULL, NULL) < 0) {
        PULSEAUDIO_CloseDevice(this);
        SDL_SetError("Could not connect PulseAudio stream");
        return 0;
    }

    do {
        if (SDL_NAME(pa_mainloop_iterate)(this->hidden->mainloop, 1, NULL) < 0) {
            PULSEAUDIO_CloseDevice(this);
            SDL_SetError("pa_mainloop_iterate() failed");
            return 0;
        }
        state = SDL_NAME(pa_stream_get_state)(this->hidden->stream);
        if (!PA_STREAM_IS_GOOD(state)) {
            PULSEAUDIO_CloseDevice(this);
            SDL_SetError("Could not create to PulseAudio stream");
            return 0;
        }
    } while (state != PA_STREAM_READY);

    /* We're ready to rock and roll. :-) */
    return 1;
}


static void
PULSEAUDIO_Deinitialize(void)
{
    UnloadPulseLibrary();
}


static int
PULSEAUDIO_Init(SDL_AudioDriverImpl * impl)
{
    if (LoadPulseLibrary() < 0) {
        return 0;
    }

    /* Set the function pointers */
    impl->OpenDevice = PULSEAUDIO_OpenDevice;
    impl->PlayDevice = PULSEAUDIO_PlayDevice;
    impl->WaitDevice = PULSEAUDIO_WaitDevice;
    impl->GetDeviceBuf = PULSEAUDIO_GetDeviceBuf;
    impl->CloseDevice = PULSEAUDIO_CloseDevice;
    impl->WaitDone = PULSEAUDIO_WaitDone;
    impl->Deinitialize = PULSEAUDIO_Deinitialize;
    impl->OnlyHasDefaultOutputDevice = 1;

    /* !!! FIXME: should test if server is available here, return 2 if so. */
    return 1;
}


AudioBootStrap PULSEAUDIO_bootstrap = {
    PULSEAUDIO_DRIVER_NAME, "PulseAudio", PULSEAUDIO_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
