
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
RandomPositiveInteger()
{
	return abs(utl_randomInt(&rndContext3));
}

int
RandomIntegerInRange(int min, int max)
{
	if(min > max || (min - max) == 0) {
		return -1; // Doesn't really make sense to return -1 on error?
	}

	int number = utl_randomInt(&rndContext3);
	number = abs(number);

	return (number % ((max + 1) - min)) + min;
}

int
GenerateBoundaryValueForSize(const int size)
{
	if(size < 0) {
		return -1;
	}

	const int adjustment = RandomIntegerInRange(-1, 1);
	int retValue = (1 << (RandomPositiveInteger() % size)) + adjustment;

	return retValue;
}

int
RandomUint8BoundaryValue()
{
	return GenerateBoundaryValueForSize(8);
}

int
RandomInt8BoundaryValue()
{
	int value = GenerateBoundaryValueForSize(8);

	return (RandomPositiveInteger() % 2 == 0 ? value : -value);
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

	const int size = abs(RandomInteger) % maxSize;
	char *string = SDL_malloc(size * sizeof(size));

	int counter = 0;
	for( ; counter < size; ++counter) {
		char character = (char) RandomPositiveIntegerInRange(1, 127);
		string[counter] = character;
	}

	string[counter] = '\0';

	return string;
}
