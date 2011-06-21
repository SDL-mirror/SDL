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

#ifndef _LOGGER_C
#define _LOGGER_C

#include "logger.h"

#include "xml.h"

#include <SDL/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*!
 * Prints the given message to stderr. Function adds nesting
 * to the output.
 *
 * \return Possible error value (\todo)
 */
int
LogGenericOutput(const char *message)
{
	/*
	int depth = indentDepth;
	while(depth--) {
		fprintf(stderr, " ");
	}
	*/

	fprintf(stderr, "%s\n", message);
	fflush(stderr);
}

void
RunStarted(LogOutputFp outputFn, const char *runnerParameters, time_t eventTime)
{
	XMLOpenDocument("testlog", outputFn);

	XMLOpenElement("parameters");
	XMLAddContent(runnerParameters);
	XMLCloseElement("parameters");
}

void
RunEnded(time_t endTime, time_t totalRuntime)
{
	XMLCloseDocument();
}

void
SuiteStarted(const char *suiteName, time_t eventTime)
{
	XMLOpenElement("suite");

	XMLOpenElement("eventTime");
	//XMLAddContent(evenTime);
	XMLCloseElement("eventTime");
}

void
SuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           double endTime, time_t totalRuntime)
{
	XMLCloseElement("suite");
}

void
TestStarted(const char *testName, const char *testDescription, time_t startTime)
{

}

void
TestEnded(const char *testName, const char *testDescription, int testResult,
          int numAsserts, time_t endTime, time_t totalRuntime)
{

}

void
Assert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime)
{

}

void
Log(const char *logMessage, time_t eventTime)
{

}


/*!
 * Main for testing the logger
 */
int
main(int argc, char *argv[])
{
	RunStarted(LogGenericOutput, "All the data from harness", 0);
	SuiteStarted("Suite data here", 0);
	SuiteEnded(0, 0, 0, 0.0f, 0);
	RunEnded(0, 0);

	return 0;
}

#endif
