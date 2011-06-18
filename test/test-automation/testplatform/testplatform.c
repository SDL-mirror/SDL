/**
 * Original code: automated SDL platform test written by Edgar Simo "bobbens"
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../SDL_test.h"

/* Test cases */
static const TestCaseReference test1 =
		(TestCaseReference){ "platform_testTypes", "description", TEST_ENABLED, 0, 0 };

static const TestCaseReference test2 =
		(TestCaseReference){ "platform_testEndianessAndSwap", "description", TEST_ENABLED, 0, 0 };

static const TestCaseReference test3 =
		(TestCaseReference){ "platform_testGetFunctions", "description", TEST_ENABLED, 0, 0 };

static const TestCaseReference test4 =
		(TestCaseReference){ "platform_testHasFunctions", "description", TEST_ENABLED, 0, 0 };

/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, &test4, NULL
};

TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/**
 * @brief Compare sizes of types.
 *
 * @note Watcom C flags these as Warning 201: "Unreachable code" if you just
 *  compare them directly, so we push it through a function to keep the
 *  compiler quiet.  --ryan.
 */
static int _compareSizeOfType( size_t sizeoftype, size_t hardcodetype )
{
    return sizeoftype != hardcodetype;
}

/**
 * @brief Tests type sizes.
 */
int platform_testTypes(void *arg)
{
   int ret;

   ret = _compareSizeOfType( sizeof(Uint8), 1 );
   AssertTrue( ret == 0, "sizeof(Uint8) = %lu instead of 1", sizeof(Uint8) );

   ret = _compareSizeOfType( sizeof(Uint16), 2 );
   AssertTrue( ret == 0, "sizeof(Uint16) = %lu instead of 2", sizeof(Uint16) );

   ret = _compareSizeOfType( sizeof(Uint32), 4 );
   AssertTrue( ret == 0, "sizeof(Uint32) = %lu instead of 4", sizeof(Uint32) );

   ret = _compareSizeOfType( sizeof(Uint64), 8 );
   AssertTrue( ret == 0, "sizeof(Uint64) = %lu instead of 8", sizeof(Uint64) );
}

/**
 * @brief Tests platform endianness and SDL_SwapXY functions.
 */
int platform_testEndianessAndSwap(void *arg)
{
    int real_byteorder;
    Uint16 value = 0x1234;
    Uint16 value16 = 0xCDAB;
    Uint16 swapped16 = 0xABCD;
    Uint32 value32 = 0xEFBEADDE;
    Uint32 swapped32 = 0xDEADBEEF;

    Uint64 value64, swapped64;
    value64 = 0xEFBEADDE;
    value64 <<= 32;
    value64 |= 0xCDAB3412;
    swapped64 = 0x1234ABCD;
    swapped64 <<= 32;
    swapped64 |= 0xDEADBEEF;
    
    if ((*((char *) &value) >> 4) == 0x1) {
        real_byteorder = SDL_BIG_ENDIAN;
    } else {
        real_byteorder = SDL_LIL_ENDIAN;
    }

    /* Test endianness. */    
    AssertTrue( real_byteorder == SDL_BYTEORDER,
             "Machine detected as %s endian but appears to be %s endian.",
             (SDL_BYTEORDER == SDL_LIL_ENDIAN) ? "little" : "big",
             (real_byteorder == SDL_LIL_ENDIAN) ? "little" : "big" );

    /* Test 16 swap. */
    AssertTrue( SDL_Swap16(value16) == swapped16,
             "SDL_Swap16(): 16 bit swapped incorrectly: 0x%X => 0x%X",
             value16, SDL_Swap16(value16) );

    /* Test 32 swap. */
    AssertTrue( SDL_Swap32(value32) == swapped32,
             "SDL_Swap32(): 32 bit swapped incorrectly: 0x%X => 0x%X",
             value32, SDL_Swap32(value32) );

    /* Test 64 swap. */
    AssertTrue( SDL_Swap64(value64) == swapped64,
#ifdef _MSC_VER
             "SDL_Swap64(): 64 bit swapped incorrectly: 0x%I64X => 0x%I64X",
#else
             "SDL_Swap64(): 64 bit swapped incorrectly: 0x%llX => 0x%llX",
#endif
             value64, SDL_Swap64(value64) );
}

/*!
 * \brief Tests SDL_GetXYZ() functions
 */
int platform_testGetFunctions (void *arg)
{
   int ret;
 
   ret = SDL_GetPlatform();
   AssertPass("SDL_GetPlatform()");
 
   ret = SDL_GetCPUCount();
   AssertPass("SDL_GetCPUCount()");
}

/*!
 * \brief Tests SDL_HasXYZ() functions
 */
int platform_testHasFunctions (void *arg)
{
   int ret;
   
   // TODO: independently determine and compare values as well
   
   ret = SDL_HasRDTSC();
   AssertPass("SDL_HasRDTSC()");
   
   ret = SDL_HasAltiVec();
   AssertPass("SDL_HasAltiVec()");

   ret = SDL_HasMMX();
   AssertPass("SDL_HasMMX()");

   ret = SDL_Has3DNow();
   AssertPass("SDL_Has3DNow()");

   ret = SDL_HasSSE();
   AssertPass("SDL_HasSSE()");

   ret = SDL_HasSSE2();
   AssertPass("SDL_HasSSE2()");

   ret = SDL_HasSSE3();
   AssertPass("SDL_HasSSE3()");

   ret = SDL_HasSSE41();
   AssertPass("SDL_HasSSE41()");

   ret = SDL_HasSSE42();
   AssertPass("SDL_HasSSE42()");
}
