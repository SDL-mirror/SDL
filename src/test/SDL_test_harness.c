/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL_config.h"

#include "SDL_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Invalid test name/description message format */
const char *SDLTest_InvalidNameFormat = "(Invalid)";

/* Log summary message format */
const char *SDLTest_LogSummaryFormat = "%s Summary: Total=%d Passed=%d Failed=%d Skipped=%d";

/* Final result message format */
const char *SDLTest_FinalResultFormat = ">>> %s '%s': %s\n";

/*! \brief Timeout for single test case execution */
static Uint32 SDLTest_TestCaseTimeout = 3600;

/**
 * Generates a random run seed string for the harness. The generated seed
 * will contain alphanumeric characters (0-9A-Z).
 *
 * Note: The returned string needs to be deallocated by the caller.
 *
 * \param length The length of the seed string to generate
 *
 * \returns The generated seed string
 */
char *
SDLTest_GenerateRunSeed(const int length)
{
	char *seed = NULL;
	SDLTest_RandomContext randomContext;
	int counter;

	// Sanity check input
	if (length <= 0) {
		SDLTest_LogError("The length of the harness seed must be >0.");
		return NULL;
	}

	// Allocate output buffer
	seed = (char *)SDL_malloc((length + 1) * sizeof(char));
	if (seed == NULL) {
		SDLTest_LogError("SDL_malloc for run seed output buffer failed.");
		return NULL;
	}

	// Generate a random string of alphanumeric characters
	SDLTest_RandomInitTime(&randomContext);
	for (counter = 0; counter < length - 1; ++counter) {
		unsigned int number = SDLTest_Random(&randomContext);
		char ch = (char) (number % (91 - 48)) + 48;
		if (ch >= 58 && ch <= 64) {
			ch = 65;
		}
		seed[counter] = ch;
	}
	seed[counter] = '\0';

	return seed;
}

/**
 * Generates an execution key for the fuzzer.
 *
 * \param runSeed		The run seed to use
 * \param suiteName		The name of the test suite
 * \param testName		The name of the test
 * \param iteration		The iteration count
 *
 * \returns The generated execution key to initialize the fuzzer with.
 *
 */
Uint64
SDLTest_GenerateExecKey(char *runSeed, char *suiteName, char *testName, int iteration)
{
	SDLTest_Md5Context md5Context;
	Uint64 *keys;
	char iterationString[16];
	Uint32 runSeedLength;
	Uint32 suiteNameLength;
	Uint32 testNameLength;
	Uint32 iterationStringLength;
	Uint32 entireStringLength;
	char *buffer;

	if (runSeed == NULL || strlen(runSeed)==0) {
		SDLTest_LogError("Invalid runSeed string.");
		return -1;
	}

	if (suiteName == NULL || strlen(suiteName)==0) {
		SDLTest_LogError("Invalid suiteName string.");
		return -1;
	}

	if (testName == NULL || strlen(testName)==0) {
		SDLTest_LogError("Invalid testName string.");
		return -1;
	}

	if (iteration <= 0) {
		SDLTest_LogError("Invalid iteration count.");
		return -1;
	}

	// Convert iteration number into a string
	memset(iterationString, 0, sizeof(iterationString));
	SDL_snprintf(iterationString, sizeof(iterationString) - 1, "%d", iteration);

	// Combine the parameters into single string
	runSeedLength = strlen(runSeed);
	suiteNameLength = strlen(suiteName);
	testNameLength = strlen(testName);
	iterationStringLength = strlen(iterationString);
	entireStringLength  = runSeedLength + suiteNameLength + testNameLength + iterationStringLength + 1;
	buffer = (char *)SDL_malloc(entireStringLength);
	if (buffer == NULL) {
		SDLTest_LogError("SDL_malloc failed to allocate buffer for execKey generation.");
		return 0;
	}
	SDL_snprintf(buffer, entireStringLength, "%s%s%s%d", runSeed, suiteName, testName, iteration);

	// Hash string and use half of the digest as 64bit exec key
	SDLTest_Md5Init(&md5Context);
	SDLTest_Md5Update(&md5Context, (unsigned char *)buffer, entireStringLength);
	SDLTest_Md5Final(&md5Context);
	SDL_free(buffer);
	keys = (Uint64 *)md5Context.digest;

	return keys[0];
}

