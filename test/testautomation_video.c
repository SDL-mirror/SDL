/**
 * Video test suite
 */

#include <stdio.h>

#include "SDL.h"
#include "SDL_test.h"

/* Test case functions */

/**
 * @brief Enable and disable screensaver while checking state
 */
int
video_enableDisableScreensaver(void *arg)
{
	SDL_bool initialResult;
	SDL_bool result;

	/* Get current state and proceed according to current state */
	initialResult = SDL_IsScreenSaverEnabled();
	SDLTest_AssertPass("Call to SDL_IsScreenSaverEnabled()");	
	if (initialResult == SDL_TRUE) {
	
	  /* Currently enabled: disable first, then enable again */
	  
	  /* Disable screensaver and check */	
	  SDL_DisableScreenSaver();
	  SDLTest_AssertPass("Call to SDL_DisableScreenSaver()");	
	  result = SDL_IsScreenSaverEnabled();
	  SDLTest_AssertPass("Call to SDL_IsScreenSaverEnabled()");
	  SDLTest_AssertCheck(result == SDL_FALSE, "Verify result from SDL_IsScreenSaverEnabled, expected: %i, got: %i", SDL_FALSE, result);
	
	  /* Enable screensaver and check */	
	  SDL_EnableScreenSaver();
	  SDLTest_AssertPass("Call to SDL_EnableScreenSaver()");
	  result = SDL_IsScreenSaverEnabled();
	  SDLTest_AssertPass("Call to SDL_IsScreenSaverEnabled()");
	  SDLTest_AssertCheck(result == SDL_TRUE, "Verify result from SDL_IsScreenSaverEnabled, expected: %i, got: %i", SDL_TRUE, result);

	} else {

	  /* Currently disabled: enable first, then disable again */
	  
	  /* Enable screensaver and check */	
	  SDL_EnableScreenSaver();
	  SDLTest_AssertPass("Call to SDL_EnableScreenSaver()");
	  result = SDL_IsScreenSaverEnabled();
	  SDLTest_AssertPass("Call to SDL_IsScreenSaverEnabled()");
	  SDLTest_AssertCheck(result == SDL_TRUE, "Verify result from SDL_IsScreenSaverEnabled, expected: %i, got: %i", SDL_TRUE, result);

	  /* Disable screensaver and check */	
	  SDL_DisableScreenSaver();
	  SDLTest_AssertPass("Call to SDL_DisableScreenSaver()");	
	  result = SDL_IsScreenSaverEnabled();
	  SDLTest_AssertPass("Call to SDL_IsScreenSaverEnabled()");
	  SDLTest_AssertCheck(result == SDL_FALSE, "Verify result from SDL_IsScreenSaverEnabled, expected: %i, got: %i", SDL_FALSE, result);	
	}	
	
	return TEST_COMPLETED;
}

/**
 * @brief Tests the functionality of the SDL_CreateWindow function using different positions
 */
