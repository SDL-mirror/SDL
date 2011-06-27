#ifndef _PLAIN_LOGGER_H
#define _PLAIN_LOGGER_H

#include "logger.h"

void PlainRunStarted(int parameterCount, char *runnerParameters[], time_t eventTime);

void PlainRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
                   time_t endTime, double totalRuntime);

void PlainSuiteStarted(const char *suiteName, time_t eventTime);

void PlainSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           time_t endTime, double totalRuntime);

void PlainTestStarted(const char *testName, const char *suiteName,
                      const char *testDescription, time_t startTime);

void PlainTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, double totalRuntime);


void PlainAssert(const char *assertName, int assertResult, const char *assertMessage,
				time_t eventTime);

void PlainAssertWithValues(const char *assertName, int assertResult, const char *assertMessage,
		int actualValue, int excpected, time_t eventTime);

void PlainAssertSummary(int numAsserts, int numAssertsFailed, int numAssertsPass, time_t eventTime);


void PlainLog(const char *logMessage, time_t eventTime);

#endif
