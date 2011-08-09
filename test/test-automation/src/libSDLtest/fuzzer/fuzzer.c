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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "../../../include/SDL_test.h"

#include "fuzzer.h"


//! context for test-specific random number generator
static RND_CTX rndContext;

Uint64
GenerateExecKey(char *runSeed, char *suiteName,
				char *testName, int iterationNumber)
{
	if(runSeed == NULL) {
		fprintf(stderr, "Error: Incorrect runSeed given to GenerateExecKey function\n");
		return -1;
	}

	if(suiteName == NULL) {
		fprintf(stderr, "Error: Incorrect suiteName given to GenerateExecKey function\n");
		return -1;
	}

	if(testName == NULL) {
		fprintf(stderr, "Error: Incorrect testName given to GenerateExecKey function\n");
		return -1;
	}

	if(iterationNumber < 0) {
		fprintf(stderr, "Error: Incorrect iteration number given to GenerateExecKey function\n");
		return -1;
	}


	// Change to itoa
	char iterationString[16];
	memset(iterationString, 0, sizeof(iterationString));
	SDL_snprintf(iterationString, sizeof(iterationString) - 1, "%d", iterationNumber);

	// combine the parameters
	const Uint32 runSeedLength = strlen(runSeed);
	const Uint32 suiteNameLength = strlen(suiteName);
	const Uint32 testNameLength = strlen(testName);
	const Uint32 iterationStringLength = strlen(iterationString);

	// size of the entire + 3 for slashes and + 1 for '\0'
	const Uint32 entireString  = runSeedLength + suiteNameLength +
							  testNameLength + iterationStringLength + 3 + 1;

	char *buffer = SDL_malloc(entireString);
	if(!buffer) {
		return -1;
	}

	SDL_snprintf(buffer, entireString, "%s/%s/%s/%d", runSeed, suiteName,
			testName, iterationNumber);

	MD5_CTX md5Context;
	utl_md5Init(&md5Context);
	utl_md5Update(&md5Context, buffer, entireString);
	utl_md5Final(&md5Context);

	SDL_free(buffer);

	Uint64 *keys = (Uint64 *)md5Context.digest;
	Uint64 key = keys[0];

	return key;
}

void
InitFuzzer(Uint64 execKey)
{
	Uint32 a = (execKey >> 32)  & 0x00000000FFFFFFFF;
	Uint32 b = execKey & 0x00000000FFFFFFFF;
	utl_randomInit(&rndContext, a, b);
}

void
DeinitFuzzer()
{

}

Sint32
RandomInteger()
{
	return utl_randomInt(&rndContext);
}

Uint32
RandomPositiveInteger()
{
	return utl_randomInt(&rndContext);
}

Sint32
RandomIntegerInRange(Sint32 pMin, Sint32 pMax)
{
	Sint64 min = (Sint64) pMin, max = (Sint64) pMax;

	if(min > max) {
		Sint64 temp = min;
		min = max;
		max = temp;
	} else if(min == max) {
		return min;
	}

	Sint32 number = abs(utl_randomInt(&rndContext));

	return (number % ((max + 1) - min)) + min;
}

/*!
 * Generates boundary values between the given boundaries.
 * Boundary values are inclusive. See the examples below.
 * If boundary2 < boundary1, the values are swapped.
 * If boundary1 == boundary2, value of boundary1 will be returned
 *
 * Generating boundary values for Uint8:
 * BoundaryValues(sizeof(Uint8), 10, 20, True) -> [10,11,19,20]
 * BoundaryValues(sizeof(Uint8), 10, 20, False) -> [9,21]
 * BoundaryValues(sizeof(Uint8), 0, 15, True) -> [0, 1, 14, 15]
 * BoundaryValues(sizeof(Uint8), 0, 15, False) -> [16]
 * BoundaryValues(sizeof(Uint8), 0, 255, False) -> NULL
 *
 * Generator works the same for other types of unsigned integers.
 *
 * \param maxValue The biggest value that is acceptable for this data type.
 * 					For instance, for Uint8 -> 255, Uint16 -> 65536 etc.
 * \param pBoundary1 defines lower boundary
 * \param pBoundary2 defines upper boundary
 * \param validDomain Generate only for valid domain (for the data type)
 *
 * \param outBuffer The generated boundary values are put here
 * \param outBufferSize Size of outBuffer
 *
 * \returns NULL on error, outBuffer on success
 */
