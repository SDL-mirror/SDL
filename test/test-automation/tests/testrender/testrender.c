/**
 * Original code: automated SDL platform test written by Edgar Simo "bobbens"
 * Extended and updated by aschiffler at ferzkopp dot net
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../../include/SDL_test.h"

#define SCREEN_W     80
#define SCREEN_H     60

#define FACE_W       img_face.width
#define FACE_H       img_face.height

static SDL_Renderer *renderer;

/*!
 * Note: Port tests from "/test/automated/render" here
 *
 */

/* Test cases */
static const TestCaseReference test1 =
		(TestCaseReference){ "render_testGetNumRenderDrivers", "Tests call to SDL_GetNumRenderDrivers", TEST_ENABLED, 0, 0 };

static const TestCaseReference test2 =
		(TestCaseReference){ "render_testCreateRenderer", "Tests SDL_CreateRenderer", TEST_ENABLED, 0, 0 };

static const TestCaseReference test3 =
		(TestCaseReference){ "render_testPrimitives", "Tests rendering primitives", TEST_ENABLED, 0, 0 };

static const TestCaseReference test4 =
		(TestCaseReference){ "render_testPrimitivesBlend", "Tests rendering primitives with blending", TEST_ENABLED, 0, 0 };

static const TestCaseReference test5 =
		(TestCaseReference){ "render_testBlit", "Tests blitting", TEST_ENABLED, 0, 0 };

static const TestCaseReference test6 =
		(TestCaseReference){ "render_testBlitColour", "Tests blitting with color", TEST_ENABLED, 0, 0 };

static const TestCaseReference test7 =
		(TestCaseReference){ "render_testBlitAlpha", "Tests blitting with alpha", TEST_ENABLED, 0, 0 };

static const TestCaseReference test8 =
		(TestCaseReference){ "render_testBlitBlend", "Tests blitting with blending", TEST_ENABLED, 0, 0 };





/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, &test4, &test5, &test6, &test7, &test8, NULL
};

TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

// Fixture