int
video_createWindowVariousPositions(void *arg)
{
  SDL_Window* window;
  const char* title = "video_createWindowVariousPositions Test Window";
  int x, y, w, h;
  int xVariation, yVariation;
  
  for (xVariation = 0; xVariation < 6; xVariation++) {
   for (yVariation = 0; yVariation < 6; yVariation++) {
    switch(xVariation) {
     case 0:
      /* Zero X Position */  
      x = 0;
      break;
     case 1:
      /* Random X position inside screen */  
      x = SDLTest_RandomIntegerInRange(1, 100);
      break;
     case 2:
      /* Random X position outside screen (positive) */  
      x = SDLTest_RandomIntegerInRange(10000, 11000);
      break;
     case 3:
      /* Random X position outside screen (negative) */  
      x = SDLTest_RandomIntegerInRange(-1000, -100);
      break;
     case 4:
      /* Centered X position */  
      x = SDL_WINDOWPOS_CENTERED;
      break;
     case 5:
      /* Undefined X position */  
      x = SDL_WINDOWPOS_UNDEFINED;
      break;
    }

    switch(yVariation) {
     case 0:
      /* Zero X Position */  
      y = 0;
      break;
     case 1:
      /* Random X position inside screen */  
      y = SDLTest_RandomIntegerInRange(1, 100);
      break;
     case 2:
      /* Random X position outside screen (positive) */  
      y = SDLTest_RandomIntegerInRange(10000, 11000);
      break;
     case 3:
      /* Random Y position outside screen (negative) */  
      y = SDLTest_RandomIntegerInRange(-1000, -100);
      break;
     case 4:
      /* Centered Y position */  
      y = SDL_WINDOWPOS_CENTERED;
      break;
     case 5:
      /* Undefined Y position */  
      y = SDL_WINDOWPOS_UNDEFINED;
      break;
    }
     
    w = SDLTest_RandomIntegerInRange(32, 96);
    h = SDLTest_RandomIntegerInRange(32, 96);
    window = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_SHOWN);
    SDLTest_AssertPass("Call to SDL_CreateWindow('Title',%d,%d,%d,%d,SHOWN)", x, y, w, h);
    SDLTest_AssertCheck(window != NULL, "Validate that returned window struct is not NULL");
    if (window != NULL) {
      SDL_DestroyWindow(window);
      SDLTest_AssertPass("Call to SDL_DestroyWindow");
    }
   }
  } 

  return TEST_COMPLETED;
}

/**
 * @brief Tests the functionality of the SDL_CreateWindow function using different sizes
 */
int
video_createWindowVariousSizes(void *arg)
{
  SDL_Window* window;
  const char* title = "video_createWindowVariousSizes Test Window";
  int x, y, w, h;
  int wVariation, hVariation;
  
  x = SDLTest_RandomIntegerInRange(1, 100);
  y = SDLTest_RandomIntegerInRange(1, 100);
  for (wVariation = 0; wVariation < 3; wVariation++) {
   for (hVariation = 0; hVariation < 3; hVariation++) {
    switch(wVariation) {
     case 0:
      /* Width of 1 */  
      w = 1;
      break;
     case 1:
      /* Random "normal" width */  
      w = SDLTest_RandomIntegerInRange(320, 1920);
      break;
     case 2:
      /* Random "large" width */  
      w = SDLTest_RandomIntegerInRange(2048, 4095);
      break;
    }

    switch(hVariation) {
     case 0:
      /* Height of 1 */  
      h = 1;
      break;
     case 1:
      /* Random "normal" height */  
      h = SDLTest_RandomIntegerInRange(320, 1080);
      break;
     case 2:
      /* Random "large" height */  
      h = SDLTest_RandomIntegerInRange(2048, 4095);
      break;
     }
     
    window = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_SHOWN);
    SDLTest_AssertPass("Call to SDL_CreateWindow('Title',%d,%d,%d,%d,SHOWN)", x, y, w, h);
    SDLTest_AssertCheck(window != NULL, "Validate that returned window struct is not NULL");
    if (window != NULL) {
      SDL_DestroyWindow(window);
      SDLTest_AssertPass("Call to SDL_DestroyWindow");
    }
   }
  }  

  return TEST_COMPLETED;
}

/**
 * @brief Tests the functionality of the SDL_CreateWindow function using different flags
 */
