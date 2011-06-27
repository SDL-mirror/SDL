#ifndef _PLAIN_LOGGER_H
#define _PLAIN_LOGGER_H

#include "logger.h"

void PlainRunStarted(const char *runnerParameters, time_t eventTime);

void PlainRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
                   time_t endTime, time_t totalRuntime);

void PlainSuiteStarted(const char *suiteName, time_t eventTime);

void PlainSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           double endTime, time_t totalRuntime);

void PlainTestStarted(const char *testName, const char *suiteName,
                      const char *testDescription, time_t startTime);

void PlainTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, time_t totalRuntime);


void PlainAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime);

void PlainAssertSummary(int numAsserts, int numAssertsFailed, int numAssertsPass);


void PlainLog(const char *logMessage, time_t eventTime);

#endif
