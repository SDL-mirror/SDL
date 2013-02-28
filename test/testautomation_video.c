/**
 * Video test suite
 */

#include <stdio.h>

#include "SDL.h"
#include "SDL_test.h"

/* Private helpers */

/* 
 * Create a test window
 */
SDL_Window *_createVideoSuiteTestWindow(const char *title)
{
  SDL_Window* window;
  int x, y, w, h;
  SDL_WindowFlags flags;

  /* Standard window */
  x = SDLTest_RandomIntegerInRange(1, 100);
  y = SDLTest_RandomIntegerInRange(1, 100);
  w = SDLTest_RandomIntegerInRange(320, 1024);
  h = SDLTest_RandomIntegerInRange(320, 768);
  flags = SDL_WINDOW_SHOWN;
  
  window = SDL_CreateWindow(title, x, y, w, h, flags);
  SDLTest_AssertPass("Call to SDL_CreateWindow('Title',%d,%d,%d,%d,%d)", x, y, w, h, flags);
  SDLTest_AssertCheck(window != NULL, "Validate that returned window struct is not NULL");

  return window;
}

/*
 * Destroy test window  
 */
void _destroyVideoSuiteTestWindow(SDL_Window *window)
{
  if (window != NULL) {  
     SDL_DestroyWindow(window);
     window = NULL;
     SDLTest_AssertPass("Call to SDL_DestroyWindow");
  }
}

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

    /* Clean up */    
    _destroyVideoSuiteTestWindow(window);
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

    /* Clean up */    
    _destroyVideoSuiteTestWindow(window);
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


    /* Clean up */    
    _destroyVideoSuiteTestWindow(window);  
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
  SDL_WindowFlags flags;
  Uint32 actualFlags;
  
  /* Reliable flag set always set in test window */
  flags = SDL_WINDOW_SHOWN;
  
  /* Call against new test window */ 
  window = _createVideoSuiteTestWindow(title);
  if (window != NULL) {
      actualFlags = SDL_GetWindowFlags(window);
      SDLTest_AssertPass("Call to SDL_GetWindowFlags");
      SDLTest_AssertCheck((flags & actualFlags) == flags, "Verify returned value has flags %d set, got: %d", flags, actualFlags);
  }

  /* Clean up */    
  _destroyVideoSuiteTestWindow(window);
  
  return TEST_COMPLETED;
}

/**
 * @brief Tests the functionality of the SDL_GetNumDisplayModes function
 */
int
video_getNumDisplayModes(void *arg)
{
  int result;
  int displayNum;
  int i;

  /* Get number of displays */
  displayNum = SDL_GetNumVideoDisplays();
  SDLTest_AssertPass("Call to SDL_GetNumVideoDisplays");

  /* Make call for each display */  
  for (i=0; i<displayNum; i++) {
    result = SDL_GetNumDisplayModes(i);
    SDLTest_AssertPass("Call to SDL_GetNumDisplayModes(%d)", i);
    SDLTest_AssertCheck(result >= 1, "Validate returned value from function; expected: >=1; got: %d", result);
  }

  return TEST_COMPLETED;
}

/**
 * @brief Tests negative call to SDL_GetNumDisplayModes function
 */
int
video_getNumDisplayModesNegative(void *arg)
{
  int result;
  int displayNum;
  int displayIndex;

  /* Get number of displays */
  displayNum = SDL_GetNumVideoDisplays();
  SDLTest_AssertPass("Call to SDL_GetNumVideoDisplays");

  /* Invalid boundary values */
  displayIndex =  SDLTest_RandomSint32BoundaryValue(0, displayNum, SDL_FALSE);
  result = SDL_GetNumDisplayModes(displayIndex);
  SDLTest_AssertPass("Call to SDL_GetNumDisplayModes(%d=out-of-bounds/boundary)", displayIndex);
  SDLTest_AssertCheck(result < 0, "Validate returned value from function; expected: <0; got: %d", result);  
  
  /* Large (out-of-bounds) display index */
  displayIndex = SDLTest_RandomIntegerInRange(-2000, -1000);
  result = SDL_GetNumDisplayModes(displayIndex);
  SDLTest_AssertPass("Call to SDL_GetNumDisplayModes(%d=out-of-bounds/large negative)", displayIndex);
  SDLTest_AssertCheck(result < 0, "Validate returned value from function; expected: <0; got: %d", result);  

  displayIndex = SDLTest_RandomIntegerInRange(1000, 2000);
  result = SDL_GetNumDisplayModes(displayIndex);
  SDLTest_AssertPass("Call to SDL_GetNumDisplayModes(%d=out-of-bounds/large positive)", displayIndex);
  SDLTest_AssertCheck(result < 0, "Validate returned value from function; expected: <0; got: %d", result);  

  return TEST_COMPLETED;
}

