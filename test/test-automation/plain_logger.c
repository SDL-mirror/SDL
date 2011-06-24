
#ifndef _PLAIN_LOGGER
#define _PLAIN_LOGGER

#include <stdio.h>

#include "plain_logger.h"


LogOutputFp logger = 0;

void
PlainRunStarted(LogOutputFp outputFn, const char *runnerParameters, time_t eventTime)
{
	logger = outputFn;
}

void
PlainRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
              time_t endTime, time_t totalRuntime)
{
	// \todo add total number of tests, suites, pass/failure test count
}

void
PlainSuiteStarted(const char *suiteName, time_t eventTime)
{
	logger("Executing tests in %s\n", suiteName);
}

void
PlainSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           double endTime, time_t totalRuntime)
{
	logger("Suite executed. %d passed, %d failed and %d skipped\n", testsPassed, testsFailed, testsSkipped);
}

void
PlainTestStarted(const char *testName, const char *suiteName, const char *testDescription, time_t startTime)
{
}

void
PlainTestEnded(const char *testName, const char *suiteName,
          int testResult, int numAsserts, time_t endTime, time_t totalRuntime)
{
	logger("Asserts:%d\n", numAsserts);
	logger("%s: ok\n", testName);
}

void
PlainAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime)
{
	const char *result = (assertResult) ? "passed" : "failed";
	logger("%s %d: %s\n", assertName, assertResult, assertMessage);
}

void
PlainLog(const char *logMessage, time_t eventTime)
{
}

#endif
