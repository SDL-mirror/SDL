
/* Simple test of the SDL threading code */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "SDL.h"
#include "SDL_thread.h"

static int alive = 0;

int ThreadFunc(void *data)
{
	printf("Started thread %s: My thread id is %u\n",
				(char *)data, SDL_ThreadID());
	while ( alive ) {
		printf("Thread '%s' is alive!\n", (char *)data);
		SDL_Delay(1*1000);
	}
	printf("Thread '%s' exiting!\n", (char *)data);
	return(0);
}

static void killed(int sig)
{
	printf("Killed with SIGTERM, waiting 5 seconds to exit\n");
	SDL_Delay(5*1000);
	alive = 0;
	exit(0);
}

int main(int argc, char *argv[])
{
	SDL_Thread *thread;

	/* Load the SDL library */
	if ( SDL_Init(0) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	alive = 1;
	thread = SDL_CreateThread(ThreadFunc, "#1");
	if ( thread == NULL ) {
		fprintf(stderr, "Couldn't create thread: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_Delay(5*1000);
	printf("Waiting for thread #1\n");
	alive = 0;
	SDL_WaitThread(thread, NULL);

	alive = 1;
	thread = SDL_CreateThread(ThreadFunc, "#2");
	if ( thread == NULL ) {
		fprintf(stderr, "Couldn't create thread: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_Delay(5*1000);
	printf("Killing thread #2\n");
	SDL_KillThread(thread);

	alive = 1;
	signal(SIGTERM, killed);
	thread = SDL_CreateThread(ThreadFunc, "#3");
	if ( thread == NULL ) {
		fprintf(stderr, "Couldn't create thread: %s\n", SDL_GetError());
		exit(1);
	}
	raise(SIGTERM);

	return(0);	/* Never reached */
}