/**
 * @brief Tests the functionality of the SDL_GetClosestDisplayMode function against current resolution
 */
int
video_getClosestDisplayModeCurrentResolution(void *arg)
{
  int result;
  SDL_DisplayMode current; 
  SDL_DisplayMode target; 
  SDL_DisplayMode closest;
  SDL_DisplayMode* dResult;
  int displayNum;
  int i;
  int variation;

  /* Get number of displays */
  displayNum = SDL_GetNumVideoDisplays();
  SDLTest_AssertPass("Call to SDL_GetNumVideoDisplays");

  /* Make calls for each display */  
  for (i=0; i<displayNum; i++) {
    SDLTest_Log("Testing against display: %d", i);
  
    /* Get first display mode to get a sane resolution; this should always work */
    result = SDL_GetDisplayMode(i, 0, &current);
    SDLTest_AssertPass("Call to SDL_GetDisplayMode");
    SDLTest_AssertCheck(result == 0, "Verify return value, expected: 0, got: %d", result);
    if (result != 0) {
      return TEST_ABORTED;
    }
       
    /* Set the desired resolution equals to current resolution */
    target.w = current.w;
    target.h = current.h;    
    for (variation = 0; variation < 8; variation ++) {
      /* Vary constraints on other query parameters */
      target.format = (variation & 1) ? current.format : 0;
      target.refresh_rate = (variation & 2) ? current.refresh_rate : 0;
      target.driverdata = (variation & 4) ? current.driverdata : 0;
          
      /* Make call */
      dResult = SDL_GetClosestDisplayMode(i, &target, &closest);
      SDLTest_AssertPass("Call to SDL_GetClosestDisplayMode(target=current/variation%d)", variation);
      SDLTest_AssertCheck(dResult != NULL, "Verify returned value is not NULL");
    
      /* Check that one gets the current resolution back again */
      SDLTest_AssertCheck(closest.w == current.w, "Verify returned width matches current width; expected: %d, got: %d", current.w, closest.w);
      SDLTest_AssertCheck(closest.h == current.h, "Verify returned height matches current height; expected: %d, got: %d", current.h, closest.h);
      SDLTest_AssertCheck(closest.w == dResult->w, "Verify return value matches assigned value; expected: %d, got: %d", closest.w, dResult->w);
      SDLTest_AssertCheck(closest.h == dResult->h, "Verify return value matches assigned value; expected: %d, got: %d", closest.h, dResult->h);
    }
  }

  return TEST_COMPLETED;
}

/**
 * @brief Tests the functionality of the SDL_GetClosestDisplayMode function against random resolution
 */
