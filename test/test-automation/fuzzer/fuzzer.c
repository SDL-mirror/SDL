
#include <stdio.h>
#include <stdlib.h>

#include "../SDL_test.h"

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

	const char *execKey = md5Context.digest;

	//printf("Debug: digest = %s\n", execKey);

	Uint64 key = execKey[8]  << 56 |
				 execKey[9]  << 48 |
				 execKey[10] << 40 |
				 execKey[11] << 32 |
				 execKey[12] << 24 |
				 execKey[13] << 16 |
				 execKey[14] << 8  |
				 execKey[15] << 0;

	return key;
}

void
InitFuzzer(Uint64 execKey)
{
	Uint32 a = (execKey >> 32)  & 0x00000000FFFFFFFF;
	Uint32 b = execKey & 0x00000000FFFFFFFF;

	//printf("Debug: execKey: %llx\n", execKey);
	//printf("Debug: a = %x - b = %x\n", a, b);

	utl_randomInit(&rndContext, a, b);
}

void
DeinitFuzzer()
{

}

int
RandomInteger()
{
	return utl_randomInt(&rndContext);
}

int
RandomPositiveInteger()
{
	return abs(utl_randomInt(&rndContext));
}

int
RandomIntegerInRange(int min, int max)
{
	if(min > max || (min - max) == 0) {
		return -1; // Doesn't really make sense to return -1 on error?
	}

	int number = utl_randomInt(&rndContext);
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

	int size = abs(RandomInteger) % maxSize;
	char *string = SDL_malloc(size * sizeof(size));

	int counter = 0;
	for( ; counter < size; ++counter) {
		string[counter] = (char) RandomIntegerInRange(1, 127);
	}

	string[counter] = '\0';

	return string;
}