/**
 * \brief Set timeout handler for test.
 *
 * Note: SDL_Init(SDL_INIT_TIMER) will be called if it wasn't done so before.
 *
 * \param timeout Timeout interval in seconds.
 * \param callback Function that will be called after timeout has elapsed.
 * 
 * \return Timer id or -1 on failure.
 */
SDL_TimerID
SDLTest_SetTestTimeout(int timeout, void (*callback)())
{
	Uint32 timeoutInMilliseconds;
	SDL_TimerID timerID;

	if (callback == NULL) {
		SDLTest_LogError("Timeout callback can't be NULL");
		return -1;
	}

	if (timeout < 0) {
		SDLTest_LogError("Timeout value must be bigger than zero.");
		return -1;
	}

	/* Init SDL timer if not initialized before */
	if (SDL_WasInit(SDL_INIT_TIMER) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_TIMER)) {
			SDLTest_LogError("Failed to init timer subsystem: %s", SDL_GetError());
			return -1;
		}
	}

	/* Set timer */
	timeoutInMilliseconds = timeout * 1000;
	timerID = SDL_AddTimer(timeoutInMilliseconds, (SDL_TimerCallback)callback, 0x0);
	if (timerID == 0) {
		SDLTest_LogError("Creation of SDL timer failed: %s", SDL_GetError());
		return -1;
	}

	return timerID;
}

void
SDLTest_BailOut()
{
	SDLTest_LogError("TestCaseTimeout timer expired. Aborting test run.");
	exit(TEST_ABORTED); // bail out from the test
}

/**
 * \brief Execute a test using the given execution key.
 *
 * \param testSuite Suite containing the test case.
 * \param testCase Case to execute.
 * \param execKey Execution key for the fuzzer.
 *
 * \returns Test case result.
 */
int
SDLTest_RunTest(SDLTest_TestSuiteReference *testSuite, SDLTest_TestCaseReference *testCase, Uint64 execKey)
{
	SDL_TimerID timer = 0;
	int testResult = 0;

	if (testSuite==NULL || testCase==NULL || testSuite->name==NULL || testCase->name==NULL)
	{
		SDLTest_LogError("Setup failure: testSuite or testCase references NULL");
		return TEST_RESULT_SETUP_FAILURE;
	}

	if (!testCase->enabled)
	{
		SDLTest_Log((char *)SDLTest_FinalResultFormat, "Test", testCase->name, "Skipped");
		return TEST_RESULT_SKIPPED;
	}

    // Initialize fuzzer
	SDLTest_FuzzerInit(execKey);

	// Reset assert tracker
	SDLTest_ResetAssertSummary();

	// Set timeout timer
	timer = SDLTest_SetTestTimeout(SDLTest_TestCaseTimeout, SDLTest_BailOut);

	// Maybe run suite initalizer function
	if (testSuite->testSetUp) {
		testSuite->testSetUp(0x0);
		if (SDLTest_AssertSummaryToTestResult() == TEST_RESULT_FAILED) {
			SDLTest_LogError((char *)SDLTest_FinalResultFormat, "Suite Setup", testSuite->name, "Failed");
			return TEST_RESULT_SETUP_FAILURE;
		}
	}

	// Run test case function
	testCase->testCase(0x0);
	testResult = SDLTest_AssertSummaryToTestResult();

	// Maybe run suite cleanup function (ignore failed asserts)
	if (testSuite->testTearDown) {
		testSuite->testTearDown(0x0);
	}

	// Cancel timeout timer
	if (timer) {
		SDL_RemoveTimer(timer);
	}

	// Report on asserts and fuzzer usage
	SDLTest_Log("Fuzzer invocations: %d", SDLTest_GetFuzzerInvocationCount());
	SDLTest_LogAssertSummary();

	return testResult;
}

