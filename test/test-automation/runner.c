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

#include "SDL/SDL_loadso.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
	int pid = getpid();
	int testsFailed = 0, testsPassed = 0;
	
	char *libName = "libtest.so";
	printf("%d: Loading .so containing tests\n", pid);
	void *library = SDL_LoadObject(libName);
	if(library == NULL) {
		printf("Loading %s failed\n", libName);
		printf("%s\n", SDL_GetError());
	}

	printf("%d: Asking for the test case names\n", pid);
	char **(*suite)(void);
	suite = (char **(*)(void)) SDL_LoadFunction(library, "suite");
	if(suite == NULL) {
		printf("%d: Retrieving test names failed, suite == NULL\n", pid);
		printf("%s\n", SDL_GetError());
	} else {
		char **tests = suite();

		char *testname = NULL;
		int counter = 0;
		for(; (testname = tests[counter]); ++counter) {
			int childpid = fork();
			if(childpid == 0) {
				pid = getpid();
				
				printf("%d: Loading test: %s\n", pid, testname);

				void (*test)(void *arg);

				test = (void (*)(void *)) SDL_LoadFunction(library, testname);
				if(test == NULL) {
					printf("%d: Loading test failed, tests == NULL\n", pid);
					printf("%s\n", SDL_GetError());
				} else {
					test(0x0);
				}
				return 0; // exit the child if the test didn't exit
			} else {
				int stat_lock = -1;
				int child = wait(&stat_lock);
				
				if(WIFEXITED(stat_lock)) {
					int rv = WEXITSTATUS(stat_lock);
					printf("%d: %d exited normally with value %d\n", pid, child, rv);

					testsPassed++;
				} else if(WIFSIGNALED(stat_lock)) {
					int signal = WTERMSIG(stat_lock);
					printf("%d: %d was killed by signal nro %d\n", pid, child, signal);
					
					testsFailed++;
				} else if(WIFSTOPPED(stat_lock)) {
					//int signal = WSTOPSIG(stat_lock);
					//printf("%d: %d was stopped by signal nro %d\n", pid, child, signal);
				}
			}
		}
	}
	
	printf("%d: all tests executed\n", pid);
	printf("%d: %d tests passed\n", pid, testsPassed);
	printf("%d: %d tests failed\n", pid, testsFailed);

	SDL_UnloadObject(library);
	
	return 0;
}
