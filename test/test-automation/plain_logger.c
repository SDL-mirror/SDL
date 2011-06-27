
#ifndef _PLAIN_LOGGER
#define _PLAIN_LOGGER

#include <stdio.h>

#include <SDL/SDL.h>

#include "plain_logger.h"


/*!
 * Pritns out the output of the logger
 * \return Possible error value (\todo)
 */
int
Output(const char *message, ...)
{
	va_list list;
	va_start(list, message);

	char buffer[1024];
	SDL_vsnprintf(buffer, sizeof(buffer), message, list);

	fprintf(stderr, "%s\n", buffer);
	fflush(stderr);
}

void
PlainRunStarted(int parameterCount, char *runnerParameters[], time_t eventTime)
{
	Output("Test run started");
	Output("Given command line options: %s", "add options");
}

void
PlainRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
              time_t endTime, double totalRuntime)
{
	Output("Ran %d tests in %0.5f seconds.", testCount, totalRuntime);

	Output("%d tests passed", testPassCount);
	Output("%d tests failed", testFailCount);
}

void
PlainSuiteStarted(const char *suiteName, time_t eventTime)
{
	Output("Executing tests in %s", suiteName);
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
	Output("test %s (in %s) started", testName, suiteName);
}

void
PlainTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, double totalRuntime)
{
	Output("%s: ok", testName);
}

void
PlainAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime)
{
	const char *result = (assertResult) ? "passed" : "failed";
	Output("%s %d: %s", assertName, assertResult, assertMessage);
}

void
PlainAssertSummary(int numAsserts, int numAssertsFailed, int numAssertsPass, time_t eventTime)
{
	Output("Asserts:%d", numAsserts);
}

void
PlainLog(const char *logMessage, time_t eventTime)
{
	Output("%s %d", logMessage, eventTime);
}

#endif