int
video_createWindowVariousFlags(void *arg)
{
  SDL_Window* window;
  const char* title = "video_createWindowVariousFlags Test Window";
  int x, y, w, h;
  int fVariation;
  SDL_WindowFlags flags;
  
  /* Standard window */
  x = SDLTest_RandomIntegerInRange(1, 100);
  y = SDLTest_RandomIntegerInRange(1, 100);
  w = SDLTest_RandomIntegerInRange(320, 1024);
  h = SDLTest_RandomIntegerInRange(320, 768);

  for (fVariation = 0; fVariation < 13; fVariation++) {
    switch(fVariation) {
     case 0:
      flags = SDL_WINDOW_FULLSCREEN;
      /* Skip - blanks screen; comment out next line to run test */
      continue;   
      break;
     case 1:
      flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
      /* Skip - blanks screen; comment out next line to run test */
      continue;  
      break;
     case 2:
      flags = SDL_WINDOW_OPENGL;
      break;  
     case 3:
      flags = SDL_WINDOW_SHOWN;
      break;   
     case 4:    
      flags = SDL_WINDOW_HIDDEN;
      break;     
     case 5:
      flags = SDL_WINDOW_BORDERLESS;
      break;       
     case 6:
      flags = SDL_WINDOW_RESIZABLE;
      break;         
     case 7:
      flags = SDL_WINDOW_MINIMIZED;
      break;           
     case 8:
      flags = SDL_WINDOW_MAXIMIZED;
      break;
     case 9: 
      flags = SDL_WINDOW_INPUT_GRABBED;
      break;
     case 10:              
      flags = SDL_WINDOW_INPUT_FOCUS;
      break;                 
     case 11:                      
      flags = SDL_WINDOW_MOUSE_FOCUS;
      break;
     case 12: 
      flags = SDL_WINDOW_FOREIGN;
      break;
    }
       
    window = SDL_CreateWindow(title, x, y, w, h, flags);
    SDLTest_AssertPass("Call to SDL_CreateWindow('Title',%d,%d,%d,%d,%d)", x, y, w, h, flags);
    SDLTest_AssertCheck(window != NULL, "Validate that returned window struct is not NULL");
    if (window != NULL) {
      SDL_DestroyWindow(window);
      SDLTest_AssertPass("Call to SDL_DestroyWindow");
    }   
  }

  return TEST_COMPLETED;
}

/**
 * @brief Tests the functionality of the SDL_GetWindowFlags function
 */
int
video_getWindowFlags(void *arg)
{
  SDL_Window* window;
  const char* title = "video_getWindowFlags Test Window";
  int x, y, w, h;
  SDL_WindowFlags flags;
  Uint32 actualFlags;
  
  /* Standard window */
  x = SDLTest_RandomIntegerInRange(1, 100);
  y = SDLTest_RandomIntegerInRange(1, 100);
  w = SDLTest_RandomIntegerInRange(320, 1024);
  h = SDLTest_RandomIntegerInRange(320, 768);
  
  /* Reliable flag */
  flags = SDL_WINDOW_SHOWN;
  
  window = SDL_CreateWindow(title, x, y, w, h, flags);
  SDLTest_AssertPass("Call to SDL_CreateWindow('Title',%d,%d,%d,%d,%d)", x, y, w, h, flags);
  SDLTest_AssertCheck(window != NULL, "Validate that returned window struct is not NULL");
  if (window != NULL) {
      actualFlags = SDL_GetWindowFlags(window);
      SDLTest_AssertPass("Call to SDL_GetWindowFlags");
      SDLTest_AssertCheck((flags & actualFlags) == flags, "Verify returned value has flags %d set, got: %d", flags, actualFlags);
      SDL_DestroyWindow(window);
      SDLTest_AssertPass("Call to SDL_DestroyWindow");  
  }

  return TEST_COMPLETED;
}

/* ================= Test References ================== */

/* Video test cases */
static const SDLTest_TestCaseReference videoTest1 =
		{ (SDLTest_TestCaseFp)video_enableDisableScreensaver, "video_enableDisableScreensaver",  "Enable and disable screenaver while checking state", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest2 =
		{ (SDLTest_TestCaseFp)video_createWindowVariousPositions, "video_createWindowVariousPositions",  "Create windows at various locations", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest3 =
		{ (SDLTest_TestCaseFp)video_createWindowVariousSizes, "video_createWindowVariousSizes",  "Create windows with various sizes", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest4 =
		{ (SDLTest_TestCaseFp)video_createWindowVariousFlags, "video_createWindowVariousFlags",  "Create windows using various flags", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest5 =
		{ (SDLTest_TestCaseFp)video_getWindowFlags, "video_getWindowFlags",  "Set and get window flags", TEST_ENABLED };

/* Sequence of Video test cases */
static const SDLTest_TestCaseReference *videoTests[] =  {
	&videoTest1, &videoTest2, &videoTest3, &videoTest4, &videoTest5, NULL
};

/* Video test suite (global) */
SDLTest_TestSuiteReference videoTestSuite = {
	"Video",
	NULL,
	videoTests,
	NULL
};
