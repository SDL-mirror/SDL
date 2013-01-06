/**
 * Keyboard test suite
 */

#include <stdio.h>

#include "SDL.h"
#include "SDL_test.h"

/* ================= Test Case Implementation ================== */

/*!
 * TODO: Add tests for keyboard here
 *
 */

/* Test case functions */

/**
 * @brief Check call to SDL_GetKeyboardState
 * 
 */
int
keyboard_getKeyboardState(void *arg)
{
   int numkeys;
   Uint8 *state;

   /* Case where numkeys pointer is NULL */    
   state = SDL_GetKeyboardState(NULL);
   SDLTest_AssertPass("Call to SDL_GetKeyboardState(NULL)");
   SDLTest_AssertCheck(state != NULL, "Validate that return value from SDL_GetKeyboardState is not NULL");

   /* Case where numkeys pointer is not NULL */
   numkeys = -1;
   state = SDL_GetKeyboardState(&numkeys);
   SDLTest_AssertPass("Call to SDL_GetKeyboardState(&numkeys)");
   SDLTest_AssertCheck(state != NULL, "Validate that return value from SDL_GetKeyboardState is not NULL");
   SDLTest_AssertCheck(numkeys >= 0, "Validate that value of numkeys is >= 0, got: %i", numkeys);
   
   return TEST_COMPLETED;
}

/* ================= Test References ================== */

/* Keyboard test cases */
static const SDLTest_TestCaseReference keyboardTest1 =
		{ (SDLTest_TestCaseFp)keyboard_getKeyboardState, "keyboard_getKeyboardState", "Check call to SDL_GetKeyboardState", TEST_ENABLED };

/* Sequence of Keyboard test cases */
static const SDLTest_TestCaseReference *keyboardTests[] =  {
	&keyboardTest1, NULL
};

/* Keyboard test suite (global) */
SDLTest_TestSuiteReference keyboardTestSuite = {
	"Keyboard",
	NULL,
	keyboardTests,
	NULL
};
