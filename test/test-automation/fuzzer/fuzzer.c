#include <stdio.h>
#include <stdlib.h>

#include <string.h>


#include "../SDL_test.h"

#include "fuzzer.h"


//! context for test-specific random number generator
RND_CTX rndContext3;

int
GenerateExecKey(CRC32_CTX crcContext, char *runSeed, char *suiteName,
				char *testName, int iterationNumber)
{
	if(runSeed == NULL || suiteName == NULL ||
	   testName == NULL || iterationNumber < 0) {
		fprintf(stderr, "Incorrect parameter given to GenerateExecKey function\n");
		return -1;
	}

	char iterationString[256];
	memset(iterationString, 0, sizeof(iterationString));

	snprintf(iterationString, sizeof(iterationString), "%d", iterationNumber);

	// combine the parameters
	const int runSeedLength = strlen(runSeed);
	const int suiteNameLength = strlen(suiteName);
	const int testNameLength = strlen(testName);
	const int iterationStringLength = strlen(iterationString);

	// size of the entire + 3 for slashes and + 1 for '\0'
	const int entireString  = runSeedLength + suiteNameLength +
							  testNameLength + iterationString + 3 + 1;

	int result = 0;

	/* Let's take a hash from the strings separately because
	 * it's really slow to calculate md5 or crc32 for a really long string
	 * like 'runSeed/testSuiteName/testName/iteration'
	 */
	MD5_CTX md5Context;
	utl_md5Init(&md5Context);

	utl_md5Update(&md5Context, runSeed, runSeedLength);
	utl_md5Update(&md5Context, suiteName, suiteNameLength);
	utl_md5Update(&md5Context, testName, testNameLength);
	utl_md5Update(&md5Context, iterationString, iterationStringLength);

	utl_md5Final(&md5Context);

	utl_crc32Calc(&crcContext, md5Context.digest, sizeof(md5Context.digest), &result);

	return result;
}

void
InitFuzzer(const int execKey)
{
	utl_randomInit(&rndContext3, globalExecKey, globalExecKey / 0xfafafafa);
}

void
DeinitFuzzer()
{

}

int
RandomInteger()
{
	return utl_randomInt(&rndContext3);
}

int
RandomPositiveIntegerInRange(int min, int max)
{
	int number = utl_randomInt(&rndContext3);
	number = abs(number);

	return (number % (max - min)) + min;
}

int
RandomBoundaryValue(const int max)
{
	// Note: somehow integrate with RandomInteger?
	// try to make more sensible & add new values
	int boundaryValues[] = {0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 65};
	int retValue = -1;

	do {
		int index = RandomPositiveIntegerInRange(0, 10);
		retValue = boundaryValues[index];

	} while( !(retValue <= max) );

	return retValue;
}


char *
RandomAsciiString()
{
	const int size = abs(RandomInteger);
	char *string = SDL_malloc(size * sizeof(size));

	int counter = 0;
	for( ; counter < size; ++counter) {
		char character = (char) RandomPositiveIntegerInRange(0, 127);
		string[counter] = character;
	}

	return string;
}