void
SetUp(void *arg)
{
  /* Start SDL. */
  int ret = SDL_InitSubSystem( SDL_INIT_VIDEO );
  AssertTrue(ret==0, "SDL_Init(SDL_INIT_VIDEO): %s", SDL_GetError());

  SDL_Window *w = SDL_CreateWindow( "title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		  80, 60, SDL_WINDOW_SHOWN );
  //if (SDL_ATassert( "SDL_CreateWindow", w!=NULL ))
  renderer = SDL_CreateRenderer(w, 0, 0 );
}
                        
void
TearDown(void *arg)
{
  /* Quit SDL video */
 SDL_QuitSubSystem(SDL_INIT_VIDEO);
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



/**
 * @brief Compares screen pixels with image pixels.
 *
 *    @param msg Message on failure.
 *    @param s Image to compare against.
 *    @return 0 on success.
 */
static void render_compare( const char *msg, const SurfaceImage_t *s, int allowable_error )
{
   int ret;
   SDL_Rect rect;
   Uint8 pix[4*80*60];
   SDL_Surface *testsur;

   /* Read pixels. */
   /* Explicitly specify the rect in case the window isn't expected size... */
   rect.x = 0;
   rect.y = 0;
   rect.w = 80;
   rect.h = 60;
   ret = SDL_RenderReadPixels(renderer, &rect, FORMAT, pix, 80*4 );
   AssertEquals(ret, 0, "SDL_RenderReadPixels failed");

   /* Create surface. */
   testsur = SDL_CreateRGBSurfaceFrom( pix, 80, 60, 32, 80*4,
                                       RMASK, GMASK, BMASK, AMASK);
   AssertTrue(testsur!=NULL, "SDL_CreateRGBSurface failed");
   /* Compare surface. */
   ret = surface_compare( testsur, s, allowable_error );
   AssertEquals(ret, 0, "surface_compare failed");

   /* Clean up. */
   SDL_FreeSurface( testsur );
}

/**
 * @brief Checks to see if functionality is supported.
 */
static int render_isSupported( int code )
{
   return (code == 0);
}


/**
 * @brief Test to see if we can vary the draw colour.
 */
static int render_hasDrawColor (void)
{
   int ret, fail;
   Uint8 r, g, b, a;

   fail = 0;

   /* Set colour. */
   ret = SDL_SetRenderDrawColor(renderer, 100, 100, 100, 100 );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a );
   if (!render_isSupported(ret))
      fail = 1;
   /* Restore natural. */
   ret = SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
   if (!render_isSupported(ret))
      fail = 1;

   /* Something failed, consider not available. */
   if (fail)
      return 0;
   /* Not set properly, consider failed. */
   else if ((r != 100) || (g != 100) || (b != 100) || (a != 100))
      return 0;
   return 1;
}


/**
 * @brief Test to see if we can vary the blend mode.
 */
static int render_hasBlendModes (void)
{
   int fail;
   int ret;
   SDL_BlendMode mode;

   fail = 0;

   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawBlendMode(renderer, &mode );
   if (!render_isSupported(ret))
      fail = 1;
   ret = (mode != SDL_BLENDMODE_BLEND);
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawBlendMode(renderer, &mode );
   if (!render_isSupported(ret))
      fail = 1;
   ret = (mode != SDL_BLENDMODE_ADD);
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawBlendMode(renderer, &mode );
   if (!render_isSupported(ret))
      fail = 1;
   ret = (mode != SDL_BLENDMODE_MOD);
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawBlendMode(renderer, &mode );
   if (!render_isSupported(ret))
      fail = 1;
   ret = (mode != SDL_BLENDMODE_NONE);
   if (!render_isSupported(ret))
      fail = 1;

   return !fail;
}


/**
 * @brief Loads the test face.
 */
static SDL_Texture * render_loadTestFace (void)
{
   SDL_Surface *face;
   SDL_Texture *tface;

   /* Create face surface. */
   face = SDL_CreateRGBSurfaceFrom( (void*)img_face.pixel_data,
         img_face.width, img_face.height, 32, img_face.width*4,
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
         0xff000000, /* Red bit mask. */
         0x00ff0000, /* Green bit mask. */
         0x0000ff00, /* Blue bit mask. */
         0x000000ff /* Alpha bit mask. */
#else
         0x000000ff, /* Red bit mask. */
         0x0000ff00, /* Green bit mask. */
         0x00ff0000, /* Blue bit mask. */
         0xff000000 /* Alpha bit mask. */
#endif
         );
   if (face == NULL)
      return 0;
   tface = SDL_CreateTextureFromSurface(renderer, face);
   SDL_FreeSurface(face);

   return tface;
}


/**
 * @brief Test to see if can set texture colour mode.
 */
static int render_hasTexColor (void)
{
   int fail;
   int ret;
   SDL_Texture *tface;
   Uint8 r, g, b;

   /* Get test face. */
   tface = render_loadTestFace();
   if (tface == 0)
      return 0;

   /* See if supported. */
   fail = 0;
   ret = SDL_SetTextureColorMod( tface, 100, 100, 100 );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetTextureColorMod( tface, &r, &g, &b );
   if (!render_isSupported(ret))
      fail = 1;

   /* Clean up. */
   SDL_DestroyTexture( tface );

   if (fail)
      return 0;
   else if ((r != 100) || (g != 100) || (b != 100))
      return 0;
   return 1;
}


/**
 * @brief Test to see if we can vary the alpha of the texture.
 */
static int render_hasTexAlpha (void)
{
   int fail;
   int ret;
   SDL_Texture *tface;
   Uint8 a;

   /* Get test face. */
   tface = render_loadTestFace();
   if (tface == 0)
      return 0;

   /* See if supported. */
   fail = 0;
   ret = SDL_SetTextureAlphaMod( tface, 100 );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetTextureAlphaMod( tface, &a );
   if (!render_isSupported(ret))
      fail = 1;

   /* Clean up. */
   SDL_DestroyTexture( tface );

   if (fail)
      return 0;
   else if (a != 100)
      return 0;
   return 1;
}


/**
 * @brief Tests the SDL primitives for rendering.
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_SetRenderDrawColor
 * http://wiki.libsdl.org/moin.cgi/SDL_RenderFillRect
 * http://wiki.libsdl.org/moin.cgi/SDL_RenderDrawLine
 *
 */
int render_testPrimitives (void *arg)
{
   int ret;
   int x, y;
   SDL_Rect rect;


   /* Need drawcolour or just skip test. */
   AssertTrue(render_hasDrawColor(), "hasDrawColor");

   /* Draw a rectangle. */
   rect.x = 40;
   rect.y = 0;
   rect.w = 40;
   rect.h = 80;

   ret = SDL_SetRenderDrawColor(renderer, 13, 73, 200, SDL_ALPHA_OPAQUE );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor");

   ret = SDL_RenderFillRect(renderer, &rect );
   AssertEquals(ret, 0,"SDL_RenderFillRect");

   /* Draw a rectangle. */
   rect.x = 10;
   rect.y = 10;
   rect.w = 60;
   rect.h = 40;
   ret = SDL_SetRenderDrawColor(renderer, 200, 0, 100, SDL_ALPHA_OPAQUE );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor");

   ret = SDL_RenderFillRect(renderer, &rect );
   AssertEquals(ret, 0, "SDL_RenderFillRect");

   /* Draw some points like so:
    * X.X.X.X..
    * .X.X.X.X.
    * X.X.X.X.. */
   for (y=0; y<3; y++) {
      x = y % 2;
      for (; x<80; x+=2) {
         ret = SDL_SetRenderDrawColor(renderer, x*y, x*y/2, x*y/3, SDL_ALPHA_OPAQUE );
         AssertEquals(ret, 0, "SDL_SetRenderDrawColor");

         ret = SDL_RenderDrawPoint(renderer, x, y );
         AssertEquals(ret, 0, "SDL_RenderDrawPoint");
      }
   }

   /* Draw some lines. */
   ret = SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor");

   ret = SDL_RenderDrawLine(renderer, 0, 30, 80, 30 );
   AssertEquals(ret, 0, "SDL_RenderDrawLine");

   ret = SDL_SetRenderDrawColor(renderer, 55, 55, 5, SDL_ALPHA_OPAQUE );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor");

   ret = SDL_RenderDrawLine(renderer, 40, 30, 40, 60 );
   AssertEquals(ret, 0, "SDL_RenderDrawLine");

   ret = SDL_SetRenderDrawColor(renderer, 5, 105, 105, SDL_ALPHA_OPAQUE );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor");

   ret = SDL_RenderDrawLine(renderer, 0, 0, 29, 29 );
   AssertEquals(ret, 0, "SDL_RenderDrawLine");

   ret = SDL_RenderDrawLine(renderer, 29, 30, 0, 59 );
   AssertEquals(ret, 0, "SDL_RenderDrawLine");

   ret = SDL_RenderDrawLine(renderer, 79, 0, 50, 29 );
   AssertEquals(ret, 0, "SDL_RenderDrawLine");

   ret = SDL_RenderDrawLine(renderer, 79, 59, 50, 30 );
   AssertEquals(ret, 0, "SDL_RenderDrawLine");

   /* See if it's the same. */
   render_compare( "Primitives output not the same.", &img_primitives, ALLOWABLE_ERROR_OPAQUE );
}


/**
 * @brief Tests the SDL primitives with alpha for rendering.
 */
int render_testPrimitivesBlend (void *arg)
{
   int ret;
   int i, j;
   SDL_Rect rect;

   /* Need drawcolour and blendmode or just skip test. */
   AssertTrue(render_hasDrawColor(), "render_hasDrawColor");
   AssertTrue(render_hasBlendModes(), "render_hasDrawColor");

   /* Create some rectangles for each blend mode. */
   ret = SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0 );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor" );

   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE );
   AssertEquals(ret, 0, "SDL_SetRenderDrawBlendMode" );

   ret = SDL_RenderFillRect(renderer, NULL );
   AssertEquals(ret, 0, "SDL_RenderFillRect" );

   rect.x = 10;
   rect.y = 25;
   rect.w = 40;
   rect.h = 25;
   ret = SDL_SetRenderDrawColor(renderer, 240, 10, 10, 75 );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor" );

   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD );
   AssertEquals(ret, 0, "SDL_SetRenderDrawBlendMode" );

   ret = SDL_RenderFillRect(renderer, &rect );
   AssertEquals(ret, 0, "SDL_RenderFillRect" );

   rect.x = 30;
   rect.y = 40;
   rect.w = 45;
   rect.h = 15;
   ret = SDL_SetRenderDrawColor(renderer, 10, 240, 10, 100 );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor" );

   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND );
   AssertEquals(ret, 0, "SDL_SetRenderDrawBlendMode" );

   ret = SDL_RenderFillRect(renderer, &rect );
   AssertEquals(ret, 0, "SDL_RenderFillRect" );

   rect.x = 25;
   rect.y = 25;
   rect.w = 25;
   rect.h = 25;
   ret = SDL_SetRenderDrawColor(renderer, 10, 10, 240, 125 );
   AssertEquals(ret, 0, "SDL_SetRenderDrawColor" );

   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE );
   AssertEquals(ret, 0, "SDL_SetRenderDrawBlendMode" );

   ret = SDL_RenderFillRect(renderer, &rect );
   AssertEquals(ret, 0, "SDL_RenderFillRect" );


   /* Draw blended lines, lines for everyone. */
   for (i=0; i<SCREEN_W; i+=2)  {
      ret = SDL_SetRenderDrawColor(renderer, 60+2*i, 240-2*i, 50, 3*i );
      AssertEquals(ret, 0, "SDL_SetRenderDrawColor" );

      ret = SDL_SetRenderDrawBlendMode(renderer,(((i/2)%3)==0) ? SDL_BLENDMODE_BLEND :
            (((i/2)%3)==1) ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_NONE );
      AssertEquals(ret, 0, "SDL_SetRenderDrawBlendMode" );

      ret = SDL_RenderDrawLine(renderer, 0, 0, i, 59 );
      AssertEquals(ret, 0, "SDL_RenderDrawLine" );
   }
   for (i=0; i<SCREEN_H; i+=2)  {
      ret = SDL_SetRenderDrawColor(renderer, 60+2*i, 240-2*i, 50, 3*i );
      AssertEquals(ret, 0, "SDL_SetRenderDrawColor" );

      ret = SDL_SetRenderDrawBlendMode(renderer,(((i/2)%3)==0) ? SDL_BLENDMODE_BLEND :
            (((i/2)%3)==1) ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_NONE );

      AssertEquals(ret, 0, "SDL_SetRenderDrawBlendMode" );

      ret = SDL_RenderDrawLine(renderer, 0, 0, 79, i );

      AssertEquals(ret, 0, "SDL_RenderDrawLine" );
   }

   /* Draw points. */
   for (j=0; j<SCREEN_H; j+=3) {
      for (i=0; i<SCREEN_W; i+=3) {
         ret = SDL_SetRenderDrawColor(renderer, j*4, i*3, j*4, i*3 );
         AssertEquals(ret, 0, "SDL_SetRenderDrawColor" );

         ret = SDL_SetRenderDrawBlendMode(renderer, ((((i+j)/3)%3)==0) ? SDL_BLENDMODE_BLEND :
               ((((i+j)/3)%3)==1) ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_NONE );

         AssertEquals(ret, 0, "SDL_SetRenderDrawBlendMode" );

         ret = SDL_RenderDrawPoint(renderer, i, j );
         AssertEquals(ret, 0, "SDL_RenderDrawPoint" );
      }
   }

   /* See if it's the same. FIXME needs assert */
   render_compare( "Blended primitives output not the same.", &img_blend, ALLOWABLE_ERROR_BLENDED );
}


