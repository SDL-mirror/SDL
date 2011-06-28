
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <SDL/SDL.h>

#include "logger.h"
#include "xml_logger.h"
#include "plain_logger.h"

//! Pointers to selected logger implementation
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

/*!
 * Sets up the XML logger
 */
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

/*!
 * Sets up the plain logger
 */
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
