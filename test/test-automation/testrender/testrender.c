/**
 * Original code: automated SDL platform test written by Edgar Simo "bobbens"
 * Extended and updated by aschiffler at ferzkopp dot net
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../SDL_test.h"



/*!
 * Note: Port tests from "/test/automated/render" here
 *
 */

/* Test cases */
static const TestCaseReference test1 =
		(TestCaseReference){ "render_test", "rendery", TEST_ENABLED, 0, 0 };

/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, NULL
};

TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/**
 * @brief Document test case here
 */
int
render_test(void *arg)
{
	AssertPass("");
}
