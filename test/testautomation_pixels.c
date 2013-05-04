/**
 * Pixels test suite
 */

#include <stdio.h>

#include "SDL.h"
#include "SDL_test.h"

/* Test case functions */

/* Definition of all RGB formats used to test pixel conversions */
const int _numRGBPixelFormats = 30;
Uint32 _RGBPixelFormats[] = 
  {
    SDL_PIXELFORMAT_INDEX1LSB,
    SDL_PIXELFORMAT_INDEX1MSB,
    SDL_PIXELFORMAT_INDEX4LSB,
    SDL_PIXELFORMAT_INDEX4MSB,
    SDL_PIXELFORMAT_INDEX8,
    SDL_PIXELFORMAT_RGB332,
    SDL_PIXELFORMAT_RGB444,
    SDL_PIXELFORMAT_RGB555,
    SDL_PIXELFORMAT_BGR555,
    SDL_PIXELFORMAT_ARGB4444,
    SDL_PIXELFORMAT_RGBA4444,
    SDL_PIXELFORMAT_ABGR4444,
    SDL_PIXELFORMAT_BGRA4444,
    SDL_PIXELFORMAT_ARGB1555,
    SDL_PIXELFORMAT_RGBA5551,
    SDL_PIXELFORMAT_ABGR1555,
    SDL_PIXELFORMAT_BGRA5551,
    SDL_PIXELFORMAT_RGB565,
    SDL_PIXELFORMAT_BGR565,
    SDL_PIXELFORMAT_RGB24,
    SDL_PIXELFORMAT_BGR24,
    SDL_PIXELFORMAT_RGB888,
    SDL_PIXELFORMAT_RGBX8888,
    SDL_PIXELFORMAT_BGR888,
    SDL_PIXELFORMAT_BGRX8888,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_PIXELFORMAT_ABGR8888,
    SDL_PIXELFORMAT_BGRA8888,
    SDL_PIXELFORMAT_ARGB2101010
  };
char* _RGBPixelFormatsVerbose[] = 
  {
    "SDL_PIXELFORMAT_INDEX1LSB",
    "SDL_PIXELFORMAT_INDEX1MSB",
    "SDL_PIXELFORMAT_INDEX4LSB",
    "SDL_PIXELFORMAT_INDEX4MSB",
    "SDL_PIXELFORMAT_INDEX8",
    "SDL_PIXELFORMAT_RGB332",
    "SDL_PIXELFORMAT_RGB444",
    "SDL_PIXELFORMAT_RGB555",
    "SDL_PIXELFORMAT_BGR555",
    "SDL_PIXELFORMAT_ARGB4444",
    "SDL_PIXELFORMAT_RGBA4444",
    "SDL_PIXELFORMAT_ABGR4444",
    "SDL_PIXELFORMAT_BGRA4444",
    "SDL_PIXELFORMAT_ARGB1555",
    "SDL_PIXELFORMAT_RGBA5551",
    "SDL_PIXELFORMAT_ABGR1555",
    "SDL_PIXELFORMAT_BGRA5551",
    "SDL_PIXELFORMAT_RGB565",
    "SDL_PIXELFORMAT_BGR565",
    "SDL_PIXELFORMAT_RGB24",
    "SDL_PIXELFORMAT_BGR24",
    "SDL_PIXELFORMAT_RGB888",
    "SDL_PIXELFORMAT_RGBX8888",
    "SDL_PIXELFORMAT_BGR888",
    "SDL_PIXELFORMAT_BGRX8888",
    "SDL_PIXELFORMAT_ARGB8888",
    "SDL_PIXELFORMAT_RGBA8888",
    "SDL_PIXELFORMAT_ABGR8888",
    "SDL_PIXELFORMAT_BGRA8888",
    "SDL_PIXELFORMAT_ARGB2101010"
  };

/* Definition of all Non-RGB formats used to test pixel conversions */
const int _numNonRGBPixelFormats = 5;
Uint32 _nonRGBPixelFormats[] = 
  {
    SDL_PIXELFORMAT_YV12,
    SDL_PIXELFORMAT_IYUV,
    SDL_PIXELFORMAT_YUY2,
    SDL_PIXELFORMAT_UYVY,
    SDL_PIXELFORMAT_YVYU 
  };
char* _nonRGBPixelFormatsVerbose[] = 
  {
    "SDL_PIXELFORMAT_YV12",
    "SDL_PIXELFORMAT_IYUV",
    "SDL_PIXELFORMAT_YUY2",
    "SDL_PIXELFORMAT_UYVY",
    "SDL_PIXELFORMAT_YVYU" 
  };

/* Test case functions */

/**
 * @brief Call to SDL_AllocFormat and SDL_FreeFormat
 *
 * @sa http://wiki.libsdl.org/moin.fcg/SDL_AllocFormat
 * @sa http://wiki.libsdl.org/moin.fcg/SDL_FreeFormat 
 */