/* Prints summary of all suites/tests contained in the given reference */
void SDLTest_LogTestSuiteSummary(SDLTest_TestSuiteReference *testSuites)
{
	int suiteCounter;
	int testCounter;
	SDLTest_TestSuiteReference *testSuite;
	SDLTest_TestCaseReference *testCase;

	// Loop over all suites
	suiteCounter = 0;
	while(&testSuites[suiteCounter]) {
		testSuite=&testSuites[suiteCounter];
		suiteCounter++;
		SDLTest_Log("Test Suite %i - %s\n", suiteCounter, 
			(testSuite->name) ? testSuite->name : SDLTest_InvalidNameFormat);

		// Loop over all test cases
		testCounter = 0;
		while(testSuite->testCases[testCounter])
		{
			testCase=(SDLTest_TestCaseReference *)testSuite->testCases[testCounter];
			testCounter++;
			SDLTest_Log("  Test Case %i - %s: %s", testCounter, 
				(testCase->name) ? testCase->name : SDLTest_InvalidNameFormat, 
				(testCase->description) ? testCase->description : SDLTest_InvalidNameFormat);
		}
	}
}

/* Gets a timer value in seconds */
float GetClock()
{
	float currentClock = (float)clock();
	return currentClock / (float)CLOCKS_PER_SEC;
}

/**
 * \brief Execute a test suite using the given run seend and execution key.
 *
 * \param testSuites Suites containing the test case.
 * \param userRunSeed Custom run seed provided by user, or NULL to autogenerate one.
 * \param userExecKey Custom execution key provided by user, or 0 to autogenerate one.
 * \param testIterations Number of iterations to run each test case.
 *
 * \returns Test run result; 0 when all tests passed, 1 if any tests failed.
 */
