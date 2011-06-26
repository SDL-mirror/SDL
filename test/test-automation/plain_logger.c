
#ifndef _PLAIN_LOGGER
#define _PLAIN_LOGGER

#include <stdio.h>

#include "plain_logger.h"


LogOutputFp logger = 0;

void
PlainRunStarted(LogOutputFp outputFn, const char *runnerParameters, time_t eventTime)
{
	logger = outputFn;
	logger("Test run started");
	logger("Given command line options: %s", "add options");
}

void
PlainRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
              time_t endTime, time_t totalRuntime)
{
	logger("Ran %d tests in %0.5f seconds.", testCount, totalRuntime);

	logger("%d tests passed", testPassCount);
	logger("%d tests failed", testFailCount);
}

void
PlainSuiteStarted(const char *suiteName, time_t eventTime)
{
	logger("Executing tests in %s", suiteName);
}

void
PlainSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           double endTime, time_t totalRuntime)
{
	logger("Suite executed. %d passed, %d failed and %d skipped", testsPassed, testsFailed, testsSkipped);
}

void
PlainTestStarted(const char *testName, const char *suiteName, const char *testDescription, time_t startTime)
{
	logger("test %s (in %s) started", testName, suiteName);
}

void
PlainTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, time_t totalRuntime)
{
	logger("%s: ok", testName);
}

void
PlainAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime)
{
	const char *result = (assertResult) ? "passed" : "failed";
	logger("%s %d: %s", assertName, assertResult, assertMessage);
}

void
PlainAssertSummary(int numAsserts, int numAssertsFailed, int numAssertsPass)
{
	logger("Asserts:%d", numAsserts);
}

void
PlainLog(const char *logMessage, time_t eventTime)
{
	logger("%s %d", logMessage, eventTime);
}

#endif
