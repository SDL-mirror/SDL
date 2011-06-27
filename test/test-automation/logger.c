
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
AssertWithValuesFp AssertWithValues = 0;
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
	AssertWithValues = XMLAssertWithValues;
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
	AssertWithValues = PlainAssertWithValues;
	AssertSummary = PlainAssertSummary;

	Log = PlainLog;
}


char *IntToString(const int integer) {
	static char buffer[sizeof(int) * 8 + 1]; // malloc might work better
	memset(buffer, 0, sizeof(buffer));

	SDL_snprintf(buffer, sizeof(buffer), "%d", integer);

	return buffer;
}


char *DoubleToString(const double decimal) {
	static char buffer[sizeof(double) * 8 + 1]; // malloc might work better
	memset(buffer, 0, sizeof(buffer));

	SDL_snprintf(buffer, sizeof(buffer), "%.5f", decimal);

	return buffer;
}

char *TimestampToString(const time_t timestamp) {
	static char buffer[1024];
	//char *buffer = SDL_malloc(1024);
	memset(buffer, 0, 1024);

	time_t copy = timestamp;

	struct tm *local = localtime(&copy);
	strftime(buffer, 1024, "%a %Y-%m-%d %H:%M:%S %Z", local);

	return buffer;
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
