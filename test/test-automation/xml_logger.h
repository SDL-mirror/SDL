#ifndef _XML_LOGGER_H
#define _XML_LOGGER_H

#include "logger.h"

void XMLRunStarted(int parameterCount, char *runnerParameters[], time_t eventTime);

void XMLRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
                 time_t endTime, double totalRuntime);

void XMLSuiteStarted(const char *suiteName, time_t eventTime);

void XMLSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           time_t endTime, double totalRuntime);

void XMLTestStarted(const char *testName, const char *suiteName, const char *testDescription, time_t startTime);

void XMLTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, double totalRuntime);

void XMLAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime);

void XMLAssertSummary(int numAsserts, int numAssertsFailed, int numAssertsPass, time_t eventTime);

void XMLLog(const char *logMessage, time_t eventTime);

#endif
