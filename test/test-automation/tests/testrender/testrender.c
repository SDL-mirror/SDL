/**
 * Original code: automated SDL platform test written by Edgar Simo "bobbens"
 * Extended and updated by aschiffler at ferzkopp dot net
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../../include/SDL_test.h"



/*!
 * Note: Port tests from "/test/automated/render" here
 *
 */

/* Test cases */
static const TestCaseReference test1 =
		(TestCaseReference){ "render_testGetNumRenderDrivers", "Tests call to SDL_GetNumRenderDrivers", TEST_ENABLED, 0, 0 };

static const TestCaseReference test2 =
		(TestCaseReference){ "render_testCreateRenderer", "Tests SDL_CreateRenderer", TEST_ENABLED, 0, 0 };

/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, NULL
};

TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

// Fixture

void
SetUp(void *arg)
{
  /* Start SDL. */
  int ret = SDL_Init( SDL_INIT_VIDEO );
  AssertTrue(ret==0, "SDL_Init(SDL_INIT_VIDEO): %s", SDL_GetError());
}
                        
void
TearDown(void *arg)
{
  /* Quit SDL. */
  SDL_Quit();
}

/**
 * @brief Tests call to SDL_GetNumRenderDrivers
 * \sa 
 * http://wiki.libsdl.org/moin.cgi/SDL_GetNumRenderDrivers
 */
int
render_testGetNumRenderDrivers(void *arg)
{
  int n;
  n = SDL_GetNumRenderDrivers();
  AssertTrue(n>=1, "Number of renderers >= 1, reported as %i", n);
  if (n<0) {
    AssertFail("SDL_GetNumRenderDrivers() failed: %s", SDL_GetError());
  }
}

/**
 * @brief Tests call to SDL_CreateRenderer
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_CreateRenderer
 */
int
render_testCreateRenderer(void *arg)
{
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  int posX = 100, posY = 100, width = 320, height = 240;
  window = SDL_CreateWindow("Hello World", posX, posY, width, height, 0);
  if (window != NULL) {
    AssertPass("Window created");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer) {    
      AssertPass("Renderer created");
      SDL_DestroyRenderer(renderer);
    } else {
      AssertFail("Could not create renderer: %s", SDL_GetError());
    }
    SDL_DestroyWindow(window);
  } else {
    AssertFail("Could not create window: %s", SDL_GetError());
  }  
}