/**
 * @brief Tests some blitting routines.
 */
int
render_testBlit(void *arg)
{
   int ret;
   SDL_Rect rect;
   SDL_Texture *tface;
   int i, j, ni, nj;


   /* Need drawcolour or just skip test. */
   AssertTrue(render_hasDrawColor(), "render_hasDrawColor)");

   /* Create face surface. */
   tface = render_loadTestFace();
   AssertTrue(tface != 0,  "render_loadTestFace()");

   /* Constant values. */
   rect.w = img_face.width;
   rect.h = img_face.height;
   ni     = SCREEN_W - img_face.width;
   nj     = SCREEN_H - img_face.height;

   /* Loop blit. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         AssertEquals(ret, 0, "SDL_RenderCopy");
      }
   }

   /* Clean up. */
   SDL_DestroyTexture( tface );

   /* See if it's the same. FIXME needs assert */
   render_compare( "Blit output not the same.", &img_blit, ALLOWABLE_ERROR_OPAQUE );
}

/**
 * @brief Blits doing colour tests.
 */
int
render_testBlitColour (void *arg)
{
   int ret;
   SDL_Rect rect;
   SDL_Texture *tface;
   int i, j, ni, nj;

   /* Create face surface. */
   tface = render_loadTestFace();

   AssertTrue(tface != 0, "render_loadTestFace()");

   /* Constant values. */
   rect.w = img_face.width;
   rect.h = img_face.height;
   ni     = SCREEN_W - img_face.width;
   nj     = SCREEN_H - img_face.height;

   /* Test blitting with colour mod. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set colour mod. */
         ret = SDL_SetTextureColorMod( tface, (255/nj)*j, (255/ni)*i, (255/nj)*j );
         AssertEquals(ret, 0, "SDL_SetTextureColorMod");

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         AssertEquals(ret, 0, "SDL_RenderCopy");
      }
   }

   /* Clean up. */
   SDL_DestroyTexture( tface );

   /* See if it's the same. */
   render_compare( "Blit output not the same (using SDL_SetTextureColorMod).",
            &img_blitColour, ALLOWABLE_ERROR_OPAQUE );
}

