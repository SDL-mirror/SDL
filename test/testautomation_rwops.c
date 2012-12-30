/**
 * Automated SDL_RWops test.
 *
 * Original code written by Edgar Simo "bobbens"
 * Ported by Markus Kauppila (markus.kauppila@gmail.com)
 * Updated for SDL_test by aschiffler at ferzkopp dot net
 *
 * Released under Public Domain.
 */

/* quiet windows compiler warnings */
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#include "SDL.h"
#include "SDL_test.h"

/* ================= Test Case Implementation ================== */

const char* RWopsReadTestFilename = "rwops_read";
const char* RWopsWriteTestFilename = "rwops_write";

static const char RWopsHelloWorldTestString[] = "Hello World!";
static const char RWopsHelloWorldCompString[] = "Hello World!";

/* Fixture */

void
RWopsSetUp(void *arg)
{
	int fileLen = SDL_strlen(RWopsHelloWorldTestString);
	FILE *handle;
	int writtenLen;
	int result;

	/* Clean up from previous runs (if any); ignore errors */
	remove(RWopsReadTestFilename);
	remove(RWopsWriteTestFilename);

	/* Create a test file */
	handle = fopen(RWopsReadTestFilename, "w");
	SDLTest_AssertCheck(handle != NULL, "Verify creation of file '%s' returned non NULL handle", RWopsReadTestFilename);
    if (handle == NULL) return;

	/* Write some known test into it */
	writtenLen = (int)fwrite(RWopsHelloWorldTestString, 1, fileLen, handle);
	SDLTest_AssertCheck(fileLen == writtenLen, "Verify number of written bytes, expected %i, got %i", fileLen, writtenLen);
	result = fclose(handle);
	SDLTest_AssertCheck(result == 0, "Verify result from fclose, expected 0, got %i", result);

	SDLTest_AssertPass("Creation of test file completed");
}

void
RWopsTearDown(void *arg)
{
	int result;
	
	/* Remove the created files to clean up; ignore errors for write filename */
	result = remove(RWopsReadTestFilename);
	SDLTest_AssertCheck(result == 0, "Verify result from remove(%s), expected 0, got %i", RWopsReadTestFilename, result);
	remove(RWopsWriteTestFilename);

	SDLTest_AssertPass("Cleanup of test files completed");
}

/**
 * @brief Makes sure parameters work properly. Local helper function.
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_RWseek
 * http://wiki.libsdl.org/moin.cgi/SDL_RWread
 */
