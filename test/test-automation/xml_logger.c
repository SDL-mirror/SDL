/*
  Copyright (C) 2011 Markus Kauppila <markus.kauppila@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>

#include "xml.h"
#include "logger.h"

#include "xml_logger.h"

/*!
 *  Helper functions. Turns the given integer in to a string
 *
 *  \param integer The converted integer
 *  \returns Given integer as string
 */
char *IntToString(const int integer) {
	static char buffer[sizeof(int) * 8 + 1]; // malloc might work better
	memset(buffer, 0, sizeof(buffer));

	SDL_snprintf(buffer, sizeof(buffer), "%d", integer);

	return buffer;
}

/*!
 *  Helper functions. Turns the given double value in to a string
 *
 *  \param integer The converted double value
 *  \returns Given double value as string
 */
char *DoubleToString(const double decimal) {
	static char buffer[sizeof(double) * 8 + 1]; // malloc might work better
	memset(buffer, 0, sizeof(buffer));

	SDL_snprintf(buffer, sizeof(buffer), "%.5f", decimal);

	return buffer;
}

/*!
 * Converts unix timestamp to it's ascii presentation
 *
 * \param timestamp Timestamp
 * \return Ascii presentation
 */
char *TimestampToString(const time_t timestamp) {
	static char buffer[1024];
	//char *buffer = SDL_malloc(1024);
	memset(buffer, 0, 1024);

	time_t copy = timestamp;

	struct tm *local = localtime(&copy);
	strftime(buffer, 1024, "%a %Y-%m-%d %H:%M:%S %Z", local);

	return buffer;
}

static int indentLevel;

//! Constants for XMLOuputters EOL parameter
#define YES 1
#define NO 0

/*! Controls printing the identation in relation to line breaks */
static int prevEOL = YES;

/*
 * Prints out the given xml element etc.
 *
 * \param  identLevel the indent level of the message
 * \param EOL will it print end of line character or not
 * \param the XML element itself
 *
 */
void XMLOutputter(const int currentIdentLevel, int EOL,  const char *message) {
	int ident = 0;
	for( ; ident < currentIdentLevel && prevEOL; ++ident) {
		printf("\t");
	}

	prevEOL = EOL;

	if(EOL) {
		printf("%s\n", message);
	} else {
		printf("%s", message);
	}

	fflush(stdout);
}

void
XMLRunStarted(int parameterCount, char *runnerParameters[], time_t eventTime)
{
	char *output = XMLOpenDocument("testlog");
	XMLOutputter(indentLevel++, YES, output);
	SDL_free(output);

	output = XMLOpenElement("parameters");
	XMLOutputter(indentLevel++, YES, output);
	SDL_free(output);

	int counter = 0;
	for(counter = 0; counter < parameterCount; counter++) {
		char *parameter = runnerParameters[counter];

		output = XMLOpenElement("parameter");
		XMLOutputter(indentLevel++, NO, output);
		SDL_free(output);

		output = XMLAddContent(parameter);
		XMLOutputter(indentLevel, NO, output);
		SDL_free(output);

		output = XMLCloseElement("parameter");
		XMLOutputter(--indentLevel, YES, output);
		SDL_free(output);
	}

	output = XMLCloseElement("parameters");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLOpenElement("eventTime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(TimestampToString(eventTime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("eventTime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}

void
XMLRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
            time_t endTime, double totalRuntime)
{
	// log suite count
	char *output = XMLOpenElement("numSuites");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(suiteCount));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("numSuites");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log test count
	output = XMLOpenElement("numTest");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(testCount));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("numTest");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log passed test count
	output = XMLOpenElement("numPassedTests");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(testPassCount));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("numPassedTests");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log failed test count
	output = XMLOpenElement("numFailedTests");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(testFailCount));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("numFailedTests");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);


	// log end timte
	output = XMLOpenElement("endTime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(TimestampToString(endTime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("endTime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log total runtime
	output = XMLOpenElement("totalRuntime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(DoubleToString(totalRuntime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("totalRuntime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLCloseDocument("testlog");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}

void
XMLSuiteStarted(const char *suiteName, time_t eventTime)
{
	char *output = XMLOpenElement("suite");
	XMLOutputter(indentLevel++, YES, output);
	SDL_free(output);

	output = XMLOpenElement("eventTime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(TimestampToString(eventTime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("eventTime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}

void
XMLSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           time_t endTime, double totalRuntime)
{
	// log tests passed
	char *output = XMLOpenElement("testsPassed");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(testsPassed));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("testsPassed");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log tests failed
	output = XMLOpenElement("testsFailed");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(testsFailed));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("testsFailed");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log tests skipped
	output = XMLOpenElement("testsSkipped");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(testsSkipped));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("testsSkipped");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log tests skipped
	output = XMLOpenElement("endTime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(TimestampToString(endTime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("endTime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log total runtime
	output = XMLOpenElement("totalRuntime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(DoubleToString(totalRuntime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("totalRuntime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);


	output = XMLCloseElement("suite");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}

