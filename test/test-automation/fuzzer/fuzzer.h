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
void InitFuzzer(const int execKey);

/*!
 * Deinits the fuzzer (for a test)
 */
void DeinitFuzzer();


/*!
 * Returns random integer
 *
 * \returns Generated integer
 */
int RandomInteger();

/*!
 * Returns positive integer in range [min, max]
 *
 * \returns Generated integer
 */
int RandomPositiveIntegerInRange(int min, int max);

/*!
 * Generates random ASCII string
 *
 * \returns newly allocated random string
 */
char *RandomAsciiString();

/*!
 * Generates a random boundary value. Max is the biggest
 * value the function can return.
 *
 * \returns a boundary value
 */
int RandomBoundaryValue(const int max);

/*!
 * Generates execution key (used for random seed) for a test
 *
 * \param runSeed Seed of the harness
 * \param suiteName Test suite name
 * \param testName Test name
 * \param iteration Number of test iteration
 *
 * \return Generated execution key
 */
int GenerateExecKey(CRC32_CTX crcContext, char *runSeed, char *suiteName, char *testName, int interationNumber);

#endif
