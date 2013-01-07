/**
 * Mouse test suite
 */

#include <stdio.h>
#include <limits.h>

#include "SDL.h"
#include "SDL_test.h"

/* ================= Test Case Implementation ================== */

/* Test case functions */

/* Helper to evaluate state returned from SDL_GetMouseState */
int _mouseStateCheck(Uint8 state)
{
  return (state == 0) || 
         SDL_BUTTON(SDL_BUTTON_LEFT) || 
         SDL_BUTTON(SDL_BUTTON_MIDDLE) || 
         SDL_BUTTON(SDL_BUTTON_RIGHT) || 
         SDL_BUTTON(SDL_BUTTON_X1) || 
         SDL_BUTTON(SDL_BUTTON_X2);
}

/**
 * @brief Check call to SDL_GetMouseState
 * 
 */
int
mouse_getMouseState(void *arg)
{
   int x;
   int y;
   Uint8 state;

   /* Pump some events to update mouse state */
   SDL_PumpEvents();
   SDLTest_AssertPass("Call to SDL_PumpEvents()");

   /* Case where x, y pointer is NULL */    
   state = SDL_GetMouseState(NULL, NULL);
   SDLTest_AssertPass("Call to SDL_GetMouseState(NULL, NULL)");
   SDLTest_AssertCheck(_mouseStateCheck(state), "Validate state returned from function, got: %i", state);

   /* Case where x pointer is not NULL */
   x = INT_MIN;
   state = SDL_GetMouseState(&x, NULL);
   SDLTest_AssertPass("Call to SDL_GetMouseState(&x, NULL)");
   SDLTest_AssertCheck(x > INT_MIN, "Validate that value of x is > INT_MIN, got: %i", x);
   SDLTest_AssertCheck(_mouseStateCheck(state), "Validate state returned from function, got: %i", state);

   /* Case where y pointer is not NULL */
   y = INT_MIN;
   state = SDL_GetMouseState(NULL, &y);
   SDLTest_AssertPass("Call to SDL_GetMouseState(NULL, &y)");
   SDLTest_AssertCheck(y > INT_MIN, "Validate that value of y is > INT_MIN, got: %i", y);
   SDLTest_AssertCheck(_mouseStateCheck(state), "Validate state returned from function, got: %i", state);

   /* Case where x and y pointer is not NULL */
   x = INT_MIN;
   y = INT_MIN;
   state = SDL_GetMouseState(&x, &y);
   SDLTest_AssertPass("Call to SDL_GetMouseState(&x, &y)");
   SDLTest_AssertCheck(x > INT_MIN, "Validate that value of x is > INT_MIN, got: %i", x);
   SDLTest_AssertCheck(y > INT_MIN, "Validate that value of y is > INT_MIN, got: %i", y);
   SDLTest_AssertCheck(_mouseStateCheck(state), "Validate state returned from function, got: %i", state);
   
   return TEST_COMPLETED;
}

/**
 * @brief Check call to SDL_GetRelativeMouseState
 * 
 */
