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

#include <stdio.h> /* printf/fprintf */
#include <stdarg.h> /* va_list */

#include "SDL_test.h"

/*! \brief return value of test case. Non-zero value means that the test failed */
int _testReturnValue;

/*! \brief counts the failed asserts */
int _testAssertsFailed;

/*! \brief counts the passed asserts */
int _testAssertsPassed;

void
_TestCaseInit()
{
	_testReturnValue = 0;
	_testAssertsFailed = 0;
	_testAssertsPassed = 0;
}

int
_TestCaseQuit()
{
	//! \todo make the test fail, if it does not contain any asserts
	printf("Asserts: passed %d, failed %d\n", _testAssertsPassed, _testAssertsFailed);fflush(stdout);
	return _testReturnValue;
}

void
AssertEquals(Uint32 expected, Uint32 actual, char* message, ...)
{
   va_list args;
   char buf[256];

   if(expected != actual) {
      va_start( args, message );
      SDL_vsnprintf( buf, sizeof(buf), message, args );
      va_end( args );
      printf("Assert Equals failed: expected %d, got %d; %s\n", expected, actual, buf); fflush(stdout);
      _testReturnValue = 1;
      _testAssertsFailed++;
   } else {
      _testAssertsPassed++;
   }
}

void
AssertTrue(int condition, char *message, ...)
{
   va_list args;
   char buf[256];

   if (!condition) {
      va_start( args, message );
      SDL_vsnprintf( buf, sizeof(buf), message, args );
      va_end( args );

      printf("Assert IsTrue failed: %s\n", buf);
      _testReturnValue = 1;
      _testAssertsFailed++;
   } else {
      _testAssertsPassed++;
   }
}

#endif