Uint64 *
GenerateUnsignedBoundaryValues(const Uint64 maxValue,
					Uint64 pBoundary1, Uint64 pBoundary2, SDL_bool validDomain,
					Uint64 *outBuffer, Uint32 *outBufferSize)
{
	Uint64 boundary1 = pBoundary1, boundary2 = pBoundary2;

	if(outBuffer != NULL) {
		SDL_free(outBuffer);
	}

	if(boundary1 > boundary2) {
		Uint64 temp = boundary1;
		boundary1 = boundary2;
		boundary2 = temp;
	}

	Uint64 tempBuf[8];
	memset(tempBuf, 0, 8 * sizeof(Uint64));

	Uint64 index = 0;

	if(boundary1 == boundary2) {
		tempBuf[index++] = boundary1;
	}
	else if(validDomain) {
		tempBuf[index++] = boundary1;

		if(boundary1 < UINT64_MAX)
			tempBuf[index++] = boundary1 + 1;

		tempBuf[index++] = boundary2 - 1;
		tempBuf[index++] = boundary2;
	}
	else {
		if(boundary1 > 0) {
			tempBuf[index++] = boundary1 - 1;
		}

		if(boundary2 < maxValue && boundary2 < UINT64_MAX) {
			tempBuf[index++] = boundary2 + 1;
		}
	}

	if(index == 0) {
		// There are no valid boundaries
		return NULL;
	}

	// Create the return buffer
	outBuffer = SDL_malloc(index * sizeof(Uint64));
	if(outBuffer == NULL) {
		return NULL;
	}

	SDL_memcpy(outBuffer, tempBuf, index * sizeof(Uint64));

	*outBufferSize = index;

	return outBuffer;
}

Uint8
RandomUint8BoundaryValue(Uint8 boundary1, Uint8 boundary2, SDL_bool validDomain)
{
	Uint64 *buffer = NULL;
	Uint32 size;

	// max value for Uint8
	const Uint64 maxValue = UINT8_MAX;

	buffer = GenerateUnsignedBoundaryValues(maxValue,
										(Uint64) boundary1, (Uint64) boundary2,
										validDomain, buffer, &size);
	if(buffer == NULL) {
		return -1; // Change to some better error value? What would be better?
	}

	Uint32 index = RandomInteger() % size;
	Uint8 retVal = (Uint8) buffer[index];

	SDL_free(buffer);

	return retVal;
}

Uint16
RandomUint16BoundaryValue(Uint16 boundary1, Uint16 boundary2, SDL_bool validDomain)
{
	Uint64 *buffer = NULL;
	Uint32 size;

	// max value for Uint16
	const Uint64 maxValue = UINT16_MAX;

	buffer = GenerateUnsignedBoundaryValues(maxValue,
										(Uint64) boundary1, (Uint64) boundary2,
										validDomain, buffer, &size);
	if(buffer == NULL) {
		return -1; // Change to some better error value? What would be better?
	}

	Uint32 index = RandomInteger() % size;
	Uint16 retVal = (Uint16) buffer[index];

	SDL_free(buffer);

	return retVal;
}

Uint32
RandomUint32BoundaryValue(Uint32 boundary1, Uint32 boundary2, SDL_bool validDomain)
{
	Uint64 *buffer = NULL;
	Uint32 size;

	// max value for Uint32
	const Uint64 maxValue = UINT32_MAX;

	buffer = GenerateUnsignedBoundaryValues(maxValue,
										(Uint64) boundary1, (Uint64) boundary2,
										validDomain, buffer, &size);
	if(buffer == NULL) {
		return -1; // Change to some better error value? What would be better?
	}

	Uint32 index = RandomInteger() % size;
	Uint32 retVal = (Uint32) buffer[index];

	SDL_free(buffer);

	return retVal;
}

Uint64
RandomUint64BoundaryValue(Uint64 boundary1, Uint64 boundary2, SDL_bool validDomain)
{
	Uint64 *buffer = NULL;
	Uint32 size;

	// max value for Uint64
	const Uint64 maxValue = UINT64_MAX;

	buffer = GenerateUnsignedBoundaryValues(maxValue,
										(Uint64) boundary1, (Uint64) boundary2,
										validDomain, buffer, &size);
	if(buffer == NULL) {
		return -1; // Change to some better error value? What would be better?
	}

	Uint32 index = RandomInteger() % size;
	Uint64 retVal = (Uint64) buffer[index];

	SDL_free(buffer);

	return retVal;
}

/*!
 * Generates boundary values between the given boundaries.
 * Boundary values are inclusive. See the examples below.
 * If boundary2 < boundary1, the values are swapped.
 * If boundary1 == boundary2, value of boundary1 will be returned
 *
 * Generating boundary values for Sint8:
 * <Add examples>
 *
 * Generator works the same for other types of signed integers.
 *
 * \paran minValue The smallest value  that is acceptable for this data type.
 *					For instance, for Uint8 -> -128, Uint16 -> -32,768 etc.
 * \param maxValue The biggest value that is acceptable for this data type.
 * 					For instance, for Uint8 -> 127, Uint16 -> 32767 etc.
 * \param pBoundary1 defines lower boundary
 * \param pBoundary2 defines upper boundary
 * \param validDomain Generate only for valid domain (for the data type)
 *
 * \param outBuffer The generated boundary values are put here
 * \param outBufferSize Size of outBuffer
 *
 * \returns NULL on error, outBuffer on success
 */