/**
 * @brief Tests blitting with alpha.
 */
int
render_testBlitAlpha (void *arg)
{
   int ret;
   SDL_Rect rect;
   SDL_Texture *tface;
   int i, j, ni, nj;

   /* Need alpha or just skip test. */
   AssertTrue(render_hasTexAlpha(), "render_hasTexAlpha");

   /* Create face surface. */
   tface = render_loadTestFace();
   AssertTrue(tface != 0, "render_loadTestFace()");

   /* Constant values. */
   rect.w = img_face.width;
   rect.h = img_face.height;
   ni     = SCREEN_W - img_face.width;
   nj     = SCREEN_H - img_face.height;

   /* Test blitting with alpha mod. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set alpha mod. */
         ret = SDL_SetTextureAlphaMod( tface, (255/ni)*i );
         AssertEquals(ret, 0, "SDL_SetTextureAlphaMod");

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         AssertEquals(ret, 0, "SDL_RenderCopy");
      }
   }

   /* Clean up. */
   SDL_DestroyTexture( tface );

   /* See if it's the same. */
   render_compare( "Blit output not the same (using SDL_SetSurfaceAlphaMod).",
            &img_blitAlpha, ALLOWABLE_ERROR_BLENDED );
}

/**
 * @brief Clears the screen.
 *
 * @note We don't test for errors, but they shouldn't happen.
 */
