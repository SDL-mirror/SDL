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

//!< Function pointer to a test case function
typedef int (*TestCase)(void *arg);

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

/*!
 * Loads test case from a test suite
 *
 * \param Test suite
 * \testName Name of the test that is going to be loaded
 *
 * \return loaded test
 */
TestCase LoadTestCase(void *suite, char *testName) {
	TestCase test = (int (*)(void *)) SDL_LoadFunction(suite, testName);
	if(test == NULL) {
		printf("Loading test failed, tests == NULL\n");
		printf("%s\n", SDL_GetError());
	}

	return test;
}



/*!
 * If using out-of-proc execution of tests. This function
 * will handle the return value of the child process
 * and interprets it to the runner. Also prints warnings
 * if child was aborted by a signela.
 *
 * \param stat_lock information about the exited child process
 *
 * \return 0 if test case succeeded, 1 otherwise
 */
int HandleTestReturnValue(int stat_lock) {
	//! \todo rename to: HandleChildReturn Value
	int returnValue = -1;

	if(WIFEXITED(stat_lock)) {
		returnValue = WEXITSTATUS(stat_lock);
	} else if(WIFSIGNALED(stat_lock)) {
		int signal = WTERMSIG(stat_lock);
		printf("FAILURE: test was aborted due to signal nro %d\n", signal);
		returnValue = 1;
	} else if(WIFSTOPPED(stat_lock)) {
	}

	return returnValue;
}

//!< Flag for executing tests in-process
int execute_inproc = 0;

/*!
 * Parse command line arguments
 */
void
ParseOptions(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc; ++i) {
      const char *arg = argv[i];
      if (SDL_strcmp(arg, "--in-proc") == 0) {
         execute_inproc = 1;
      }
   }
}

int
main(int argc, char *argv[])
{
	ParseOptions(argc, argv);

	// print: Testing againts SDL version fuu (rev: bar) if verbose == true

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

			int retVal = 1;
			if(execute_inproc) {
				TestCase test = (TestCase) LoadTestCase(suite, testname);
				retVal = test(0x0);
			} else {
				int childpid = fork();
				if(childpid == 0) {
					TestCase test = (TestCase) LoadTestCase(suite, testname);
					return test(0x0);
				} else {
					int stat_lock = -1;
					int child = wait(&stat_lock);

					retVal = HandleTestReturnValue(stat_lock);
				}
			}

			if(retVal) {
				failureCount++;
				printf("%s (in %s): FAILED\n", testname, libName);
			} else {
				passCount++;
				printf("%s (in %s): ok\n", testname, libName);
			}
		}

		printf("\n");
	}
	
	SDL_UnloadObject(suite);

	const Uint32 endTicks = SDL_GetTicks();

	printf("Ran %d tests in %0.5f seconds.\n", (passCount + failureCount), (endTicks-startTicks)/1000.0f);

	printf("%d tests passed\n", passCount);
	printf("%d tests failed\n", failureCount);

	return 0;
}
