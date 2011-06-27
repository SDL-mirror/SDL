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
#include <string.h>
#include <dirent.h>

#include <sys/types.h>

#include "SDL_test.h"
#include "logger.h"

//!< Function pointer to a test case function
typedef void (*TestCaseFp)(void *arg);
//!< Function pointer to a test case init function
typedef void (*TestCaseInitFp)(const int);
//!< Function pointer to a test case quit function
typedef int  (*TestCaseQuitFp)(void);


//!< Flag for executing tests in-process
static int execute_inproc = 0;
//!< Flag for only printing out the test names
static int only_print_tests = 0;
//!< Flag for executing only test with selected name
static int only_selected_test  = 0;
//!< Flag for executing only the selected test suite
static int only_selected_suite = 0;
//!< Flag for executing only tests that contain certain string in their name
static int only_tests_with_string = 0;
//!< Flag for enabling XML logging
static int xml_enabled = 0;


//!< Size of the test and suite name buffers
#define NAME_BUFFER_SIZE 1024
//!< Name of the selected test
char selected_test_name[NAME_BUFFER_SIZE];
//!< Name of the selected suite
char selected_suite_name[NAME_BUFFER_SIZE];

//!< substring of test case name
char testcase_name_substring[NAME_BUFFER_SIZE];

//! Default directory of the test suites
#define DEFAULT_TEST_DIRECTORY "tests/"


/*!
 * Holds information about test suite such as it's name
 * and pointer to dynamic library. Implemented as linked list.
 */
typedef struct TestSuiteReference {
	char *name; //!< test suite name
	void *library; //!< pointer to shared/dynamic library implementing the suite

	struct TestSuiteReference *next; //!< Pointer to next item in the list
} TestSuiteReference;


/*!
 * Holds information about the tests that will be executed.
 *
 * Implemented as linked list.
 */
typedef struct TestCaseItem {
	char *testName;
	char *suiteName;

	char *description;
	long requirements;
	long timeout;

	TestCaseInitFp testCaseInit;
	TestCaseFp testCase;
	TestCaseQuitFp testCaseQuit;

	struct TestCaseItem *next;
} TestCase;



/*! Some function prototypes. Add the rest of functions and move to runner.h */
TestCaseFp LoadTestCaseFunction(void *suite, char *testName);
TestCaseInitFp LoadTestCaseInitFunction(void *suite);
TestCaseQuitFp LoadTestCaseQuitFunction(void *suite);
TestCaseReference **QueryTestCaseReferences(void *library);


/*!
 * Scans the tests/ directory and returns the names
 * of the dynamic libraries implementing the test suites.
 *
 * Note: currently function assumes that test suites names
 * are in following format: libtestsuite.dylib or libtestsuite.so.
 *
 * Note: if only_selected_suite flags is non-zero, only the selected
 * test will be loaded.
 *
 * \param directoryName Name of the directory which will be scanned
 * \param extension What file extension is used with dynamic objects
 *
 * \return Pointer to TestSuiteReference which holds all the info about suites
 */
TestSuiteReference *
ScanForTestSuites(char *directoryName, char *extension)
{
	typedef struct dirent Entry;
	DIR *directory = opendir(directoryName);

	TestSuiteReference *suites = NULL;

	Entry *entry = NULL;
	if(!directory) {
		perror("Couldn't open test suite directory!");
	}

	while(entry = readdir(directory)) {
		if(strlen(entry->d_name) > 2) { // discards . and ..
			const char *delimiters = ".";
			char *name = strtok(entry->d_name, delimiters);
			char *ext = strtok(NULL, delimiters);

			// filter out all other suites but the selected test suite
			int ok = 1;
			if(only_selected_suite) {
				ok = SDL_strncmp(selected_suite_name, name, NAME_BUFFER_SIZE) == 0;
			}

			if(ok && SDL_strcmp(ext, extension)  == 0) {
				char buffer[NAME_BUFFER_SIZE];
				memset(buffer, 0, NAME_BUFFER_SIZE);

				strcat(buffer, directoryName);
				strcat(buffer, name);
				strcat(buffer, ".");
				strcat(buffer, ext);

				// create test suite reference
				TestSuiteReference *reference = (TestSuiteReference *) SDL_malloc(sizeof(TestSuiteReference));
				memset(reference, 0, sizeof(TestSuiteReference));

				int length = strlen(buffer) + 1; // + 1 for '\0'?
				reference->name = SDL_malloc(length * sizeof(char));

				strcpy(reference->name, buffer);

				reference->next = suites;
				suites = reference;

				//printf("Reference added to: %s\n", buffer);
			}
		}
	}

	closedir(directory);

	return suites;
}