void 
_testGenericRWopsValidations(SDL_RWops *rw, int write)
{
   char buf[sizeof(RWopsHelloWorldTestString)];
   Sint64 i;
   int seekPos = SDLTest_RandomIntegerInRange(4, 8);

   /* Clear buffer */
   SDL_zero(buf);

   /* Set to start. */
   i = SDL_RWseek(rw, 0, RW_SEEK_SET );
   SDLTest_AssertPass("Call to SDL_RWseek succeeded");
   SDLTest_AssertCheck(i == (Sint64)0, "Verify seek to 0 with SDL_RWseek (RW_SEEK_SET), expected 0, got %i", i);

   /* Test write. */
   i = SDL_RWwrite(rw, RWopsHelloWorldTestString, sizeof(RWopsHelloWorldTestString)-1, 1);
   SDLTest_AssertPass("Call to SDL_RWwrite succeeded");
   if (write) {
		SDLTest_AssertCheck(i == (Sint64)1, "Verify result of writing one byte with SDL_RWwrite, expected 1, got %i", i);
   }
   else {
		SDLTest_AssertCheck(i != (Sint64)1, "Verify result of writing with SDL_RWwrite, expected !=1, got %i", i);
   }

   /* Test seek to random position */
   i = SDL_RWseek( rw, seekPos, RW_SEEK_SET );
   SDLTest_AssertPass("Call to SDL_RWseek succeeded");
   SDLTest_AssertCheck(i == (Sint64)seekPos, "Verify seek to %i with SDL_RWseek (RW_SEEK_SET), expected %i, got %i", seekPos, seekPos, i);

   /* Test seek back to start */
   i = SDL_RWseek(rw, 0, RW_SEEK_SET );
   SDLTest_AssertPass("Call to SDL_RWseek succeeded");
   SDLTest_AssertCheck(i == (Sint64)0, "Verify seek to 0 with SDL_RWseek (RW_SEEK_SET), expected 0, got %i", i);

   /* Test read */
   i = SDL_RWread( rw, buf, 1, sizeof(RWopsHelloWorldTestString)-1 );
   SDLTest_AssertPass("Call to SDL_RWread succeeded");
   SDLTest_AssertCheck(
	   i == (Sint64)(sizeof(RWopsHelloWorldTestString)-1), 
	   "Verify result from SDL_RWread, expected %i, got %i",
	   sizeof(RWopsHelloWorldTestString)-1,
	   i);
   SDLTest_AssertCheck(
	   SDL_memcmp(buf, RWopsHelloWorldTestString, sizeof(RWopsHelloWorldTestString)-1 ) == 0, 
	   "Verify read bytes match expected string, expected '%s', got '%s'", RWopsHelloWorldTestString, buf);

   /* More seek tests. */
   i = SDL_RWseek( rw, -4, RW_SEEK_CUR );
   SDLTest_AssertPass("Call to SDL_RWseek(...,-4,RW_SEEK_CUR) succeeded");
   SDLTest_AssertCheck(
	   i == (Sint64)(sizeof(RWopsHelloWorldTestString)-5), 
	   "Verify seek to -4 with SDL_RWseek (RW_SEEK_CUR), expected %i, got %i",
	   sizeof(RWopsHelloWorldTestString)-5,
	   i);

   i = SDL_RWseek( rw, -1, RW_SEEK_END );
   SDLTest_AssertPass("Call to SDL_RWseek(...,-1,RW_SEEK_END) succeeded");
   SDLTest_AssertCheck(
	   i == (Sint64)(sizeof(RWopsHelloWorldTestString)-2), 
	   "Verify seek to -1 with SDL_RWseek (RW_SEEK_END), expected %i, got %i",
	   sizeof(RWopsHelloWorldTestString)-2,
	   i);
}

/*!
 * Negative test for SDL_RWFromFile parameters
 *
 * \sa http://wiki.libsdl.org/moin.cgi/SDL_RWFromFile
 *
 */
int
rwops_testParamNegative (void)
{
   SDL_RWops *rwops;

   /* These should all fail. */
   rwops = SDL_RWFromFile(NULL, NULL);
   SDLTest_AssertPass("Call to SDL_RWFromFile(NULL, NULL) succeeded");
   SDLTest_AssertCheck(rwops == NULL, "Verify SDL_RWFromFile(NULL, NULL) returns NULL");

   rwops = SDL_RWFromFile(NULL, "ab+");
   SDLTest_AssertPass("Call to SDL_RWFromFile(NULL, \"ab+\") succeeded");
   SDLTest_AssertCheck(rwops == NULL, "Verify SDL_RWFromFile(NULL, \"ab+\") returns NULL");

   rwops = SDL_RWFromFile(NULL, "sldfkjsldkfj");
   SDLTest_AssertPass("Call to SDL_RWFromFile(NULL, \"sldfkjsldkfj\") succeeded");
   SDLTest_AssertCheck(rwops == NULL, "Verify SDL_RWFromFile(NULL, \"sldfkjsldkfj\") returns NULL");

   rwops = SDL_RWFromFile("something", "");
   SDLTest_AssertPass("Call to SDL_RWFromFile(\"something\", \"\") succeeded");
   SDLTest_AssertCheck(rwops == NULL, "Verify SDL_RWFromFile(\"something\", \"\") returns NULL");

   rwops = SDL_RWFromFile("something", NULL);
   SDLTest_AssertPass("Call to SDL_RWFromFile(\"something\", NULL) succeeded");
   SDLTest_AssertCheck(rwops == NULL, "Verify SDL_RWFromFile(\"something\", NULL) returns NULL");

   return TEST_COMPLETED;
}

/**
 * @brief Tests opening from memory.
 *
 * \sa http://wiki.libsdl.org/moin.cgi/SDL_RWFromMem
 */