int
video_getClosestDisplayModeRandomResolution(void *arg)
{
  SDL_DisplayMode target; 
  SDL_DisplayMode closest;
  SDL_DisplayMode* dResult;
  int displayNum;
  int i;
  int variation;

  /* Get number of displays */
  displayNum = SDL_GetNumVideoDisplays();
  SDLTest_AssertPass("Call to SDL_GetNumVideoDisplays");

  /* Make calls for each display */  
  for (i=0; i<displayNum; i++) {
    SDLTest_Log("Testing against display: %d", i);
         
    for (variation = 0; variation < 16; variation ++) {
    
      /* Set random constraints */
      target.w = (variation & 1) ? SDLTest_RandomIntegerInRange(1, 4096) : 0;
      target.h = (variation & 2) ? SDLTest_RandomIntegerInRange(1, 4096) : 0;    
      target.format = (variation & 4) ? SDLTest_RandomIntegerInRange(1, 10) : 0;
      target.refresh_rate = (variation & 8) ? SDLTest_RandomIntegerInRange(25, 120) : 0;
      target.driverdata = 0;
          
      /* Make call; may or may not find anything, so don't validate any further */
      dResult = SDL_GetClosestDisplayMode(i, &target, &closest);
      SDLTest_AssertPass("Call to SDL_GetClosestDisplayMode(target=random/variation%d)", variation);      
    }
  }

  return TEST_COMPLETED;
}

/**
 * @brief Tests call to SDL_GetWindowBrightness
 *
* @sa http://wiki.libsdl.org/moin.fcg/SDL_GetWindowBrightness
 */
int
video_getWindowBrightness(void *arg)
{
  SDL_Window* window;
  const char* title = "video_getWindowBrightness Test Window";
  float result;

  /* Call against new test window */ 
  window = _createVideoSuiteTestWindow(title);
  if (window != NULL) {
      result = SDL_GetWindowBrightness(window);
      SDLTest_AssertPass("Call to SDL_GetWindowBrightness");
      SDLTest_AssertCheck(result >= 0.0 && result <= 1.0, "Validate range of result value; expected: [0.0, 1.0], got: %f", result);
  }

  /* Clean up */    
  _destroyVideoSuiteTestWindow(window);
  
  return TEST_COMPLETED;
}

/**
 * @brief Tests call to SDL_GetWindowBrightness with invalid input
 *
* @sa http://wiki.libsdl.org/moin.fcg/SDL_GetWindowBrightness
 */
int
video_getWindowBrightnessNegative(void *arg)
{
  const char *invalidWindowError = "Invalid window";
  char *lastError;
  const char* title = "video_getWindowBrightnessNegative Test Window";
  float result;

  /* Call against invalid window */ 
  result = SDL_GetWindowBrightness(NULL);
  SDLTest_AssertPass("Call to SDL_GetWindowBrightness(window=NULL)");
  SDLTest_AssertCheck(result == 1.0, "Validate result value; expected: 1.0, got: %f", result);
  lastError = (char *)SDL_GetError();
  SDLTest_AssertPass("SDL_GetError()");
  SDLTest_AssertCheck(lastError != NULL, "Verify error message is not NULL");
  if (lastError != NULL) {
      SDLTest_AssertCheck(SDL_strcmp(lastError, invalidWindowError) == 0,
         "SDL_GetError(): expected message '%s', was message: '%s'",
         invalidWindowError,
         lastError);
  }

  return TEST_COMPLETED;
}

/**
 * @brief Tests call to SDL_GetWindowDisplayMode
 *
 * @sa http://wiki.libsdl.org/moin.fcg/SDL_GetWindowDisplayMode
 */
int
video_getWindowDisplayMode(void *arg)
{
  SDL_Window* window;
  const char* title = "video_getWindowDisplayMode Test Window";
  SDL_DisplayMode mode;
  int result;

  /* Invalidate part of the mode content so we can check values later */
  mode.w = -1;
  mode.h = -1;
  mode.refresh_rate = -1;

  /* Call against new test window */ 
  window = _createVideoSuiteTestWindow(title);
  if (window != NULL) {
      result = SDL_GetWindowDisplayMode(window, &mode);
      SDLTest_AssertPass("Call to SDL_GetWindowDisplayMode");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);
      SDLTest_AssertCheck(mode.w > 0, "Validate mode.w content; expected: >0, got: %d", mode.w);
      SDLTest_AssertCheck(mode.h > 0, "Validate mode.h content; expected: >0, got: %d", mode.h);
      SDLTest_AssertCheck(mode.refresh_rate > 0, "Validate mode.refresh_rate content; expected: >0, got: %d", mode.refresh_rate);
  }

  /* Clean up */    
  _destroyVideoSuiteTestWindow(window);
  
  return TEST_COMPLETED;
}