/*!
 * Goes through the previously loaded test suites and
 * loads test cases from them. Test cases are filtered
 * during the process. Function will only return the
 * test cases which aren't filtered out.
 *
 * \param suites previously loaded test suites
 *
 * \return Test cases that survived filtering process.
 */
TestCase *
LoadTestCases(TestSuiteReference *suites)
{
	TestCase *testCases = NULL;

	TestSuiteReference *suiteReference = NULL;
	for(suiteReference = suites; suiteReference; suiteReference = suiteReference->next) {
		TestCaseReference **tests = QueryTestCaseReferences(suiteReference->library);

		TestCaseReference *testReference = NULL;
		int counter = 0;
		for(testReference = tests[counter]; testReference; testReference = tests[++counter]) {

			void *suite = suiteReference->library;

			// Load test case functions
			TestCaseInitFp testCaseInit = LoadTestCaseInitFunction(suiteReference->library);
			TestCaseQuitFp testCaseQuit = LoadTestCaseQuitFunction(suiteReference->library);
			TestCaseFp testCase = (TestCaseFp) LoadTestCaseFunction(suiteReference->library, testReference->name);

			// Do the filtering
			if(FilterTestCase(testReference)) {
				TestCase *item = SDL_malloc(sizeof(TestCase));
				memset(item, 0, sizeof(TestCase));

				item->testCaseInit = testCaseInit;
				item->testCase = testCase;
				item->testCaseQuit = testCaseQuit;

				// copy suite name
				int length = SDL_strlen(suiteReference->name) + 1;
				item->suiteName = SDL_malloc(length);
				strncpy(item->suiteName, suiteReference->name, length);

				// copy test name
				length = SDL_strlen(testReference->name) + 1;
				item->testName = SDL_malloc(length);
				strncpy(item->testName, testReference->name, length);

				// copy test description
				length = SDL_strlen(testReference->description) + 1;
				item->description = SDL_malloc(length);
				strncpy(item->description, testReference->description, length);

				item->requirements = testReference->requirements;
				item->timeout = testReference->timeout;

				// prepend the list
				item->next = testCases;
				testCases = item;

				//printf("Added test: %s\n", testReference->name);
			}
		}
	}

	return testCases;
}


/*!
 * Unloads the given TestCases. Frees all the resources
 * allocated for test cases.
 *
 * \param testCases Test cases to be deallocated
 */
void
UnloadTestCases(TestCase *testCases)
{
	TestCase *ref = testCases;
	while(ref) {
		SDL_free(ref->testName);
		SDL_free(ref->suiteName);
		SDL_free(ref->description);

		TestCase *temp = ref->next;
		SDL_free(ref);
		ref = temp;
	}

	testCases = NULL;
}


/*!
 * Filters a test case based on its properties in TestCaseReference and user
 * preference.
 *
 * \return Non-zero means test will be added to execution list, zero means opposite
 */
int
FilterTestCase(TestCaseReference *testReference)
{
	int retVal = 1;

	if(testReference->enabled == TEST_DISABLED) {
		retVal = 0;
	}

	if(only_selected_test) {
		if(SDL_strncmp(testReference->name, selected_test_name, NAME_BUFFER_SIZE) == 0) {
			retVal = 1;
		} else {
			retVal = 0;
		}
	}

	if(only_tests_with_string) {
		if(strstr(testReference->name, testcase_name_substring) != NULL) {
			retVal = 1;
		} else {
			retVal = 0;
		}
	}

	return retVal;
}


/*!
 * Loads test suite which is implemented as dynamic library.
 *
 * \param testSuiteName Name of the test suite which will be loaded
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
 * Goes through all the given TestSuiteReferences
 * and loads the dynamic libraries. Updates the suites
 * parameter on-the-fly and returns it.
 *
 * \param suites Suites that will be loaded
 *
 * \return Updated TestSuiteReferences with pointer to loaded libraries
 */
