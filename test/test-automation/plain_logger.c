
#ifndef _PLAIN_LOGGER
#define _PLAIN_LOGGER

#include <stdio.h>

#include <SDL/SDL.h>

#include "plain_logger.h"


static int indentLevel;

/*!
 * Prints out the output of the logger
 *
 * \param message The message to be printed out
 */
int
Output(const int currentIdentLevel, const char *message, ...)
{
	va_list list;
	va_start(list, message);

	int ident = 0;
	for( ; ident < currentIdentLevel; ++ident) {
		fprintf(stdout, "  "); // \todo make configurable?
	}

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
	Output(indentLevel, "\nRan %d tests in %0.5f seconds from %d suites.",
			testCount, totalRuntime, suiteCount);

	Output(indentLevel, "%d tests passed", testPassCount);
	Output(indentLevel, "%d tests failed", testFailCount);
}

void
PlainSuiteStarted(const char *suiteName, time_t eventTime)
{
	Output(indentLevel++, "Executing tests from %s", suiteName);
}

void
PlainSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           time_t endTime, double totalRuntime)
{
	Output(--indentLevel, "Suite executed. %d passed, %d failed and %d skipped. Total runtime %0.5f seconds",
			testsPassed, testsFailed, testsSkipped, totalRuntime);
	Output(indentLevel, "");
}

void
PlainTestStarted(const char *testName, const char *suiteName, const char *testDescription, time_t startTime)
{
	Output(indentLevel++, "%s (in %s) started", testName, suiteName);
}

void
PlainTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, double totalRuntime)
{
	if(testResult) {
		if(testResult == 2) {
			Output(--indentLevel, "%s: failed -> no assert", testName);
		} else {
			Output(--indentLevel, "%s: failed", testName);
		}
	} else {
		Output(--indentLevel, "%s: ok", testName);
	}
}

void
PlainAssert(const char *assertName, int assertResult, const char *assertMessage,
		time_t eventTime)
{
	const char *result = (assertResult) ? "passed" : "failed";
	Output(indentLevel, "%s: %s", assertName, assertMessage);
}

void
PlainAssertWithValues(const char *assertName, int assertResult, const char *assertMessage,
		int actualValue, int excpected, time_t eventTime)
{
	const char *result = (assertResult) ? "passed" : "failed";
	Output(indentLevel, "%s %d: %s", assertName, assertResult, assertMessage);
}

void
PlainAssertSummary(int numAsserts, int numAssertsFailed, int numAssertsPass, time_t eventTime)
{
	Output(indentLevel, "Assert summary: %d failed, %d passed (total: %d)",
			numAssertsFailed, numAssertsPass, numAsserts);
}

void
PlainLog(const char *logMessage, time_t eventTime)
{
	Output(indentLevel, "%s %d", logMessage, eventTime);
}

#endif