int
mouse_getRelativeMouseState(void *arg)
{
   int x;
   int y;
   Uint8 state;

   /* Pump some events to update mouse state */
   SDL_PumpEvents();
   SDLTest_AssertPass("Call to SDL_PumpEvents()");

   /* Case where x, y pointer is NULL */    
   state = SDL_GetRelativeMouseState(NULL, NULL);
   SDLTest_AssertPass("Call to SDL_GetRelativeMouseState(NULL, NULL)");
   SDLTest_AssertCheck(_mouseStateCheck(state), "Validate state returned from function, got: %i", state);

   /* Case where x pointer is not NULL */
   x = INT_MIN;
   state = SDL_GetRelativeMouseState(&x, NULL);
   SDLTest_AssertPass("Call to SDL_GetRelativeMouseState(&x, NULL)");
   SDLTest_AssertCheck(x > INT_MIN, "Validate that value of x is > INT_MIN, got: %i", x);
   SDLTest_AssertCheck(_mouseStateCheck(state), "Validate state returned from function, got: %i", state);

   /* Case where y pointer is not NULL */
   y = INT_MIN;
   state = SDL_GetRelativeMouseState(NULL, &y);
   SDLTest_AssertPass("Call to SDL_GetRelativeMouseState(NULL, &y)");
   SDLTest_AssertCheck(y > INT_MIN, "Validate that value of y is > INT_MIN, got: %i", y);
   SDLTest_AssertCheck(_mouseStateCheck(state), "Validate state returned from function, got: %i", state);

   /* Case where x and y pointer is not NULL */
   x = INT_MIN;
   y = INT_MIN;
   state = SDL_GetRelativeMouseState(&x, &y);
   SDLTest_AssertPass("Call to SDL_GetRelativeMouseState(&x, &y)");
   SDLTest_AssertCheck(x > INT_MIN, "Validate that value of x is > INT_MIN, got: %i", x);
   SDLTest_AssertCheck(y > INT_MIN, "Validate that value of y is > INT_MIN, got: %i", y);
   SDLTest_AssertCheck(_mouseStateCheck(state), "Validate state returned from function, got: %i", state);
   
   return TEST_COMPLETED;
}


/* XPM definition of mouse Cursor */
static const char *_mouseArrowData[] = {
  /* pixels */
  "X                               ",
  "XX                              ",
  "X.X                             ",
  "X..X                            ",
  "X...X                           ",
  "X....X                          ",
  "X.....X                         ",
  "X......X                        ",
  "X.......X                       ",
  "X........X                      ",
  "X.....XXXXX                     ",
  "X..X..X                         ",
  "X.X X..X                        ",
  "XX  X..X                        ",
  "X    X..X                       ",
  "     X..X                       ",
  "      X..X                      ",
  "      X..X                      ",
  "       XX                       ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                "
};

/* Helper that creates a new mouse cursor from an XPM */
static SDL_Cursor *_initArrowCursor(const char *image[])
{
  SDL_Cursor *cursor;
  int i, row, col;
  Uint8 data[4*32];
  Uint8 mask[4*32];

  i = -1;
  for ( row=0; row<32; ++row ) {
    for ( col=0; col<32; ++col ) {
      if ( col % 8 ) {
        data[i] <<= 1;
        mask[i] <<= 1;
      } else {
        ++i;
        data[i] = mask[i] = 0;
      }
      switch (image[row][col]) {
        case 'X':
          data[i] |= 0x01;
          mask[i] |= 0x01;
          break;
        case '.':
          mask[i] |= 0x01;
          break;
        case ' ':
          break;
      }
    }
  }

  cursor = SDL_CreateCursor(data, mask, 32, 32, 0, 0);
  return cursor;
}

/**
 * @brief Check call to SDL_CreateCursor and SDL_FreeCursor
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_CreateCursor
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_FreeCursor
 */
int
mouse_createFreeCursor(void *arg)
{
	SDL_Cursor *cursor;	

	/* Create a cursor */
	cursor = _initArrowCursor(_mouseArrowData);
    SDLTest_AssertPass("Call to SDL_CreateCursor()");
    SDLTest_AssertCheck(cursor != NULL, "Validate result from SDL_CreateCursor() is not NULL");	
	if (cursor == NULL) {
		return TEST_ABORTED;
	}

	/* Free cursor again */
	SDL_FreeCursor(cursor);
	SDLTest_AssertPass("Call to SDL_FreeCursor()");

	return TEST_COMPLETED;
}

/* Helper that changes cursor visibility */
void _changeCursorVisibility(int state)
{
	int oldState;
	int newState;
	int result;

    oldState = SDL_ShowCursor(SDL_QUERY);
	SDLTest_AssertPass("Call to SDL_ShowCursor(SDL_QUERY)");

    result = SDL_ShowCursor(state);
	SDLTest_AssertPass("Call to SDL_ShowCursor(%s)", (state == SDL_ENABLE) ? "SDL_ENABLE" : "SDL_DISABLE");
	SDLTest_AssertCheck(result == oldState, "Validate result from SDL_ShowCursor(%s), expected: %i, got: %i", 
		(state == SDL_ENABLE) ? "SDL_ENABLE" : "SDL_DISABLE", oldState, result);
    
	newState = SDL_ShowCursor(SDL_QUERY);
	SDLTest_AssertPass("Call to SDL_ShowCursor(SDL_QUERY)");
	SDLTest_AssertCheck(state == newState, "Validate new state, expected: %i, got: %i", 
		state, newState);
}