/**
 * @brief Tests call to SDL_GetWindowDisplayMode with invalid input
 *
 * @sa http://wiki.libsdl.org/moin.fcg/SDL_GetWindowDisplayMode
 */
int
video_getWindowDisplayModeNegative(void *arg)
{
  const char *expectedError = "Parameter 'mode' is invalid";
  const char *invalidWindowError = "Invalid window";
  char *lastError;
  SDL_Window* window;
  const char* title = "video_getWindowDisplayModeNegative Test Window";
  SDL_DisplayMode mode;
  int result;

  /* Call against new test window */ 
  window = _createVideoSuiteTestWindow(title);
  if (window != NULL) {
      result = SDL_GetWindowDisplayMode(window, NULL);
      SDLTest_AssertPass("Call to SDL_GetWindowDisplayMode(...,mode=NULL)");
      SDLTest_AssertCheck(result == -1, "Validate result value; expected: -1, got: %d", result);
      lastError = (char *)SDL_GetError();
      SDLTest_AssertPass("SDL_GetError()");
      SDLTest_AssertCheck(lastError != NULL, "Verify error message is not NULL");
      if (lastError != NULL) {
      	SDLTest_AssertCheck(SDL_strcmp(lastError, expectedError) == 0,
             "SDL_GetError(): expected message '%s', was message: '%s'",
             expectedError,
             lastError);
      }
  }

  /* Clean up */    
  _destroyVideoSuiteTestWindow(window);
  
  /* Call against invalid window */
  result = SDL_GetWindowDisplayMode(NULL, &mode);
  SDLTest_AssertPass("Call to SDL_GetWindowDisplayMode(window=NULL,...)");
  SDLTest_AssertCheck(result == -1, "Validate result value; expected: -1, got: %d", result);
  lastError = (char *)SDL_GetError();
  SDLTest_AssertPass("SDL_GetError()");
  SDLTest_AssertCheck(lastError != NULL, "Verify error message is not NULL");
  if (lastError != NULL) {
      SDLTest_AssertCheck(SDL_strcmp(lastError, invalidWindowError) == 0,
         "SDL_GetError(): expected message '%s', was message: '%s'",
         invalidWindowError,
         lastError);
  }
  
  return TEST_COMPLETED;
}

/**
 * @brief Tests call to SDL_GetWindowGammaRamp
 *
 * @sa http://wiki.libsdl.org/moin.fcg/SDL_GetWindowGammaRamp
 */
int
video_getWindowGammaRamp(void *arg)
{
  SDL_Window* window;
  const char* title = "video_getWindowGammaRamp Test Window";
  Uint16 red[256];
  Uint16 green[256];
  Uint16 blue[256];
  int result;

  /* Call against new test window */ 
  window = _createVideoSuiteTestWindow(title);
  if (window != NULL) {
      /* Retrieve no channel */
      result = SDL_GetWindowGammaRamp(window, NULL, NULL, NULL);
      SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(all NULL)");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);

      /* Retrieve single channel */
      result = SDL_GetWindowGammaRamp(window, red, NULL, NULL);
      SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(r)");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);

      result = SDL_GetWindowGammaRamp(window, NULL, green, NULL);
      SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(g)");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);

      result = SDL_GetWindowGammaRamp(window, NULL, NULL, blue);
      SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(b)");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);

      /* Retrieve two channels */
      result = SDL_GetWindowGammaRamp(window, red, green, NULL);
      SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(r, g)");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);

      result = SDL_GetWindowGammaRamp(window, NULL, green, blue);
      SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(g,b)");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);

      result = SDL_GetWindowGammaRamp(window, red, NULL, blue);
      SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(r,b)");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);

      /* Retrieve all channels */
      result = SDL_GetWindowGammaRamp(window, red, green, blue);
      SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(r,g,b)");
      SDLTest_AssertCheck(result == 0, "Validate result value; expected: 0, got: %d", result);
  }

  /* Clean up */    
  _destroyVideoSuiteTestWindow(window);
  
  return TEST_COMPLETED;
}