static int render_clearScreen (void)
{
   int ret;

   /* Set colour. */
   ret = SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
   /*
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   */

   /* Clear screen. */
   ret = SDL_RenderFillRect(renderer, NULL );
   /*
   if (SDL_ATassert( "SDL_RenderFillRect", ret == 0))
      return -1;
   */

   /* Set defaults. */
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE );
   /*
   if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
      return -1;
   */
   ret = SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE );
   /*
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   */

   return 0;
}

/**
 * @brief Tests a blend mode. Helper.
 */
void
render_testBlitBlendMode( SDL_Texture * tface, int mode )
{
   int ret;
   int i, j, ni, nj;
   SDL_Rect rect;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Steps to take. */
   ni     = SCREEN_W - FACE_W;
   nj     = SCREEN_H - FACE_H;

   /* Constant values. */
   rect.w = FACE_W;
   rect.h = FACE_H;

   /* Test blend mode. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set blend mode. */
         ret = SDL_SetTextureBlendMode( tface, mode );
         AssertEquals(ret, 0, "SDL_SetTextureBlendMode");

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         AssertEquals(ret, 0, "SDL_RenderCopy");
      }
   }
}




/**
 * @brief Tests some more blitting routines.
 */
int
render_testBlitBlend (void *arg)
{
   int ret;
   SDL_Rect rect;
   SDL_Texture *tface;
   int i, j, ni, nj;
   int mode;


   AssertTrue(render_hasBlendModes(), "render_hasBlendModes");
   AssertTrue(render_hasTexColor(), "render_hasTexColor");
   AssertTrue(render_hasTexAlpha(), "render_hasTexAlpha");

   /* Create face surface. */
   tface = render_loadTestFace();
   AssertTrue(tface != 0, "render_loadTestFace()");

   /* Steps to take. */
   ni     = SCREEN_W - FACE_W;
   nj     = SCREEN_H - FACE_H;

   /* Constant values. */
   rect.w = img_face.width;
   rect.h = img_face.height;

   /* Set alpha mod. */
   ret = SDL_SetTextureAlphaMod( tface, 100 );
   AssertEquals(ret, 0, "SDL_SetTextureAlphaMod");

   /* Test None. */
   render_testBlitBlendMode( tface, SDL_BLENDMODE_NONE );
   /* See if it's the same. */
   render_compare( "Blit blending output not the same (using SDL_BLENDMODE_NONE).",
            &img_blendNone, ALLOWABLE_ERROR_OPAQUE ); //FIXME add assert


   /* Test Blend. */
   render_testBlitBlendMode( tface, SDL_BLENDMODE_BLEND );
   render_compare( "Blit blending output not the same (using SDL_BLENDMODE_BLEND).",
            &img_blendBlend, ALLOWABLE_ERROR_BLENDED ); //FIXME add assert


   /* Test Add. */
   render_testBlitBlendMode( tface, SDL_BLENDMODE_ADD );

  render_compare( "Blit blending output not the same (using SDL_BLENDMODE_ADD).",
            &img_blendAdd, ALLOWABLE_ERROR_BLENDED ); //FIXME add assert

   /* Test Mod. */
   render_testBlitBlendMode( tface, SDL_BLENDMODE_MOD);

   render_compare( "Blit blending output not the same (using SDL_BLENDMODE_MOD).",
            &img_blendMod, ALLOWABLE_ERROR_BLENDED );

   /* Clear surface. */
   render_clearScreen();

   /* Loop blit. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {

         /* Set colour mod. */
         ret = SDL_SetTextureColorMod( tface, (255/nj)*j, (255/ni)*i, (255/nj)*j );
         AssertEquals(ret, 0, "SDL_SetTextureColorMod");

         /* Set alpha mod. */
         ret = SDL_SetTextureAlphaMod( tface, (100/ni)*i );
         AssertEquals(ret, 0, "SDL_SetTextureAlphaMod");

         /* Crazy blending mode magic. */
         mode = (i/4*j/4) % 4;
         if (mode==0) mode = SDL_BLENDMODE_NONE;
         else if (mode==1) mode = SDL_BLENDMODE_BLEND;
         else if (mode==2) mode = SDL_BLENDMODE_ADD;
         else if (mode==3) mode = SDL_BLENDMODE_MOD;
         ret = SDL_SetTextureBlendMode( tface, mode );
         AssertEquals(ret, 0, "SDL_SetTextureBlendMode");

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         AssertEquals(ret, 0, "SDL_RenderCopy");
      }
   }

   /* Clean up. */
   SDL_DestroyTexture( tface );

   /* Check to see if matches. */
   render_compare( "Blit blending output not the same (using SDL_BLENDMODE_*).",
            &img_blendAll, ALLOWABLE_ERROR_BLENDED); // FIXME add assert
}