int
rwops_testMem (void)
{
   char mem[sizeof(RWopsHelloWorldTestString)];
   SDL_RWops *rw;

   /* Clear buffer */
   SDL_zero(mem);

   /* Open */
   rw = SDL_RWFromMem(mem, sizeof(RWopsHelloWorldTestString)-1);
   SDLTest_AssertPass("Call to SDL_RWFromMem() succeeded");
   SDLTest_AssertCheck(rw != NULL, "Verify opening memory with SDL_RWFromMem does not return NULL");

   /* Bail out if NULL */
   if (rw == NULL) return TEST_ABORTED;

   /* Run generic tests */
   _testGenericRWopsValidations(rw, 1);

   /* Close */
   SDL_RWclose(rw);
   SDLTest_AssertPass("Call to SDL_RWclose() succeeded");
   SDL_FreeRW(rw);
   SDLTest_AssertPass("Call to SDL_FreeRW() succeeded");

   return TEST_COMPLETED;
}


/**
 * @brief Tests opening from memory.
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_RWFromConstMem
 */
int 
rwops_testConstMem (void)
{
   SDL_RWops *rw;

   /* Open handle */
   rw = SDL_RWFromConstMem( RWopsHelloWorldCompString, sizeof(RWopsHelloWorldCompString)-1 );
   SDLTest_AssertPass("Call to SDL_RWFromConstMem() succeeded");
   SDLTest_AssertCheck(rw != NULL, "Verify opening memory with SDL_RWFromConstMem does not return NULL");

   /* Bail out if NULL */
   if (rw == NULL) return TEST_ABORTED;

   /* Run generic tests */
   _testGenericRWopsValidations( rw, 0 );

   /* Close handle */
   SDL_RWclose(rw);
   SDLTest_AssertPass("Call to SDL_RWclose() succeeded");
   SDL_FreeRW( rw );
   SDLTest_AssertPass("Call to SDL_FreeRW() succeeded");

  return TEST_COMPLETED;
}


/**
 * @brief Tests reading from file.
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_RWFromFile
 * http://wiki.libsdl.org/moin.cgi/SDL_FreeRW
 */
int
rwops_testFileRead(void)
{
   SDL_RWops *rw;

   /* Read test. */
   rw = SDL_RWFromFile(RWopsReadTestFilename, "r");
   SDLTest_AssertPass("Call to SDL_RWFromFile(..,\"r\") succeeded");
   SDLTest_AssertCheck(rw != NULL, "Verify opening file with SDL_RWFromFile in read mode does not return NULL");

   // Bail out if NULL
   if (rw == NULL) return TEST_ABORTED;

   /* Run generic tests */
   _testGenericRWopsValidations( rw, 0 );

   /* Close handle */
   SDL_RWclose(rw);
   SDLTest_AssertPass("Call to SDL_RWclose() succeeded");
   SDL_FreeRW( rw );
   SDLTest_AssertPass("Call to SDL_FreeRW() succeeded");

   return TEST_COMPLETED;
}

/**
 * @brief Tests writing from memory.
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_RWFromFile
 * http://wiki.libsdl.org/moin.cgi/SDL_FreeRW
 */
int
rwops_testFileWrite(void)
{
   SDL_RWops *rw;

   /* Write test. */
   rw = SDL_RWFromFile(RWopsWriteTestFilename, "w+");
   SDLTest_AssertPass("Call to SDL_RWFromFile(..,\"w+\") succeeded");
   SDLTest_AssertCheck(rw != NULL, "Verify opening file with SDL_RWFromFile in write mode does not return NULL");

   // Bail out if NULL
   if (rw == NULL) return TEST_ABORTED;

   /* Run generic tests */
   _testGenericRWopsValidations( rw, 1 );

   /* Close handle */
   SDL_RWclose(rw);
   SDLTest_AssertPass("Call to SDL_RWclose() succeeded");
   SDL_FreeRW( rw );
   SDLTest_AssertPass("Call to SDL_FreeRW() succeeded");

   return TEST_COMPLETED;
}


/**
 * @brief Tests reading from file handle
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_RWFromFP
 * http://wiki.libsdl.org/moin.cgi/SDL_FreeRW
 *
 */
