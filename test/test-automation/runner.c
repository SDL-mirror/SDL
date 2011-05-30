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

#include "tests/SDL_test.h"

/*!
 * Loads test suite which is implemented as dynamic library.
 *
 * \return Loaded test suite
 */
void *LoadTestSuite() {
#if defined(linux) || defined( __linux)
	char *libName = "tests/libtest.so";
#else
	char *libName = "tests/libtest.dylib";
#endif

	void *library = SDL_LoadObject(libName);
	if(library == NULL) {
		printf("Loading %s failed\n", libName);
		printf("%s\n", SDL_GetError());
	}

	return library;
}

/*!
 * Loads the test case references from the given test suite.

 * \param library Previously loaded dynamic library AKA test suite
 * \return Loaded TestCaseReferences
 */
TestCaseReference **QueryTestCases(void *library) {
	TestCaseReference **(*suite)(void);

	suite = (TestCaseReference **(*)(void)) SDL_LoadFunction(library, "QueryTestSuite");
	if(suite == NULL) {
		printf("Loading QueryTestCaseReferences() failed.\n");
		printf("%s\n", SDL_GetError());
	}

	TestCaseReference **tests = suite();
	if(tests == NULL) {
		printf("Failed to load test references.\n");
		printf("%s\n", SDL_GetError());
	}

	return tests;
}

/*
 *
 */

/*!
 * Success or failure of test case is determined by
 * it's return value. If test case succeeds, it'll
 * return 0, if not it will return a positive integer.
 *
 * The function checks the return value and returns value
 * based on it. If the test is aborted due to a signal
 * function warn about it.
 *
 * \return 1 if test case succeeded, 0 otherwise
 */
int HandleTestReturnValue(int stat_lock) {
	if(WIFEXITED(stat_lock)) {
		int returnValue = WEXITSTATUS(stat_lock);

		if(returnValue == 0) {
			return  1;
		}
	} else if(WIFSIGNALED(stat_lock)) {
		int signal = WTERMSIG(stat_lock);
		printf("FAILURE: test was aborted due to signal nro %d\n", signal);

	} else if(WIFSTOPPED(stat_lock)) {
	}

	return 0;
}


int main(int argc, char *argv[]) {

	//! \todo Handle command line arguments

	// print: Testing againts SDL version fuu (rev: bar)

	int failureCount = 0, passCount = 0;

	char *libName = "libtest";

	const Uint32 startTicks = SDL_GetTicks();

	void *suite = LoadTestSuite();
	TestCaseReference **tests = QueryTestCases(suite);

	TestCaseReference *reference = NULL;
	int counter = 0;

	for(reference = tests[counter]; reference; reference = tests[++counter]) {
		if(reference->enabled == TEST_DISABLED) {
			printf("Test %s (in %s) disabled. Omitting...\n", reference->name, libName);
		} else {
			char *testname = reference->name;

			printf("Running %s (in %s):\n", testname, libName);

			int childpid = fork();
			if(childpid == 0) {
				void (*test)(void *arg);

				test = (void (*)(void *)) SDL_LoadFunction(suite, testname);
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

				int passed = -1;

				passed = HandleTestReturnValue(stat_lock);

				if(passed) {
					passCount++;
					printf("%s (in %s): ok\n", testname, libName);
				} else {
					failureCount++;
					printf("%s (in %s): failed\n", testname, libName);
				}
			}
		}

		printf("\n");
	}
	
	SDL_UnloadObject(suite);

	const Uint32 endTicks = SDL_GetTicks();

	printf("Ran %d tests in %0.3f seconds.\n", (passCount + failureCount), (endTicks-startTicks)/1000.0f);

	printf("%d tests passed\n", passCount);
	printf("%d tests failed\n", failureCount);

	return 0;
}
