/*
 *	Audio interrupt
 *
 *	Patrice Mandin
 */

#ifndef _SDL_MINTAUDIOINTERRUPT_S_H_
#define _SDL_MINTAUDIOINTERRUPT_S_H_

#include "SDL_types.h"

/* Variables */

extern void *SDL_MintAudio_audiobuf[2];	/* Pointers to buffers */
extern long SDL_MintAudio_audiosize;	/* Length of audio buffer */
extern long SDL_MintAudio_numbuf;		/* Buffer to play */
extern long SDL_MintAudio_mutex;

/* Functions */
void SDL_MintAudio_IntDma(void);
void SDL_MintAudio_IntXbios(void);
void SDL_MintAudio_IntGsxb(void);

#endif /* _SDL_MINTAUDIOINTERRUPT_S_H_ */
