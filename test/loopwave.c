
/* Program to load a wave file and loop playing it using SDL sound */

/* loopwaves.c is much more robust in handling WAVE files -- 
	This is only for simple WAVEs
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "SDL.h"
#include "SDL_audio.h"

struct {
	SDL_AudioSpec spec;
	Uint8   *sound;			/* Pointer to wave data */
	Uint32   soundlen;		/* Length of wave data */
	int      soundpos;		/* Current play position */
} wave;

void fillerup(void *unused, Uint8 *stream, int len)
{
	Uint8 *waveptr;
	int    waveleft;

	/* Set up the pointers */
	waveptr = wave.sound + wave.soundpos;
	waveleft = wave.soundlen - wave.soundpos;

	/* Go! */
	while ( waveleft <= len ) {
		SDL_MixAudio(stream, waveptr, waveleft, SDL_MIX_MAXVOLUME);
		stream += waveleft;
		len -= waveleft;
		waveptr = wave.sound;
		waveleft = wave.soundlen;
		wave.soundpos = 0;
	}
	SDL_MixAudio(stream, waveptr, len, SDL_MIX_MAXVOLUME);
	wave.soundpos += len;
}

static int done = 0;
void poked(int sig)
{
	done = 1;
}

int main(int argc, char *argv[])
{
	char name[32];

	/* Load the SDL library */
	if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	if ( argv[1] == NULL ) {
		fprintf(stderr, "Usage: %s <wavefile>\n", argv[0]);
		exit(1);
	}

	/* Load the wave file into memory */
	if ( SDL_LoadWAV(argv[1],
			&wave.spec, &wave.sound, &wave.soundlen) == NULL ) {
		fprintf(stderr, "Couldn't load %s: %s\n",
						argv[1], SDL_GetError());
		exit(1);
	}
	wave.spec.callback = fillerup;

	/* Set the signals */
#ifdef SIGHUP
	signal(SIGHUP, poked);
#endif
	signal(SIGINT, poked);
#ifdef SIGQUIT
	signal(SIGQUIT, poked);
#endif
	signal(SIGTERM, poked);

	/* Initialize fillerup() variables */
	if ( SDL_OpenAudio(&wave.spec, NULL) < 0 ) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		SDL_FreeWAV(wave.sound);
		exit(2);
	}
	SDL_PauseAudio(0);

	/* Let the audio run */
	printf("Using audio driver: %s\n", SDL_AudioDriverName(name, 32));
	while ( ! done && (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING) )
		SDL_Delay(1000);

	/* Clean up on signal */
	SDL_CloseAudio();
	SDL_FreeWAV(wave.sound);
	return(0);
}
