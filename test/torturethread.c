
/* Simple test of the SDL threading code */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "SDL.h"
#include "SDL_thread.h"

#define NUMTHREADS 10

static char volatile time_for_threads_to_die[NUMTHREADS];

int SubThreadFunc(void *data) {
	while(! *(int volatile *)data) {
		; /*SDL_Delay(10); /* do nothing */
	}
	return 0;
}

int ThreadFunc(void *data) {
	SDL_Thread *sub_threads[NUMTHREADS];
	int flags[NUMTHREADS];
	int i;
	int tid = (int ) data;

	fprintf(stderr, "Creating Thread %d\n", tid);

	for(i = 0; i < NUMTHREADS; i++) {
		flags[i] = 0;
		sub_threads[i] = SDL_CreateThread(SubThreadFunc, &flags[i]);
	}

	printf("Thread '%d' waiting for signal\n", tid);
	while(time_for_threads_to_die[tid] != 1) {
		; /* do nothing */
	}

	printf("Thread '%d' sending signals to subthreads\n", tid);
	for(i = 0; i <  NUMTHREADS; i++) {
		flags[i] = 1;
		SDL_WaitThread(sub_threads[i], NULL);
	}

	printf("Thread '%d' exiting!\n", tid);

	return 0;
}

int main(int argc, char *argv[])
{
	SDL_Thread *threads[NUMTHREADS];
	int i;

	/* Load the SDL library */
	if ( SDL_Init(0) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);


	signal(SIGSEGV, SIG_DFL);
	for(i = 0; i < NUMTHREADS; i++) {
		time_for_threads_to_die[i] = 0;
		threads[i] = SDL_CreateThread(ThreadFunc, (void *) i);
	
		if ( threads[i] == NULL ) {
			fprintf(stderr,
			"Couldn't create thread: %s\n", SDL_GetError());
			exit(1);
		}
	}

	for(i = 0; i < NUMTHREADS; i++) {
		time_for_threads_to_die[i] = 1;
	}

	for(i = NUMTHREADS-1; i >= 0; --i) {
		SDL_WaitThread(threads[i], NULL);
	}
	return(0);	/* Never reached */
}
