
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
	printf("Executing tests in %s\n", suiteName);
}

void
PlainSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           double endTime, time_t totalRuntime)
{
	printf("Suite executed. %d passed, %d failed and %d skipped\n", testsPassed, testsFailed, testsSkipped);
}

void
PlainTestStarted(const char *testName, const char *suiteName, const char *testDescription, time_t startTime)
{
}

void
PlainTestEnded(const char *testName, const char *suiteName,
          int testResult, int numAsserts, time_t endTime, time_t totalRuntime)
{
	printf("Asserts:%d\n", numAsserts);
	printf("%s: ok\n", testName);
}

void
PlainAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime)
{
	const char *result = (assertResult) ? "passed" : "failed";
	printf("%s %s: %s\n", assertName, assertResult, assertMessage);
}

void
PlainLog(const char *logMessage, time_t eventTime)
{
}

#endif
