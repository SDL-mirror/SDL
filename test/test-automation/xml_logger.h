#ifndef _XML_LOGGER_H
#define _XML_LOGGER_H

#include "logger.h"

void XMLRunStarted(LogOutputFp outputFn, const char *runnerParameters, time_t eventTime);

void XMLRunEnded(time_t endTime, time_t totalRuntime);

void XMLSuiteStarted(const char *suiteName, time_t eventTime);

void XMLSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           double endTime, time_t totalRuntime);

void XMLTestStarted(const char *testName, const char *testDescription, time_t startTime);

void XMLTestEnded(const char *testName, const char *testDescription,
          int testResult, int numAsserts, time_t endTime, time_t totalRuntime);

void XMLAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime);

void XMLLog(const char *logMessage, time_t eventTime);

#endif
