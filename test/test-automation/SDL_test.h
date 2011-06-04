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

#ifndef _SDL_TEST_H
#define _SDL_TEST_H

#include <SDL/SDL.h>

extern int _testReturnValue;
extern int _testAssertsFailed;
extern int _testAssertsPassed;

// \todo Should these be consts?
#define TEST_ENABLED  1
#define TEST_DISABLED 0

/*!
 * Holds information about a test case
 */
typedef struct TestCaseReference {
	char *name;         /*!< "Func2Stress" */
	char *description;  /*!< "This test beats the crap out of func2()" */
	int enabled;       /*!< Set to TEST_ENABLED or TEST_DISABLED */
	long requirements;  /*!< Set to TEST_REQUIRES_OPENGL, TEST_REQUIRES_AUDIO, ... */
} TestCaseReference;

/*! \fn _TestCaseInit
 *  Initialized the test case. Must be called at
 *  the beginning of every test case, before doing
 *  anything else.
 */
void _TestCaseInit();

/*! \fn _TestCaseQuit
 *  Deinitializes and exits the test case
 *
 * \return 0 if test succeeded, otherwise 1
 */
int _TestCaseQuit();

/*!
 * todo add comment
 */
void AssertEquals(Uint32 expected, Uint32 actual, char *message, ...);

/*!
 * todo add comment
 */
void AssertTrue(int condition, char *message, ...);

#endif
