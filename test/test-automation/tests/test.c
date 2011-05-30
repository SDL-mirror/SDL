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

#ifndef _TEST_C
#define _TEST_C

#include <stdio.h>

#include <SDL/SDL.h>

#include "../SDL_test.h"

/* Test cases */
static const TestCaseReference test1 =
		(TestCaseReference){ "hello", "description", TEST_ENABLED, 0 };

static const TestCaseReference test2 =
		(TestCaseReference){ "hello2", "description", TEST_ENABLED, 0 };

static const TestCaseReference test3 =
		(TestCaseReference){ "hello3", "description", TEST_ENABLED, 0 };

/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, NULL
};


TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/* Test case functions */
int hello(void *arg)
{
	TestCaseInit();

	const char *revision = SDL_GetRevision();

	printf("Revision is %s\n", revision);
	AssertEquals("will fail", 3, 5);

	return TestCaseQuit();
}

int hello2(void *arg)
{
	TestCaseInit();

	char *msg = "eello";
	//msg[0] = 'H';

	return TestCaseQuit();
}

int hello3(void *arg)
{
	TestCaseInit();
	printf("hello3\n");

	AssertEquals("passes", 3, 3);

	return TestCaseQuit();
}

#endif