Uint64 *
GenerateSignedBoundaryValues(const Sint64 minValue, const Sint64 maxValue,
					Sint64 pBoundary1, Sint64 pBoundary2, SDL_bool validDomain,
					Sint64 *outBuffer, Uint32 *outBufferSize)
{
	Sint64 boundary1 = pBoundary1, boundary2 = pBoundary2;

	if(outBuffer != NULL) {
		SDL_free(outBuffer);
	}

	if(boundary1 > boundary2) {
		Sint64 temp = boundary1;
		boundary1 = boundary2;
		boundary2 = temp;
	}

	Sint64 tempBuf[8];
	memset(tempBuf, 0, 8 * sizeof(Sint64));

	Sint64 index = 0;

	if(boundary1 == boundary2) {
		tempBuf[index++] = boundary1;
	}
	else if(validDomain) {
		tempBuf[index++] = boundary1;

		if(boundary1 < LLONG_MAX)
			tempBuf[index++] = boundary1 + 1;

		if(boundary2 > LLONG_MIN)
			tempBuf[index++] = boundary2 - 1;

		tempBuf[index++] = boundary2;
	}
	else {
		if(boundary1 > minValue &&  boundary1 > LLONG_MIN) {
			tempBuf[index++] = boundary1 - 1;
		}

		if(boundary2 < maxValue && boundary2 < UINT64_MAX) {
			tempBuf[index++] = boundary2 + 1;
		}
	}

	if(index == 0) {
		// There are no valid boundaries
		return NULL;
	}

	// Create the return buffer
	outBuffer = SDL_malloc(index * sizeof(Sint64));
	if(outBuffer == NULL) {
		return NULL;
	}

	SDL_memcpy(outBuffer, tempBuf, index * sizeof(Sint64));

	*outBufferSize = index;

	return outBuffer;
}

Sint8
RandomSint8BoundaryValue(Sint8 boundary1, Sint8 boundary2, SDL_bool validDomain)
{
	Sint64 *buffer = NULL;
	Uint32 size;

	// min & max values for Sint8
	const Sint64 maxValue = CHAR_MAX;
	const Sint64 minValue = CHAR_MIN;

	buffer = GenerateSignedBoundaryValues(minValue, maxValue,
										(Sint64) boundary1, (Sint64) boundary2,
										validDomain, buffer, &size);
	if(buffer == NULL) {
		return CHAR_MIN;
	}

	Uint32 index = RandomInteger() % size;
	Sint8 retVal = (Sint8) buffer[index];

	SDL_free(buffer);

	return retVal;
}

Sint16
RandomSint16BoundaryValue(Sint16 boundary1, Sint16 boundary2, SDL_bool validDomain)
{
	Sint64 *buffer = NULL;
	Uint32 size;

	// min & max values for Sint16
	const Sint64 maxValue = SHRT_MAX;
	const Sint64 minValue = SHRT_MIN;

	buffer = GenerateSignedBoundaryValues(minValue, maxValue,
										(Sint64) boundary1, (Sint64) boundary2,
										validDomain, buffer, &size);
	if(buffer == NULL) {
		return SHRT_MIN;
	}

	Uint32 index = RandomInteger() % size;
	Sint16 retVal = (Sint16) buffer[index];

	SDL_free(buffer);

	return retVal;
}

Sint32
RandomSint32BoundaryValue(Sint32 boundary1, Sint32 boundary2, SDL_bool validDomain)
{
	Sint64 *buffer = NULL;
	Uint32 size;

	// min & max values for Sint32
	const Sint64 maxValue = INT_MAX;
	const Sint64 minValue = INT_MIN;

	buffer = GenerateSignedBoundaryValues(minValue, maxValue,
										(Sint64) boundary1, (Sint64) boundary2,
										validDomain, buffer, &size);
	if(buffer == NULL) {
		return INT_MIN;
	}

	Uint32 index = RandomInteger() % size;
	Sint32 retVal = (Sint32) buffer[index];

	SDL_free(buffer);

	return retVal;
}

Sint64
RandomSint64BoundaryValue(Sint64 boundary1, Sint64 boundary2, SDL_bool validDomain)
{
	Sint64 *buffer = NULL;
	Uint32 size;

	// min & max values for Sint64
	const Sint64 maxValue = LLONG_MAX;
	const Sint64 minValue = LLONG_MIN;

	buffer = GenerateSignedBoundaryValues(minValue, maxValue,
										(Sint64) boundary1, (Sint64) boundary2,
										validDomain, buffer, &size);
	if(buffer == NULL) {
		return LLONG_MIN;
	}

	Uint32 index = RandomInteger() % size;
	Sint64 retVal = (Sint64) buffer[index];

	SDL_free(buffer);

	return retVal;
}

char *
RandomAsciiString()
{
	return RandomAsciiStringWithMaximumLength(255);
}

char *
RandomAsciiStringWithMaximumLength(int maxSize)
{
	if(maxSize < 0) {
		return NULL;
	}

	int size = abs(RandomInteger()) % maxSize;
	char *string = SDL_malloc(size * sizeof(size));

	int counter = 0;
	for( ; counter < size; ++counter) {
		string[counter] = (char) RandomIntegerInRange(1, 127);
	}

	string[counter] = '\0';

	return string;
}