int
rwops_testFPRead(void)
{
   FILE *fp;
   SDL_RWops *rw;

   /* Run read tests. */
   fp = fopen(RWopsReadTestFilename, "r");
   SDLTest_AssertCheck(fp != NULL, "Verify handle from opening file '%s' in read mode is not NULL", RWopsReadTestFilename);

   /* Bail out if NULL */
   if (fp == NULL) return TEST_ABORTED;

   /* Open */
   rw = SDL_RWFromFP( fp, SDL_TRUE );
   SDLTest_AssertPass("Call to SDL_RWFromFP() succeeded");
   SDLTest_AssertCheck(rw != NULL, "Verify opening file with SDL_RWFromFP in read mode does not return NULL");

   /* Bail out if NULL */
   if (rw == NULL) return TEST_ABORTED;

   /* Run generic tests */
   _testGenericRWopsValidations( rw, 0 );

   /* Close handle */
   SDL_RWclose(rw);
   SDLTest_AssertPass("Call to SDL_RWclose() succeeded");
   SDL_FreeRW( rw );
   SDLTest_AssertPass("Call to SDL_FreeRW() succeeded");
   fclose(fp);

   return TEST_COMPLETED;
}


/**
 * @brief Tests writing to file handle
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_RWFromFP
 * http://wiki.libsdl.org/moin.cgi/SDL_FreeRW
 *
 */
int
rwops_testFPWrite(void)
{
   FILE *fp;
   SDL_RWops *rw;

   /* Run write tests. */
   fp = fopen(RWopsWriteTestFilename, "w+");
   SDLTest_AssertCheck(fp != NULL, "Verify handle from opening file '%s' in write mode is not NULL", RWopsWriteTestFilename);

   /* Bail out if NULL */
   if (fp == NULL) return TEST_ABORTED;

   /* Open */
   rw = SDL_RWFromFP( fp, SDL_TRUE );
   SDLTest_AssertPass("Call to SDL_RWFromFP() succeeded");
   SDLTest_AssertCheck(rw != NULL, "Verify opening file with SDL_RWFromFP in write mode does not return NULL");

   /* Bail out if NULL */
   if (rw == NULL) return TEST_ABORTED;

   /* Run generic tests */
   _testGenericRWopsValidations( rw, 1 );

   /* Close handle */
   SDL_RWclose(rw);
   SDLTest_AssertPass("Call to SDL_RWclose() succeeded");
   SDL_FreeRW( rw );
   SDLTest_AssertPass("Call to SDL_FreeRW() succeeded");
   fclose(fp);

   return TEST_COMPLETED;
}

/* ================= Test References ================== */

/* RWops test cases */
static const SDLTest_TestCaseReference rwopsTest1 =
		{ (SDLTest_TestCaseFp)rwops_testParamNegative, "rwops_testParamNegative", "Negative test for SDL_RWFromFile parameters", TEST_ENABLED };

static const SDLTest_TestCaseReference rwopsTest2 =
		{ (SDLTest_TestCaseFp)rwops_testMem, "rwops_testMem", "Tests opening from memory", TEST_ENABLED };

static const SDLTest_TestCaseReference rwopsTest3 =
		{ (SDLTest_TestCaseFp)rwops_testConstMem, "rwops_testConstMem", "Tests opening from (const) memory", TEST_ENABLED };

static const SDLTest_TestCaseReference rwopsTest4 =
		{ (SDLTest_TestCaseFp)rwops_testFileRead, "rwops_testFileRead", "Tests reading from a file", TEST_ENABLED };

static const SDLTest_TestCaseReference rwopsTest5 =
		{ (SDLTest_TestCaseFp)rwops_testFileWrite, "rwops_testFileWrite", "Test writing to a file", TEST_ENABLED };

static const SDLTest_TestCaseReference rwopsTest6 =
		{ (SDLTest_TestCaseFp)rwops_testFPRead, "rwops_testFPRead", "Test reading from file pointer", TEST_ENABLED };

static const SDLTest_TestCaseReference rwopsTest7 =
		{ (SDLTest_TestCaseFp)rwops_testFPWrite, "rwops_testFPWrite", "Test writing to file pointer", TEST_ENABLED };

/* Sequence of RWops test cases */
static const SDLTest_TestCaseReference *rwopsTests[] =  {
	&rwopsTest1, &rwopsTest2, &rwopsTest3, &rwopsTest4, &rwopsTest5, &rwopsTest6, &rwopsTest7, NULL
};

/* RWops test suite (global) */
SDLTest_TestSuiteReference rwopsTestSuite = {
	"RWops",
	RWopsSetUp,
	rwopsTests,
	RWopsTearDown
};
