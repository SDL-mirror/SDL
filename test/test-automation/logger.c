
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <SDL/SDL.h>

#include "logger.h"
#include "xml_logger.h"
#include "plain_logger.h"

// Pointers to selected logger implementation
RunStartedFp RunStarted = 0;
RunEndedFp RunEnded = 0;
SuiteStartedFp SuiteStarted = 0;
SuiteEndedFp SuiteEnded = 0;
TestStartedFp TestStarted = 0;
TestEndedFp TestEnded = 0;
AssertFp Assert = 0;
AssertSummaryFp AssertSummary = 0;
LogFp Log = 0;

int
SetupXMLLogger()
{
	RunStarted = XMLRunStarted;
	RunEnded = XMLRunEnded;

	SuiteStarted = XMLSuiteStarted;
	SuiteEnded = XMLSuiteEnded;

	TestStarted = XMLTestStarted;
	TestEnded = XMLTestEnded;

	Assert = XMLAssert;
	AssertSummary = XMLAssertSummary;

	Log = XMLLog;
}

int
SetupPlainLogger()
{
	RunStarted = PlainRunStarted;
	RunEnded = PlainRunEnded;

	SuiteStarted = PlainSuiteStarted;
	SuiteEnded = PlainSuiteEnded;

	TestStarted = PlainTestStarted;
	TestEnded = PlainTestEnded;

	Assert = PlainAssert;
	AssertSummary = PlainAssertSummary;

	Log = PlainLog;
}

#if 0
/*!
 * Test app for logging functionality
 */
int
main(int argc, char *argv[])
{
	int xml_enabled = 1;

	if(xml_enabled) {
		SetupXMLLogger();
	} else {
		SetupPlainLogger();
	}

	RunStarted(Output, "some_<data_>here&here", 0);
	SuiteStarted("Suite data here", 0);

	TestStarted("test1", "suite", "desc", 0);
	TestEnded("test1", "suite", 0, 0, 0, 0);

	SuiteEnded(0, 0, 0, 0.0f, 0);
	RunEnded(0, 0, 0, 0, 0, 0);

	return 0;
}
#endif
