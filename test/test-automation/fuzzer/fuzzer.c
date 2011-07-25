
#include "../SDL_test.h"

#include "fuzzer.h"


//! context for test-specific random number generator
static RND_CTX rndContext;

char *
GenerateExecKey(char *runSeed, char *suiteName,
				char *testName, int iterationNumber)
{
	if(runSeed == NULL || suiteName == NULL ||
	   testName == NULL || iterationNumber < 0) {
		fprintf(stderr, "Error: Incorrect parameter given to GenerateExecKey function\n");
		return NULL;
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
							  testNameLength + iterationStringLength + 3 + 1;

	char *buffer = SDL_malloc(entireString);
	if(!buffer) {
		return NULL;
	}

	SDL_snprintf(buffer, entireString, "%s/%s/%s/%d", runSeed, suiteName,
			testName, iterationNumber);

	//printf("Debug: %s", buffer);

	MD5_CTX md5Context;
	utl_md5Init(&md5Context);

	utl_md5Update(&md5Context, buffer, entireString);
	utl_md5Final(&md5Context);

	SDL_free(buffer);

	const int keyLength = SDL_strlen(md5Context.digest);
	char *key = SDL_malloc(keyLength);
	SDL_snprintf(key, keyLength, "%s", md5Context.digest);

	return key;
}

void
InitFuzzer(char *execKey)
{
	//int a = execKey[8,9,10,11];
	int a = execKey[8] | execKey[9] | execKey[10] | execKey[11];
	int b =  execKey[12] | execKey[13] | execKey[14] | execKey[15];

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
