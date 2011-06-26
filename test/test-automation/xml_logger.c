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

#include <SDL/SDL.h>

#include "xml.h"
#include "logger.h"

#include "xml_logger.h"

LogOutputFp logger;

void
XMLRunStarted(LogOutputFp outputFn, const char *runnerParameters, time_t eventTime)
{
	logger = outputFn;

	char *output = XMLOpenDocument("testlog");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("parameters");
	logger(output);
	SDL_free(output);

	output = XMLAddContent("Add: runner parameter");
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("parameters");
	logger(output);
	SDL_free(output);
}

void
XMLRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
            time_t endTime, time_t totalRuntime)
{
	char *output = XMLCloseDocument("testlog");
	logger(output);
	SDL_free(output);
}

void
XMLSuiteStarted(const char *suiteName, time_t eventTime)
{
	char *output = XMLOpenElement("suite");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("eventtime");
	logger(output);
	SDL_free(output);

	//XMLAddContent(evenTime);
	output = XMLCloseElement("eventtime");
	logger(output);
	SDL_free(output);
}

void
XMLSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           double endTime, time_t totalRuntime)
{
	char *output = XMLCloseElement("suite");
	logger(output);
	SDL_free(output);
}

void
XMLTestStarted(const char *testName, const char *suiteName, const char *testDescription, time_t startTime)
{
	char * output = XMLOpenElement("test");
	logger(output);
	SDL_free(output);

	//Attribute attribute = {"test", "value"};
	//XMLOpenElementWithAttribute("name", &attribute);
	output = XMLOpenElement("name");
	logger(output);
	SDL_free(output);

	output = XMLAddContent(testName);
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("name");
	logger(output);
	SDL_free(output);


	output = XMLOpenElement("description");
	logger(output);
	SDL_free(output);

	output = XMLAddContent(testDescription);
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("description");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("starttime");
	logger(output);
	SDL_free(output);

	//XMLAddContent(startTime);
	output = XMLCloseElement("starttime");
	logger(output);
	SDL_free(output);
}

void
XMLTestEnded(const char *testName, const char *suiteName,
          int testResult, time_t endTime, time_t totalRuntime)
{
	char *output = XMLOpenElement("result");
	logger(output);
	SDL_free(output);

	if(testResult) {
		if(testResult == 2) {
			output = XMLAddContent("failed -> no assert");
		} else {
			output = XMLAddContent("failed");
		}
		logger(output);
		SDL_free(output);
	} else {
		output = XMLAddContent("passed");
		logger(output);
		SDL_free(output);

	}
	output = XMLCloseElement("result");
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("test");
	logger(output);
	SDL_free(output);
}

void
XMLAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime)
{
	char *output = XMLOpenElement("assert");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("result");
	logger(output);
	SDL_free(output);

	output = XMLAddContent((assertResult) ? "pass" : "failure");
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("result");
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("assert");
	logger(output);
	SDL_free(output);
}

void
XMLAssertSummary(int numAsserts, int numAssertsFailed, int numAssertsPass)
{
	char *output = XMLOpenElement("assertSummary");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("assertCount");
	logger(output);
	SDL_free(output);

	//XMLAddContent() \todo add string conversion

	output = XMLCloseElement("assertCount");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("assertsPassed");
	logger(output);
	SDL_free(output);

	const int bufferSize = sizeof(int) * 8 + 1;
	//char buffer[bufferSize];
	char *buffer = SDL_malloc(bufferSize);
	memset(buffer, 'a', bufferSize);

	//SDL_vsnprintf(buffer, bufferSize, "%d", numAssertsPass);
	snprintf(buffer, sizeof(buffer), "%d", numAssertsPass);
	buffer[3] = 'a';
	//printf("DEBUG |Ê%s == %d of size %d", buffer, numAssertsPass, bufferSize);
	XMLAddContent(buffer);

	output = XMLCloseElement("assertsPassed");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("assertsFailed");
	logger(output);
	SDL_free(output);

	//XMLAddContent() \todo add string conversion

	output = XMLCloseElement("assertsFailed");
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("assertSummary");
	logger(output);
	SDL_free(output);
}

void
XMLLog(const char *logMessage, time_t eventTime)
{
	char *output = XMLOpenElement("log");
	logger(output);
	SDL_free(output);

	output = XMLAddContent(logMessage);
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("log");
	logger(output);
	SDL_free(output);
}