void
XMLTestStarted(const char *testName, const char *suiteName,
			  const char *testDescription, time_t startTime)
{
	char * output = XMLOpenElement("test");
	XMLOutputter(indentLevel++, YES, output);
	SDL_free(output);

	//Attribute attribute = {"test", "value"};
	//XMLOpenElementWithAttribute("name", &attribute);
	output = XMLOpenElement("name");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(testName);
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("name");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLOpenElement("description");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(testDescription);
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("description");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLOpenElement("startTime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(TimestampToString(startTime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("startTime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}

void
XMLTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, double totalRuntime)
{
	char *output = XMLOpenElement("result");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	if(testResult) {
		if(testResult == 2) {
			output = XMLAddContent("failed -> no assert");
		} else {
			output = XMLAddContent("failed");
		}
		XMLOutputter(indentLevel, NO, output);
		SDL_free(output);
	} else {
		output = XMLAddContent("passed");
		XMLOutputter(indentLevel, NO, output);
		SDL_free(output);
	}

	output = XMLCloseElement("result");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log total runtime
	output = XMLOpenElement("endTime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(TimestampToString(endTime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("endTime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log total runtime
	output = XMLOpenElement("totalRuntime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(DoubleToString(totalRuntime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("totalRuntime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);


	//! \todo add endTime and TotalRuntime

	output = XMLCloseElement("test");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}

void
XMLAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime)
{
	char *output = XMLOpenElement("assert");
	XMLOutputter(indentLevel++, YES, output);
	SDL_free(output);

	// log assert result
	output = XMLOpenElement("result");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent((assertResult) ? "pass" : "failure");
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("result");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log assert message
	output = XMLOpenElement("message");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(assertMessage);
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("message");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log event time
	output = XMLOpenElement("eventTime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(TimestampToString(eventTime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("eventTime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLCloseElement("assert");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}

void
XMLAssertSummary(int numAsserts, int numAssertsFailed,
				 int numAssertsPass, time_t eventTime)
{
	char *output = XMLOpenElement("assertSummary");
	XMLOutputter(indentLevel++, YES, output);
	SDL_free(output);

	output = XMLOpenElement("assertCount");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(numAsserts));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("assertCount");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLOpenElement("assertsPassed");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(numAssertsPass));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("assertsPassed");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLOpenElement("assertsFailed");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(IntToString(numAsserts));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("assertsFailed");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLCloseElement("assertSummary");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}

void
XMLLog(const char *logMessage, time_t eventTime)
{
	char *output = XMLOpenElement("log");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	// log message
	output = XMLOpenElement("message");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(logMessage);
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("message");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	// log eventTime
	output = XMLOpenElement("eventTime");
	XMLOutputter(indentLevel++, NO, output);
	SDL_free(output);

	output = XMLAddContent(TimestampToString(eventTime));
	XMLOutputter(indentLevel, NO, output);
	SDL_free(output);

	output = XMLCloseElement("eventTime");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);

	output = XMLCloseElement("log");
	XMLOutputter(--indentLevel, YES, output);
	SDL_free(output);
}