/**
 * @brief Check call to SDL_ShowCursor
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_ShowCursor
 */
int
mouse_showCursor(void *arg)
{
	int currentState;

	/* Get current state */
	currentState = SDL_ShowCursor(SDL_QUERY);
	SDLTest_AssertPass("Call to SDL_ShowCursor(SDL_QUERY)");
	SDLTest_AssertCheck(currentState == SDL_DISABLE || currentState == SDL_ENABLE, 
		"Validate result is %i or %i, got: %i", SDL_DISABLE, SDL_ENABLE, currentState);
	if (currentState == SDL_DISABLE) {
		/* Show the cursor, then hide it again */
		_changeCursorVisibility(SDL_ENABLE);
		_changeCursorVisibility(SDL_DISABLE);
	} else if (currentState == SDL_ENABLE) {
		/* Hide the cursor, then show it again */
		_changeCursorVisibility(SDL_DISABLE);
		_changeCursorVisibility(SDL_ENABLE);
	} else {
		return TEST_ABORTED;
	}

	return TEST_COMPLETED;
}

/**
 * @brief Check call to SDL_SetCursor
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_SetCursor
 */
int
mouse_setCursor(void *arg)
{
	SDL_Cursor *cursor;	

	/* Create a cursor */
	cursor = _initArrowCursor(_mouseArrowData);
    SDLTest_AssertPass("Call to SDL_CreateCursor()");
    SDLTest_AssertCheck(cursor != NULL, "Validate result from SDL_CreateCursor() is not NULL");	
	if (cursor == NULL) {
		return TEST_ABORTED;
	}

	/* Set the arrow cursor */
	SDL_SetCursor(cursor);
	SDLTest_AssertPass("Call to SDL_SetCursor(cursor)");

	/* Force redraw */
	SDL_SetCursor(NULL);
	SDLTest_AssertPass("Call to SDL_SetCursor(NULL)");

	/* Free cursor again */
	SDL_FreeCursor(cursor);
	SDLTest_AssertPass("Call to SDL_FreeCursor()");

	return TEST_COMPLETED;
}

/* ================= Test References ================== */

/* Mouse test cases */
static const SDLTest_TestCaseReference mouseTest1 =
		{ (SDLTest_TestCaseFp)mouse_getMouseState, "mouse_getMouseState", "Check call to SDL_GetMouseState", TEST_ENABLED };

static const SDLTest_TestCaseReference mouseTest2 =
		{ (SDLTest_TestCaseFp)mouse_getRelativeMouseState, "mouse_getRelativeMouseState", "Check call to SDL_GetRelativeMouseState", TEST_ENABLED };

static const SDLTest_TestCaseReference mouseTest3 =
		{ (SDLTest_TestCaseFp)mouse_createFreeCursor, "mouse_createFreeCursor", "Check call to SDL_CreateCursor and SDL_FreeCursor", TEST_ENABLED };

static const SDLTest_TestCaseReference mouseTest4 =
		{ (SDLTest_TestCaseFp)mouse_showCursor, "mouse_showCursor", "Check call to SDL_ShowCursor", TEST_ENABLED };

static const SDLTest_TestCaseReference mouseTest5 =
		{ (SDLTest_TestCaseFp)mouse_setCursor, "mouse_setCursor", "Check call to SDL_SetCursor", TEST_ENABLED };

/* Sequence of Mouse test cases */
static const SDLTest_TestCaseReference *mouseTests[] =  {
	&mouseTest1, &mouseTest2, &mouseTest3, &mouseTest4, &mouseTest5, NULL
};

/* Mouse test suite (global) */
SDLTest_TestSuiteReference mouseTestSuite = {
	"Mouse",
	NULL,
	mouseTests,
	NULL
};
