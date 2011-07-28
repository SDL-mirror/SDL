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

#ifndef _FUZZER_H
#define _FUZZER_H

#include "utl_crc32.h"
#include "utl_md5.h"
#include "utl_random.h"


/*!
 * Inits the fuzzer for a test
 */
void InitFuzzer(int execKey);


/*!
 * Deinits the fuzzer (for a test)
 */
void DeinitFuzzer();


/*!
 * Returns a random integer
 *
 * \returns Generated integer
 */
int RandomInteger();


/*!
 * Returns a random positive integer
 *
 * \returns Generated integer
 */
int RandomPositiveInteger();


/*!
 * todo add markup
 */
int RandomUint8BoundaryValue();


/*!
 * todo add markup
 */
int RandomInt8BoundaryValue();


/*!
 * Returns integer in range [min, max]. Min and max
 * value can be negative values as long as min is smaller than max.
 * Min and max also can't be the same value.
 *
 * \returns Generated integer or ? in error
 */
int RandomIntegerInRange(int min, int max);


/*!
 * Generates random null-terminated string. The maximum length for
 * the string is 255 characters and it can contain ASCII characters
 * from 1 to 127.
 *
 * Note: Returned string needs to be deallocated.
 *
 * \returns newly allocated random string
 */
char *RandomAsciiString();


/*!
 * Generates random null-terminated string. The maximum length for
 * the string is defined by maxLenght parameter.
 * String can contain ASCII characters from 1 to 127.
 *
 * Note: Returned string needs to be deallocated.
 *
 * \param maxLength Maximum length of the generated string
 *
 * \returns newly allocated random string
 */
char *RandomAsciiStringWithMaximumLength(int maxLength);


/*!
 * Generates execution key (used for random seed) for a test
 *
 * \param runSeed Seed of the harness
 * \param suiteName Test suite name
 * \param testName Test name
 * \param iteration Number of test iteration
 *
 * \return Generated execution key as blob of 16 bytes. It needs be deallocated.
 * 			On error, returns NULL.
 */
int GenerateExecKey(char *runSeed, char *suiteName, char *testName, int interationNumber);

#endif