TestSuiteReference *
LoadTestSuites(TestSuiteReference *suites)
{
	TestSuiteReference *reference = NULL;
	for(reference = suites; reference; reference = reference->next) {
		reference->library = LoadTestSuite(reference->name);
	}

	return suites;
}


/*!
 * Unloads the given TestSuiteReferences. Frees all
 * the allocated resources including the dynamic libraries.
 *
 * \param suites TestSuiteReferences for deallocation process
 */
void
UnloadTestSuites(TestSuiteReference *suites)
{
	TestSuiteReference *ref = suites;
	while(ref) {
		SDL_free(ref->name);
		SDL_UnloadObject(ref->library);

		TestSuiteReference *temp = ref->next;
		SDL_free(ref);
		ref = temp;
	}

	suites = NULL;
}


/*!
 * Loads the test case references from the given test suite.

 * \param library Previously loaded dynamic library AKA test suite
 * \return Pointer to array of TestCaseReferences or NULL if function failed
 */
TestCaseReference **
QueryTestCaseReferences(void *library)
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
TestCaseFp
LoadTestCaseFunction(void *suite, char *testName)
{
	TestCaseFp test = (TestCaseFp) SDL_LoadFunction(suite, testName);
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
TestCaseInitFp
LoadTestCaseInitFunction(void *suite) {
	TestCaseInitFp testCaseInit = (TestCaseInitFp) SDL_LoadFunction(suite, "_TestCaseInit");
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
TestCaseQuitFp
LoadTestCaseQuitFunction(void *suite) {
	TestCaseQuitFp testCaseQuit = (TestCaseQuitFp) SDL_LoadFunction(suite, "_TestCaseQuit");
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
HandleChildProcessReturnValue(int stat_lock)
{
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
 * Executes a test case. Loads the test, executes it and
 * returns the tests return value to the caller.
 *
 * \param testItem The test case that will be executed
 * \return The return value of the test. Zero means success, non-zero failure.
 */
int
ExecuteTest(TestCase *testItem) {
	int retVal = 1;
	if(execute_inproc) {
		testItem->testCaseInit(xml_enabled);

		testItem->testCase(0x0);

		retVal = testItem->testCaseQuit();
	} else {
		int childpid = fork();
		if(childpid == 0) {
			testItem->testCaseInit(xml_enabled);

			testItem->testCase(0x0);

			exit(testItem->testCaseQuit());
		} else {
			int stat_lock = -1;
			int child = wait(&stat_lock);

			retVal = HandleChildProcessReturnValue(stat_lock);
		}
	}

	return retVal;
}


/*!
 * Prints usage information
 */
void
printUsage() {
	  printf("Usage: ./runner [--in-proc] [--suite SUITE] [--test TEST]\n");
	  printf("                [--name-contains SUBSTR] [--help]\n");
	  printf("Options:\n");
	  printf("     --in-proc                Executes tests in-process\n");
	  printf("     --show-tests             Prints out all the executable tests\n");
	  printf("     --xml             		Enables XML logger\n");
	  printf(" -t  --test TEST              Executes only tests with given name\n");
	  printf(" -ts --name-contains SUBSTR   Executes only tests that have given\n");
	  printf("                              substring in test name\n");
	  printf(" -s  --suite SUITE            Executes only the given test suite\n");

	  printf(" -h  --help                   Print this help\n");
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
      else if(SDL_strcmp(arg, "--show-tests") == 0) {
    	  only_print_tests = 1;
      }
      else if(SDL_strcmp(arg, "--xml") == 0) {
    	  xml_enabled = 1;
      }
      else if(SDL_strcmp(arg, "--test") == 0 || SDL_strcmp(arg, "-t") == 0) {
    	  only_selected_test = 1;
    	  char *testName = NULL;

    	  if( (i + 1) < argc)  {
    		  testName = argv[++i];
    	  }  else {
    		  printf("runner: test name is missing\n");
    		  printUsage();
    		  exit(1);
    	  }

    	  memset(selected_test_name, 0, NAME_BUFFER_SIZE);
    	  strcpy(selected_test_name, testName);
      }
      else if(SDL_strcmp(arg, "--name-contains") == 0 || SDL_strcmp(arg, "-ts") == 0) {
    	  only_tests_with_string = 1;
    	  char *substring = NULL;

    	  if( (i + 1) < argc)  {
    		  substring = argv[++i];
    	  }  else {
    		  printf("runner: substring of test name is missing\n");
    		  printUsage();
    		  exit(1);
    	  }

    	  memset(testcase_name_substring, 0, NAME_BUFFER_SIZE);
    	  strcpy(testcase_name_substring, substring);
      }
      else if(SDL_strcmp(arg, "--suite") == 0 || SDL_strcmp(arg, "-s") == 0) {
    	  only_selected_suite = 1;

    	  char *suiteName = NULL;
    	  if( (i + 1) < argc)  {
    		  suiteName = argv[++i];
    	  }  else {
    		  printf("runner: suite name is missing\n");
    		  printUsage();
    		  exit(1);
    	  }

    	  memset(selected_suite_name, 0, NAME_BUFFER_SIZE);
    	  strcpy(selected_suite_name, suiteName);
      }
      else if(SDL_strcmp(arg, "--help") == 0 || SDL_strcmp(arg, "-h") == 0) {
    	  printUsage();
    	  exit(0);
      }
      else {
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

	int totalTestfailureCount = 0, totalTestPassCount = 0;
	int testFailureCount = 0, testPassCount = 0, testSkipCount = 0;
	char *testSuiteName = NULL;
	int suiteCounter = 0;

#if defined(linux) || defined( __linux)
	char *extension = "so";
#else
	char *extension = "dylib";
#endif
	if(xml_enabled) {
		SetupXMLLogger();
	} else {
		SetupPlainLogger();
	}

	const Uint32 startTicks = SDL_GetTicks();

	TestSuiteReference *suites = ScanForTestSuites(DEFAULT_TEST_DIRECTORY, extension);
	suites = LoadTestSuites(suites);

	TestCase *testCases = LoadTestCases(suites);

	// if --show-tests option is given, only print tests and exit
	if(only_print_tests) {
		TestCase *testItem = NULL;
		for(testItem = testCases; testItem; testItem = testItem->next) {
			//! \todo This should be handled by the logging system?
			printf("%s (in %s)\n", testItem->testName, testItem->suiteName);
		}

		return 0;
	}

	RunStarted(argc, argv, time(0));

	char *currentSuiteName = NULL;

	int suiteStartTime = SDL_GetTicks();

	TestCase *testItem = NULL;
	for(testItem = testCases; testItem; testItem = testItem->next) {
		if(currentSuiteName == NULL) {
			currentSuiteName = testItem->suiteName;
			SuiteStarted(currentSuiteName, time(0));

			testFailureCount = testPassCount = 0;

			//suiteStartTime = SDL_GetTicks();

			suiteCounter++;
		}
		else if(strncmp(currentSuiteName, testItem->suiteName, NAME_BUFFER_SIZE) != 0) {
			const double suiteRuntime = (SDL_GetTicks() - suiteStartTime) / 1000.0f;

			SuiteEnded(testPassCount, testFailureCount, testSkipCount, time(0),
						suiteRuntime);

			currentSuiteName = testItem->suiteName;
			SuiteStarted(currentSuiteName, 0);

			testFailureCount = testPassCount = 0;

			//suiteStartTime = SDL_GetTicks();

			suiteCounter++;
		}

		TestStarted(testItem->testName, testItem->suiteName,
                    testItem->description, time(0));

		const Uint32 testTimeStart = SDL_GetTicks();

		int retVal = ExecuteTest(testItem);
		if(retVal) {
			totalTestfailureCount++;
			testFailureCount++;
		} else {
			totalTestPassCount++;
			testPassCount++;
		}

		const double testTotalRuntime = (SDL_GetTicks() - testTimeStart) / 1000.0f;

		TestEnded(testItem->testName, testItem->suiteName, retVal, time(0), testTotalRuntime);
	}

	if(currentSuiteName) {
		// \todo if no test are run, this will case incorrect nesting with
		// xml output
		SuiteEnded(testPassCount, testFailureCount, testSkipCount, time(0),
					(SDL_GetTicks() - suiteStartTime) / 1000.0f);
	}

	UnloadTestCases(testCases);
	UnloadTestSuites(suites);

	const Uint32 endTicks = SDL_GetTicks();
	const double totalRunTime = (endTicks - startTicks) / 1000.0f;

	RunEnded(totalTestPassCount + totalTestfailureCount, suiteCounter,
			 totalTestPassCount, totalTestfailureCount, time(0), totalRunTime);

	return 0;
}
