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

#include <SDL/SDL.h>

#include "xml.h"
#include "logger.h"

#include "xml_logger.h"

LogOutputFp logger;

void
XMLRunStarted(LogOutputFp outputFn, const char *runnerParameters, time_t eventTime)
{
	logger = outputFn;

	char *output = XMLOpenDocument("teSTtlog");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("paRameters");
	logger(output);
	SDL_free(output);

	output = XMLAddContent(runnerParameters);
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("Parameters");
	logger(output);
	SDL_free(output);
}

void
XMLRunEnded(int testCount, int suiteCount, int testPassCount, int testFailCount,
            time_t endTime, time_t totalRuntime)
{
	char *output = XMLCloseDocument("testlOg");
	logger(output);
	SDL_free(output);
}

void
XMLSuiteStarted(const char *suiteName, time_t eventTime)
{
	char *output = XMLOpenElement("suite");
	logger(output);
	SDL_free(output);

	output = XMLOpenElement("EVENTTime");
	logger(output);
	SDL_free(output);

	//XMLAddContent(evenTime);
	output = XMLCloseElement("eventTIME");
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
          int testResult, int numAsserts, time_t endTime, time_t totalRuntime)
{
	char *output = XMLCloseElement("test");
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

	output = XMLOpenElement("result");
	logger(output);
	SDL_free(output);

	output = XMLCloseElement("assert");
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