int
SDLTest_RunSuites(SDLTest_TestSuiteReference *testSuites[], char *userRunSeed, Uint64 userExecKey, int testIterations)
{
	int suiteCounter;
	int testCounter;
	int iterationCounter;
	SDLTest_TestSuiteReference *testSuite;
	SDLTest_TestCaseReference *testCase;
	char *runSeed = NULL;
	char *currentSuiteName;
	char *currentTestName;
	Uint64 execKey;
	float runStartSeconds;
	float suiteStartSeconds;
	float testStartSeconds;
	float runEndSeconds;
	float suiteEndSeconds;
	float testEndSeconds;
	int testResult = 0;
	int runResult = 0;
	Uint32 totalTestFailedCount = 0;
	Uint32 totalTestPassedCount = 0;
	Uint32 totalTestSkippedCount = 0;
	Uint32 testFailedCount = 0;
	Uint32 testPassedCount = 0;
	Uint32 testSkippedCount = 0;
	Uint32 countSum = 0;
	char *logFormat = (char *)SDLTest_LogSummaryFormat;

	// Sanitize test iterations
	if (testIterations < 1) {
		testIterations = 1;
	}

	// Generate run see if we don't have one already
	if (userRunSeed == NULL || strlen(userRunSeed) == 0) {
		runSeed = SDLTest_GenerateRunSeed(16);
		if (runSeed == NULL) {
			SDLTest_LogError("Generating a random seed failed");
			return 2;
		}
	}

	// Reset per-run counters
	totalTestFailedCount = 0;
	totalTestPassedCount = 0;
	totalTestSkippedCount = 0;

	// Take time - run start
	runStartSeconds = GetClock();

	// Log run with fuzzer parameters
	SDLTest_Log("::::: Test Run '%s' started\n", runSeed);

	// Loop over all suites
	suiteCounter = 0;
	while(testSuites[suiteCounter]) {
		testSuite=(SDLTest_TestSuiteReference *)testSuites[suiteCounter];
		suiteCounter++;

		// Reset per-suite counters
		testFailedCount = 0;
		testPassedCount = 0;
		testSkippedCount = 0;

		// Take time - suite start
		suiteStartSeconds = GetClock();

		// Log suite started
		currentSuiteName = (char *)((testSuite->name) ? testSuite->name : SDLTest_InvalidNameFormat);
		SDLTest_Log("===== Test Suite %i: %s started\n", 
			suiteCounter, 
			currentSuiteName);

		// Loop over all test cases
		testCounter = 0;
		while(testSuite->testCases[testCounter])
		{
			testCase=(SDLTest_TestCaseReference *)testSuite->testCases[testCounter];
			testCounter++;
			
			// Take time - test start
			testStartSeconds = GetClock();

			// Log test started
			currentTestName = (char *)((testCase->name) ? testCase->name : SDLTest_InvalidNameFormat);
			SDLTest_Log("----- Test Case %i: %s started", 
				testCounter, 
				currentTestName);
			SDLTest_Log("Test Description: %s", 
				(testCase->description) ? testCase->description : SDLTest_InvalidNameFormat);

			// Loop over all iterations
			iterationCounter = 0;
			while(iterationCounter < testIterations)
			{
				iterationCounter++;

				if(userExecKey != 0) {
					execKey = userExecKey;
				} else {
					execKey = SDLTest_GenerateExecKey(runSeed, testSuite->name, testCase->name, iterationCounter);
				}

				SDLTest_Log("Test Iteration %i: execKey %d", iterationCounter, execKey);
				testResult = SDLTest_RunTest(testSuite, testCase, execKey);

				if (testResult == TEST_RESULT_PASSED) {
					testPassedCount++;
					totalTestPassedCount++;
				} else if (testResult == TEST_RESULT_SKIPPED) {
					testSkippedCount++;
					totalTestSkippedCount++;
				} else {
					testFailedCount++;
					totalTestFailedCount++;
				}
			}

			// Take time - test end
			testEndSeconds = GetClock();

			SDLTest_Log("Test Case %s ended", currentTestName);

			// Log test runtime
			SDLTest_Log("Test runtime: %.1f sec", testEndSeconds - testStartSeconds);

			// Log final test result
			switch (testResult) {
				case TEST_RESULT_PASSED:
					SDLTest_Log((char *)SDLTest_FinalResultFormat, "Test", currentTestName, "Passed");
					break;
				case TEST_RESULT_FAILED:
					SDLTest_LogError((char *)SDLTest_FinalResultFormat, "Test", currentTestName, "Failed");
					break;
				case TEST_RESULT_NO_ASSERT:
					SDLTest_LogError((char *)SDLTest_FinalResultFormat,"Test", currentTestName, "No Asserts");
					break;
			}
		}

		// Take time - suite end
		suiteEndSeconds = GetClock();

		// Log suite runtime
		SDLTest_Log("Suite runtime: %.1f sec", suiteEndSeconds - suiteStartSeconds);

		// Log summary and final Suite result
	    countSum = testPassedCount + testFailedCount + testSkippedCount;
		if (testFailedCount == 0)
		{
			SDLTest_Log(logFormat, "Suite", countSum, testPassedCount, testFailedCount, testSkippedCount);
			SDLTest_Log((char *)SDLTest_FinalResultFormat, "Suite", currentSuiteName, "Passed");
		} 
		else 
		{
			SDLTest_LogError(logFormat, "Suite", countSum, testPassedCount, testFailedCount, testSkippedCount);
			SDLTest_LogError((char *)SDLTest_FinalResultFormat, "Suite", currentSuiteName, "Failed");
		}
	}

	// Take time - run end
	runEndSeconds = GetClock();

	// Log total runtime
	SDLTest_Log("Total runtime: %.1f sec", runEndSeconds - runStartSeconds);

	// Log summary and final run result
	countSum = totalTestPassedCount + totalTestFailedCount + totalTestSkippedCount;
	if (testFailedCount == 0)
	{
		runResult = 0;
		SDLTest_Log(logFormat, "Run", countSum, totalTestPassedCount, totalTestFailedCount, totalTestSkippedCount);
		SDLTest_Log((char *)SDLTest_FinalResultFormat, "Run", runSeed, "Passed");
	} 
	else 
	{
		runResult = 1;
		SDLTest_LogError(logFormat, "Run", countSum, totalTestPassedCount, totalTestFailedCount, totalTestSkippedCount);
		SDLTest_LogError((char *)SDLTest_FinalResultFormat, "Run", runSeed, "Failed");
	}

	return runResult;
}
