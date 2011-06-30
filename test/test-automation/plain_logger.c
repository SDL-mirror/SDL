
#ifndef _PLAIN_LOGGER
#define _PLAIN_LOGGER

#include <stdio.h>

#include <SDL/SDL.h>

#include "plain_logger.h"


/*!
 * Prints out the output of the logger
 *
 * \param message The message to be printed out
 */
int
Output(const char *message, ...)
{
	va_list list;
	va_start(list, message);

	char buffer[1024];
	SDL_vsnprintf(buffer, sizeof(buffer), message, list);

	fprintf(stdout, "%s\n", buffer);
	fflush(stdout);
}

void
PlainRunStarted(int parameterCount, char *runnerParameters[], time_t eventTime,
				void *data)
{
	/*
    Output("Test run started with following parameters\n");

	int counter = 0;
	for(counter = 0; counter < parameterCount; counter++) {
		char *parameter = runnerParameters[counter];
		Output("\t%s", parameter);
	}
	*/
}

void
PlainRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
              time_t endTime, double totalRuntime)
{
	Output("\nRan %d tests in %0.5f seconds from %d suites.",
			testCount, totalRuntime, suiteCount);

	Output("%d tests passed", testPassCount);
	Output("%d tests failed", testFailCount);
}

void
PlainSuiteStarted(const char *suiteName, time_t eventTime)
{
	Output("Executing tests from %s", suiteName);
}

void
PlainSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           time_t endTime, double totalRuntime)
{
	Output("Suite executed. %d passed, %d failed and %d skipped", testsPassed, testsFailed, testsSkipped);
}

void
PlainTestStarted(const char *testName, const char *suiteName, const char *testDescription, time_t startTime)
{
	Output("%s (in %s) started", testName, suiteName);
}

void
PlainTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, double totalRuntime)
{
	if(testResult) {
		if(testResult == 2) {
			Output("%s: failed -> no assert");
		} else {
			Output("%s: failed");
		}
	} else {
		Output("%s: ok", testName);
	}
}

void
PlainAssert(const char *assertName, int assertResult, const char *assertMessage,
		time_t eventTime)
{
	const char *result = (assertResult) ? "passed" : "failed";
	Output("%s: %s", assertName, assertMessage);
}

void
PlainAssertWithValues(const char *assertName, int assertResult, const char *assertMessage,
		int actualValue, int excpected, time_t eventTime)
{
	const char *result = (assertResult) ? "passed" : "failed";
	Output("%s %d: %s", assertName, assertResult, assertMessage);
}

void
PlainAssertSummary(int numAsserts, int numAssertsFailed, int numAssertsPass, time_t eventTime)
{
	Output("Assert summary: %d failed, %d passed (total: %d)",
			numAssertsFailed, numAssertsPass, numAsserts);
}

void
PlainLog(const char *logMessage, time_t eventTime)
{
	Output("%s %d", logMessage, eventTime);
}

#endif
