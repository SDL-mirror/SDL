/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL_config.h"

#include "SDL_test.h"

// TODO: port over harness

/**
 * Generates a random run seed string for the harness. The generated seed
 * will contain alphanumeric characters (0-9A-Z).
 *
 * Note: The returned string needs to be deallocated by the caller.
 *
 * \param length The length of the seed string to generate
 *
 * \returns The generated seed string
 */
char *
SDLTest_GenerateRunSeed(const int length)
{
	char *seed = NULL;
	SDLTest_RandomContext randomContext;
	int counter;

	// Sanity check input
	if (length <= 0) {
		SDLTest_LogError("The length of the harness seed must be >0.");
		return NULL;
	}

	// Allocate output buffer
	seed = (char *)SDL_malloc((length + 1) * sizeof(char));
	if (seed == NULL) {
		SDLTest_LogError("SDL_malloc for run seed output buffer failed.");
		return NULL;
	}

	// Generate a random string of alphanumeric characters
	SDLTest_RandomInitTime(&randomContext);
	for (counter = 0; counter < length - 1; ++counter) {
		unsigned int number = SDLTest_Random(&randomContext);
		char ch = (char) (number % (91 - 48)) + 48;
		if (ch >= 58 && ch <= 64) {
			ch = 65;
		}
		seed[counter] = ch;
	}
	seed[counter] = '\0';

	return seed;
}

/**
 * Generates an execution key for the fuzzer.
 *
 * \param runSeed		The run seed to use
 * \param suiteName		The name of the test suite
 * \param testName		The name of the test
 * \param iteration		The iteration count
 *
 * \returns The generated execution key to initialize the fuzzer with.
 *
 */
Uint64
SDLTest_GenerateExecKey(char *runSeed, char *suiteName, char *testName, int iteration)
{
	SDLTest_Md5Context md5Context;
	Uint64 *keys;
	char iterationString[16];
	Uint32 runSeedLength;
	Uint32 suiteNameLength;
	Uint32 testNameLength;
	Uint32 iterationStringLength;
	Uint32 entireStringLength;
	char *buffer;

	if (runSeed == NULL || strlen(runSeed)==0) {
		SDLTest_LogError("Invalid runSeed string.");
		return -1;
	}

	if (suiteName == NULL || strlen(suiteName)==0) {
		SDLTest_LogError("Invalid suiteName string.");
		return -1;
	}

	if (testName == NULL || strlen(testName)==0) {
		SDLTest_LogError("Invalid testName string.");
		return -1;
	}

	if (iteration <= 0) {
		SDLTest_LogError("Invalid iteration count.");
		return -1;
	}

	// Convert iteration number into a string
	memset(iterationString, 0, sizeof(iterationString));
	SDL_snprintf(iterationString, sizeof(iterationString) - 1, "%d", iteration);

	// Combine the parameters into single string
	runSeedLength = strlen(runSeed);
	suiteNameLength = strlen(suiteName);
	testNameLength = strlen(testName);
	iterationStringLength = strlen(iterationString);
	entireStringLength  = runSeedLength + suiteNameLength + testNameLength + iterationStringLength + 1;
	buffer = (char *)SDL_malloc(entireStringLength);
	if (buffer == NULL) {
		SDLTest_LogError("SDL_malloc failed to allocate buffer for execKey generation.");
		return 0;
	}
	SDL_snprintf(buffer, entireStringLength, "%s%s%s%d", runSeed, suiteName, testName, iteration);

	// Hash string and use half of the digest as 64bit exec key
	SDLTest_Md5Init(&md5Context);
	SDLTest_Md5Update(&md5Context, (unsigned char *)buffer, entireStringLength);
	SDLTest_Md5Final(&md5Context);
	SDL_free(buffer);
	keys = (Uint64 *)md5Context.digest;

	return keys[0];
}