int
pixels_allocFreeFormat(void *arg)
{
  int i;
  Uint32 format;
  Uint32 masks;
  SDL_PixelFormat* result;

  /* RGB formats */
  for (i = 0; i < _numRGBPixelFormats; i++) {
    format = _RGBPixelFormats[i];
    SDLTest_Log("RGB Format: %s (%u)", _RGBPixelFormatsVerbose[i], format);
  
    /* Allocate format */
    result = SDL_AllocFormat(format);
    SDLTest_AssertPass("Call to SDL_AllocFormat()");
    SDLTest_AssertCheck(result != NULL, "Verify result is not NULL");  
    if (result != NULL) {
      SDLTest_AssertCheck(result->format == format, "Verify value of result.format; expected: %u, got %u", format, result->format);  
      SDLTest_AssertCheck(result->BitsPerPixel > 0, "Verify value of result.BitsPerPixel; expected: >0, got %u", result->BitsPerPixel);  
      SDLTest_AssertCheck(result->BytesPerPixel > 0, "Verify value of result.BytesPerPixel; expected: >0, got %u", result->BytesPerPixel);  
      if (result->palette != NULL) {      
         masks = result->Rmask | result->Gmask | result->Bmask | result->Amask;
         SDLTest_AssertCheck(masks > 0, "Verify value of result.[RGBA]mask combined; expected: >0, got %u", masks);
      }
      
      /* Deallocate again */
      SDL_FreeFormat(result);
      SDLTest_AssertPass("Call to SDL_FreeFormat()");          
    }
  }

  /* Non-RGB formats */
  for (i = 0; i < _numNonRGBPixelFormats; i++) {
    format = _nonRGBPixelFormats[i];
    SDLTest_Log("non-RGB Format: %s (%u)", _nonRGBPixelFormatsVerbose[i], format);
  
    /* Try to allocate format */
    result = SDL_AllocFormat(format);
    SDLTest_AssertPass("Call to SDL_AllocFormat()");
    SDLTest_AssertCheck(result == NULL, "Verify result is NULL");  
  }
  
  /* Negative cases */
  format = 0xffffffff;
  result = SDL_AllocFormat(format);
  SDLTest_AssertPass("Call to SDL_AllocFormat(0xffffffff)");
  SDLTest_AssertCheck(result == NULL, "Verify result is NULL");  
  /* TODO: check error code */

  SDL_FreeFormat(NULL);
  SDLTest_AssertPass("Call to SDL_FreeFormat(NULL)");
  /* TODO: check error code */
  
  return TEST_COMPLETED;
}

/**
 * @brief Call to SDL_AllocPalette and SDL_FreePalette
 *
 * @sa http://wiki.libsdl.org/moin.fcg/SDL_AllocPalette
 * @sa http://wiki.libsdl.org/moin.fcg/SDL_FreePalette
 */
int
pixels_allocFreePalette(void *arg)
{
  int variation;
  int i;
  int ncolors;
  SDL_Palette* result;

  /* Allocate palette */
  for (variation = 1; variation <= 3; variation++) {  
    switch (variation) {
      /* Just one color */
      case 1:
        ncolors = 1;
        break;
      /* Two colors */
      case 2:
        ncolors = 2;
        break;
      /* More than two colors */
      case 3:
        ncolors = SDLTest_RandomIntegerInRange(8, 16);
        break;
    }

    result = SDL_AllocPalette(ncolors);
    SDLTest_AssertPass("Call to SDL_AllocPalette(%d)", ncolors);
    SDLTest_AssertCheck(result != NULL, "Verify result is not NULL");  
    if (result != NULL) {
      SDLTest_AssertCheck(result->ncolors == ncolors, "Verify value of result.ncolors; expected: %u, got %u", ncolors, result->ncolors);  
      if (result->ncolors > 0) {
        SDLTest_AssertCheck(result->colors != NULL, "Verify value of result.colors is not NULL");
        if (result->colors != NULL) {
          for(i = 0; i < result->ncolors; i++) {
            SDLTest_AssertCheck(result->colors[i].r == 255, "Verify value of result.colors[%d].r; expected: 255, got %u", i, result->colors[i].r);  
            SDLTest_AssertCheck(result->colors[i].g == 255, "Verify value of result.colors[%d].g; expected: 255, got %u", i, result->colors[i].g);  
            SDLTest_AssertCheck(result->colors[i].b == 255, "Verify value of result.colors[%d].b; expected: 255, got %u", i, result->colors[i].b);  
           }
         } 
      }
      
      /* Deallocate again */
      SDL_FreePalette(result);
      SDLTest_AssertPass("Call to SDL_FreePalette()");          
    }
  }

  /* Negative cases */
  for (ncolors = 0; ncolors > -3; ncolors--) {
    result = SDL_AllocPalette(ncolors);
    SDLTest_AssertPass("Call to SDL_AllocPalette(%d)", ncolors);
    SDLTest_AssertCheck(result == NULL, "Verify result is NULL");
    /* TODO: check error code */
  }

  SDL_FreePalette(NULL);
  SDLTest_AssertPass("Call to SDL_FreePalette(NULL)");
  /* TODO: check error code */
    
  return TEST_COMPLETED;
}

/* ================= Test References ================== */

/* Pixels test cases */
static const SDLTest_TestCaseReference pixelsTest1 =
		{ (SDLTest_TestCaseFp)pixels_allocFreeFormat, "pixels_allocFreeFormat", "Call to SDL_AllocFormat and SDL_FreeFormat", TEST_ENABLED };

static const SDLTest_TestCaseReference pixelsTest2 =
		{ (SDLTest_TestCaseFp)pixels_allocFreePalette, "pixels_allocFreePalette", "Call to SDL_AllocPalette and SDL_FreePalette", TEST_ENABLED };

/* Sequence of Pixels test cases */
static const SDLTest_TestCaseReference *pixelsTests[] =  {
	&pixelsTest1, &pixelsTest2, NULL
};

/* Pixels test suite (global) */
SDLTest_TestSuiteReference pixelsTestSuite = {
	"Pixels",
	NULL,
	pixelsTests,
	NULL
};
