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

#ifndef _SDL_TEST_C
#define _SDL_TEST_C

#include "SDL_test.h"

#include <stdlib.h>

/*! \brief return value of test case. Non-zero value means that the test failed */
static int _testReturnValue;

void
TestCaseInit()
{
	_testReturnValue = 0;
}

void
TestCaseQuit()
{
	exit(_testReturnValue);
}

void
AssertEquals(char *message, Uint32 expected, Uint32 actual)
{
	if(expected != actual) {
		printf("===============================\n");
		printf("Assert failed: %s\n", message);
		printf("Expected %d, got %d\n", expected, actual);
		printf("===============================\n");
		_testReturnValue = 1;
	}
}


#endif
