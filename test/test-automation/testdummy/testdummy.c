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

/*! \file
 * Dummy test suite for test runner. This can be used as a base for
 * writing new tests. Dummy suite also works as reference to using
 * various asserts and (possible) other utilities.
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../SDL_test.h"

// \todo add some helpful commenting for possible test writers?

/* Test case references */
static const TestCaseReference test1 =
		(TestCaseReference){ "dummycase1", "description", TEST_ENABLED, 0, 0};

static const TestCaseReference test2 =
		(TestCaseReference){ "dummycase2", "description", TEST_ENABLED, 0, 0};

static const TestCaseReference test3 =
		(TestCaseReference){ "dummycase3", "description", TEST_ENABLED, 0, 0};

/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, NULL
};


TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/* Test case functions */
void
dummycase1(void *arg)
{
	AssertEquals(5, 5, "Assert message");
}

void
dummycase2(void *arg)
{
	char *msg = "eello";
	//msg[0] = 'H';
	AssertTrue(1, "Assert message");
}

void
dummycase3(void *arg)
{
	AssertTrue(1, "Assert message");
}