/**
 * @brief Tests call to SDL_GetWindowGammaRamp with invalid input
 *
* @sa http://wiki.libsdl.org/moin.fcg/SDL_GetWindowGammaRamp
 */
int
video_getWindowGammaRampNegative(void *arg)
{
  const char *invalidWindowError = "Invalid window";
  char *lastError;
  const char* title = "video_getWindowGammaRampNegative Test Window";
  Uint16 red[256];
  Uint16 green[256];
  Uint16 blue[256];
  int result;

  /* Call against invalid window */ 
  result = SDL_GetWindowGammaRamp(NULL, red, green, blue);
  SDLTest_AssertPass("Call to SDL_GetWindowGammaRamp(window=NULL,r,g,b)");
  SDLTest_AssertCheck(result == -1, "Validate result value; expected: -1, got: %f", result);
  lastError = (char *)SDL_GetError();
  SDLTest_AssertPass("SDL_GetError()");
  SDLTest_AssertCheck(lastError != NULL, "Verify error message is not NULL");
  if (lastError != NULL) {
      SDLTest_AssertCheck(SDL_strcmp(lastError, invalidWindowError) == 0,
         "SDL_GetError(): expected message '%s', was message: '%s'",
         invalidWindowError,
         lastError);
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
		{ (SDLTest_TestCaseFp)video_getWindowFlags, "video_getWindowFlags",  "Get window flags set during SDL_CreateWindow", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest6 =
		{ (SDLTest_TestCaseFp)video_getNumDisplayModes, "video_getNumDisplayModes",  "Use SDL_GetNumDisplayModes function to get number of display modes", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest7 =
		{ (SDLTest_TestCaseFp)video_getNumDisplayModesNegative, "video_getNumDisplayModesNegative",  "Negative tests for SDL_GetNumDisplayModes", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest8 =
		{ (SDLTest_TestCaseFp)video_getClosestDisplayModeCurrentResolution, "video_getClosestDisplayModeCurrentResolution",  "Use function to get closes match to requested display mode for current resolution", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest9 =
		{ (SDLTest_TestCaseFp)video_getClosestDisplayModeRandomResolution, "video_getClosestDisplayModeRandomResolution",  "Use function to get closes match to requested display mode for random resolution", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest10 =
		{ (SDLTest_TestCaseFp)video_getWindowBrightness, "video_getWindowBrightness",  "Get window brightness", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest11 =
		{ (SDLTest_TestCaseFp)video_getWindowBrightnessNegative, "video_getWindowBrightnessNegative",  "Get window brightness with invalid input", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest12 =
		{ (SDLTest_TestCaseFp)video_getWindowDisplayMode, "video_getWindowDisplayMode",  "Get window display mode", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest13 =
		{ (SDLTest_TestCaseFp)video_getWindowDisplayModeNegative, "video_getWindowDisplayModeNegative",  "Get window display mode with invalid input", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest14 =
		{ (SDLTest_TestCaseFp)video_getWindowGammaRamp, "video_getWindowGammaRamp",  "Get window gamma ramp", TEST_ENABLED };

static const SDLTest_TestCaseReference videoTest15 =
		{ (SDLTest_TestCaseFp)video_getWindowGammaRampNegative, "video_getWindowGammaRampNegative",  "Get window gamma ramp against invalid input", TEST_ENABLED };

/* Sequence of Video test cases */
static const SDLTest_TestCaseReference *videoTests[] =  {
	&videoTest1, &videoTest2, &videoTest3, &videoTest4, &videoTest5, &videoTest6, 
	&videoTest7, &videoTest8, &videoTest9, &videoTest10, &videoTest11, &videoTest12, 
	&videoTest13, &videoTest14, &videoTest15, NULL
};

/* Video test suite (global) */
SDLTest_TestSuiteReference videoTestSuite = {
	"Video",
	NULL,
	videoTests,
	NULL
};
