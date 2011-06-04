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

#include "SDL_test.h"

//!< Function pointer to a test case function
typedef void (*TestCase)(void *arg);
//!< Function pointer to a test case init function
typedef void (*TestCaseInit)(void);
//!< Function pointer to a test case quit function
typedef int  (*TestCaseQuit)(void);

//!< Flag for executing tests in-process
static int execute_inproc = 0;

/*!
 * Returns the name for the dynamic library
 * which implements the test suite.
 *
 * (in the future: scans the test/ directory and
 * returns the names of the dynamic libraries
 * implementing the test suites)
 *
 * \return Name of the dummy test suite
 */
char *
ScanForTestSuites() {
#if defined(linux) || defined( __linux)
	char *libName = "tests/libtest.so";
#else
	char *libName = "tests/libtestrect.dylib";
#endif
	return libName;
}


/*!
 * Loads test suite which is implemented as dynamic library.
 *
 * \param test0,330
 *
 * \return Pointer to loaded test suite, or NULL if library could not be loaded
 */
void *
LoadTestSuite(char *testSuiteName)
{
	void *library = SDL_LoadObject(testSuiteName);
	if(library == NULL) {
		fprintf(stderr, "Loading %s failed\n", testSuiteName);
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return library;
}

/*!
 * Loads the test case references from the given test suite.

 * \param library Previously loaded dynamic library AKA test suite
 * \return Pointer to array of TestCaseReferences or NULL if function failed
 */
TestCaseReference **
QueryTestCases(void *library)
{
	TestCaseReference **(*suite)(void);

	suite = (TestCaseReference **(*)(void)) SDL_LoadFunction(library, "QueryTestSuite");
	if(suite == NULL) {
		fprintf(stderr, "Loading QueryTestCaseReferences() failed.\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	TestCaseReference **tests = suite();
	if(tests == NULL) {
		fprintf(stderr, "Failed to load test references.\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return tests;
}


/*!
 * Loads test case from a test suite
 *
 * \param suite a test suite
 * \param testName Name of the test that is going to be loaded
 *
 * \return Function Pointer (TestCase) to loaded test case, NULL if function failed
 */
TestCase
LoadTestCase(void *suite, char *testName)
{
	TestCase test = (TestCase) SDL_LoadFunction(suite, testName);
	if(test == NULL) {
		fprintf(stderr, "Loading test failed, tests == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return test;
}

/*!
 * Loads function that initialises the test case from the
 * given test suite.
 *
 * \param suite Used test suite
 *
 * \return Function pointer (TestCaseInit) which points to loaded init function. NULL if function fails.
 */
TestCaseInit
LoadTestCaseInit(void *suite) {
	TestCaseInit testCaseInit = (TestCaseInit) SDL_LoadFunction(suite, "_TestCaseInit");
	if(testCaseInit == NULL) {
		fprintf(stderr, "Loading TestCaseInit function failed, testCaseInit == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return testCaseInit;
}

/*!
 * Loads function that deinitialises the executed test case from the
 * given test suite.
 *
 * \param suite Used test suite
 *
 * \return Function pointer (TestCaseInit) which points to loaded init function. NULL if function fails.
 */
TestCaseQuit
LoadTestCaseQuit(void *suite) {
	TestCaseQuit testCaseQuit = (TestCaseQuit) SDL_LoadFunction(suite, "_TestCaseQuit");
	if(testCaseQuit == NULL) {
		fprintf(stderr, "Loading TestCaseQuit function failed, testCaseQuit == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return testCaseQuit;
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
int
HandleTestReturnValue(int stat_lock)
{
	//! \todo rename to: HandleChildProcessReturnValue?
	int returnValue = -1;

	if(WIFEXITED(stat_lock)) {
		returnValue = WEXITSTATUS(stat_lock);
	} else if(WIFSIGNALED(stat_lock)) {
		int signal = WTERMSIG(stat_lock);
		fprintf(stderr, "FAILURE: test was aborted due to signal no %d\n", signal);
		returnValue = 1;
	}

	return returnValue;
}

/*!
 * Prints usage information
 */
void printUsage() {
	  printf("Usage: ./runner [--in-proc] [--help]\n");
	  printf("Options:\n");
	  printf(" --in-proc        Executes tests in-process\n");
	  printf(" --help           Print this help\n");
}

/*!
 * Parse command line arguments
 *
 * \param argc Count of command line arguments
 * \param argv Array of commond lines arguments
 */
void
ParseOptions(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc; ++i) {
      const char *arg = argv[i];
      if(SDL_strcmp(arg, "--in-proc") == 0) {
         execute_inproc = 1;
      }
      else if(SDL_strcmp(arg, "--help") == 0 || SDL_strcmp(arg, "-h") == 0) {
    	  printUsage();
    	  exit(0);
      } else {
    	  printf("runner: unknown command '%s'\n", arg);
    	  printUsage();
    	  exit(0);
      }
   }
}

/*!
 * Entry point for test runner
 *
 * \param argc Count of command line arguments
 * \param argv Array of commond lines arguments
 */
int
main(int argc, char *argv[])
{
	ParseOptions(argc, argv);

	// print: Testing against SDL version fuu (rev: bar) if verbose == true

	int failureCount = 0, passCount = 0;

	const Uint32 startTicks = SDL_GetTicks();

	char *testSuiteName = ScanForTestSuites();
	void *suite = LoadTestSuite(testSuiteName);
	TestCaseReference **tests = QueryTestCases(suite);

	TestCaseReference *reference = NULL;
	int counter = 0;

	for(reference = tests[counter]; reference; reference = tests[++counter]) {
		if(reference->enabled == TEST_DISABLED) {
			printf("Test %s (in %s) disabled. Omitting...\n", reference->name, testSuiteName);
		} else {
			char *testname = reference->name;

			printf("Running %s (in %s):\n", testname, testSuiteName);

			TestCaseInit testCaseInit = LoadTestCaseInit(suite);
			TestCaseQuit testCaseQuit = LoadTestCaseQuit(suite);
			TestCase test = (TestCase) LoadTestCase(suite, testname);

			int retVal = 1;
			if(execute_inproc) {
				testCaseInit();

				test(0x0);

				retVal = testCaseQuit();
			} else {
				int childpid = fork();
				if(childpid == 0) {
					testCaseInit();

					test(0x0);

					return testCaseQuit();
				} else {
					int stat_lock = -1;
					int child = wait(&stat_lock);

					retVal = HandleTestReturnValue(stat_lock);
				}
			}

			if(retVal) {
				failureCount++;
				printf("%s (in %s): FAILED\n", testname, testSuiteName);
			} else {
				passCount++;
				printf("%s (in %s): ok\n", testname, testSuiteName);
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
