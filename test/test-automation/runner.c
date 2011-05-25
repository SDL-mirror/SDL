/*
  Copyright (C) 2011 Markus Kauppila <markus.kauppila@gmail.com>

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

#include "SDL/SDL.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>


int main(int argc, char *argv[]) {

	// Handle command line arguments

	// print: Testing againts SDL version fuu (rev: bar)

	int failureCount = 0, passCount = 0;

	const Uint32 startTicks = SDL_GetTicks();

#if defined(linux) || defined( __linux)
	char *libName = "tests/libtest.so";
#else 
	char *libName = "tests/libtest.0.dylib";
#endif

	void *library = SDL_LoadObject(libName);
	if(library == NULL) {
		printf("Loading %s failed\n", libName);
		printf("%s\n", SDL_GetError());
	}

	const char **(*suite)(void);
	suite = (const char **(*)(void)) SDL_LoadFunction(library, "suite");
	if(suite == NULL) {
		printf("Retrieving test names failed, suite == NULL\n");
		printf("%s\n", SDL_GetError());
	} else {
		const char **tests = suite();

		char *testname = NULL;
		int counter = 0;
		for(; (testname = (char *) tests[counter]); ++counter) {
			int childpid = fork();

			if(childpid == 0) {
				void (*test)(void *arg);

				test = (void (*)(void *)) SDL_LoadFunction(library, testname);
				if(test == NULL) {
					printf("Loading test failed, tests == NULL\n");
					printf("%s\n", SDL_GetError());
				} else {
					test(0x0);
				}
				return 0; // exit the child if the test didn't exit
			} else {
				int stat_lock = -1;
				int child = wait(&stat_lock);

				char *errorMsg = NULL;
				int passed = -1;
				if(WIFEXITED(stat_lock)) {
					int returnValue = WEXITSTATUS(stat_lock);

					if(returnValue == 0) {
						passed = 1;
					} else {
						passed = 0;
					}
				} else if(WIFSIGNALED(stat_lock)) {
					int signal = WTERMSIG(stat_lock);
					//printf("%d: %d was killed by signal nro %d\n", pid, child, signal);
					//errorMsg =
					errorMsg = SDL_malloc(256 * sizeof(char));
					sprintf(errorMsg, "was aborted due to signal nro %d", signal);

					passed = 0;
				} else if(WIFSTOPPED(stat_lock)) {
					//int signal = WSTOPSIG(stat_lock);
					//printf("%d: %d was stopped by signal nro %d\n", pid, child, signal);
				}

				printf("%s (in %s):", testname, libName);
				if(passed) {
					passCount++;
					printf("\tok\n");
				} else {
					failureCount++;
					printf("\tfailed\n");
					if(errorMsg) {
						printf("\t%s\n", errorMsg);
						SDL_free(errorMsg);
					}
				}
			}
		}
	}
	
	SDL_UnloadObject(library);

	const Uint32 endTicks = SDL_GetTicks();

	printf("Ran %d tests in %0.3f seconds.\n", (passCount + failureCount), (endTicks-startTicks)/1000.0f);

	printf("all tests executed\n");
	printf("%d tests passed\n", passCount);
	printf("%d tests failed\n", failureCount);

	return 0;
}
