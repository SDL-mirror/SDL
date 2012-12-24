/**
 * Original code: automated SDL audio test written by Edgar Simo "bobbens"
 * New/updated tests: aschiffler at ferzkopp dot net
 */

#include <stdio.h>

#include "SDL.h"
#include "SDL_test.h"

/* ================= Test Case Implementation ================== */

/* Fixture */

void
_audioSetUp(void *arg)
{
	/* Start SDL audio subsystem */
	int ret = SDL_InitSubSystem( SDL_INIT_AUDIO );
        SDLTest_AssertPass("Call to SDL_InitSubSystem(SDL_INIT_AUDIO)");
	SDLTest_AssertCheck(ret==0, "Check result from SDL_InitSubSystem(SDL_INIT_AUDIO)");
	if (ret != 0) {
           SDLTest_LogError("%s", SDL_GetError());
        }
}


/* Test case functions */

/**
 * \brief Enumerate and name available audio devices (output and capture).
 * 
 * \sa http://wiki.libsdl.org/moin.cgi/SDL_GetNumAudioDevices
 * \sa http://wiki.libsdl.org/moin.cgi/SDL_GetAudioDeviceName
 */
int audio_enumerateAndNameAudioDevices()
{
   int t, tt;
   int i, n, nn;
   const char *name, *nameAgain;

   /* Iterate over types: t=0 output device, t=1 input/capture device */
   for (t=0; t<2; t++) {
   
      /* Get number of devices. */
      n = SDL_GetNumAudioDevices(t);
      SDLTest_AssertPass("Call to SDL_GetNumAudioDevices(%i)", t);
      SDLTest_Log("Number of %s devices < 0, reported as %i", (t) ? "output" : "capture", n);
      SDLTest_AssertCheck(n >= 0, "Validate result is >= 0, got: %i", n);

      /* Variation of non-zero type */
      if (t==1) {
         tt = t + SDLTest_RandomIntegerInRange(1,10);
         nn = SDL_GetNumAudioDevices(tt);
         SDLTest_AssertCheck(n==nn, "Verify result from SDL_GetNumAudioDevices(%i), expected same number of audio devices %i, got %i", tt, n, nn);
         nn = SDL_GetNumAudioDevices(-tt);
         SDLTest_AssertCheck(n==nn, "Verify result from SDL_GetNumAudioDevices(%i), expected same number of audio devices %i, got %i", -tt, n, nn);
      } 
   
      /* List devices. */
      if (n>0) {
         for (i=0; i<n; i++) {
            name = SDL_GetAudioDeviceName(i, t);
            SDLTest_AssertPass("Call to SDL_GetAudioDeviceName(%i, %i)", i, t);
            SDLTest_AssertCheck(name != NULL, "Verify result from SDL_GetAudioDeviceName(%i, %i) is not NULL", i, t);
            if (name != NULL) {
              SDLTest_AssertCheck(SDL_strlen(name)>0, "verify result from SDL_GetAudioDeviceName(%i, %i) is not empty, got: '%s'", i, t, name);
              if (t==1) {
                  /* Also try non-zero type */
                  tt = t + SDLTest_RandomIntegerInRange(1,10);
                  nameAgain = SDL_GetAudioDeviceName(i, tt);
                  SDLTest_AssertCheck(nameAgain != NULL, "Verify result from SDL_GetAudioDeviceName(%i, %i) is not NULL", i, tt);
                  if (nameAgain != NULL) {
                    SDLTest_AssertCheck(SDL_strlen(nameAgain)>0, "Verify result from SDL_GetAudioDeviceName(%i, %i) is not empty, got: '%s'", i, tt, nameAgain);
                    SDLTest_AssertCheck(SDL_strcmp(name, nameAgain)==0, 
                      "Verify SDL_GetAudioDeviceName(%i, %i) and SDL_GetAudioDeviceName(%i %i) return the same string", 
                      i, t, i, tt);
                  }
               }
            }
         }
      }
   }
   
   return TEST_COMPLETED;
}

/**
 * \brief Negative tests around enumeration and naming of audio devices.
 * 
 * \sa http://wiki.libsdl.org/moin.cgi/SDL_GetNumAudioDevices
 * \sa http://wiki.libsdl.org/moin.cgi/SDL_GetAudioDeviceName
 */
int audio_enumerateAndNameAudioDevicesNegativeTests()
{
   int t;
   int i, j, no, nc;
   const char *name;
      
   /* Get number of devices. */
   no = SDL_GetNumAudioDevices(0);
   SDLTest_AssertPass("Call to SDL_GetNumAudioDevices(0)");
   nc = SDL_GetNumAudioDevices(1);
   SDLTest_AssertPass("Call to SDL_GetNumAudioDevices(1)");
   
   /* Invalid device index when getting name */
   for (t=0; t<2; t++) {
      /* Negative device index */
      i = SDLTest_RandomIntegerInRange(-10,-1);
      name = SDL_GetAudioDeviceName(i, t);
      SDLTest_AssertPass("Call to SDL_GetAudioDeviceName(%i, %i)", i, t);
      SDLTest_AssertCheck(name == NULL, "Check SDL_GetAudioDeviceName(%i, %i) result NULL, expected NULL, got: %s", i, t, (name == NULL) ? "NULL" : name);
      
      /* Device index past range */
      for (j=0; j<3; j++) {
         i = (t) ? nc+j : no+j;
         name = SDL_GetAudioDeviceName(i, t);
         SDLTest_AssertPass("Call to SDL_GetAudioDeviceName(%i, %i)", i, t);
         SDLTest_AssertCheck(name == NULL, "Check SDL_GetAudioDeviceName(%i, %i) result, expected: NULL, got: %s", i, t, (name == NULL) ? "NULL" : name);
      }
      
      /* Capture index past capture range but within output range */
      if ((no>0) && (no>nc) && (t==1)) {
         i = no-1;
         name = SDL_GetAudioDeviceName(i, t);
         SDLTest_AssertPass("Call to SDL_GetAudioDeviceName(%i, %i)", i, t);
         SDLTest_AssertCheck(name == NULL, "Check SDL_GetAudioDeviceName(%i, %i) result, expected: NULL, got: %s", i, t, (name == NULL) ? "NULL" : name);
      }
   }

   return TEST_COMPLETED;
}


/**
 * @brief Checks available audio driver names.
 */
int audio_printAudioDrivers()
{
   int i, n;
   const char *name;

   /* Get number of drivers */
   n = SDL_GetNumAudioDrivers();
   SDLTest_AssertPass("Call to SDL_GetNumAudioDrivers()");
   SDLTest_AssertCheck(n>=0, "Verify number of audio drivers >= 0, got: %i", n);
   
   /* List drivers. */
   if (n>0)
   {
      for (i=0; i<n; i++) {
         name = SDL_GetAudioDriver(i);
         SDLTest_AssertPass("Call to SDL_GetAudioDriver(%i)", i);
         SDLTest_AssertCheck(name != NULL, "Verify returned name is not NULL");
         if (name != NULL) {
            SDLTest_AssertCheck(SDL_strlen(name)>0, "Verify returned name is not empty, got: '%s'", name);
         }
      }
   }

   return TEST_COMPLETED;
}


/**
 * @brief Checks current audio driver name with initialized audio.
 */
int audio_printCurrentAudioDriver()
{
   const char *name;

   /* Check current audio driver */
   name = SDL_GetCurrentAudioDriver();
   SDLTest_AssertPass("Call to SDL_GetCurrentAudioDriver()");
   SDLTest_AssertCheck(name != NULL, "Verify returned name is not NULL");
   if (name != NULL) {
      SDLTest_AssertCheck(SDL_strlen(name)>0, "Verify returned name is not empty, got: '%s'", name);
   }

   return TEST_COMPLETED;
}

/* ================= Test Case References ================== */

/* Audio test cases */
static const SDLTest_TestCaseReference audioTest1 =
		{ (SDLTest_TestCaseFp)audio_enumerateAndNameAudioDevices, "audio_enumerateAndNameAudioDevices", "Enumerate and name available audio devices (output and capture)", TEST_ENABLED };

static const SDLTest_TestCaseReference audioTest2 =
		{ (SDLTest_TestCaseFp)audio_enumerateAndNameAudioDevicesNegativeTests, "audio_enumerateAndNameAudioDevicesNegativeTests", "Negative tests around enumeration and naming of audio devices.", TEST_ENABLED };

static const SDLTest_TestCaseReference audioTest3 =
		{ (SDLTest_TestCaseFp)audio_printAudioDrivers, "audio_printAudioDrivers", "Checks available audio driver names.", TEST_ENABLED };

static const SDLTest_TestCaseReference audioTest4 =
		{ (SDLTest_TestCaseFp)audio_printCurrentAudioDriver, "audio_printCurrentAudioDriver", "Checks current audio driver name with initialized audio.", TEST_ENABLED };

/* Sequence of Audio test cases */
static const SDLTest_TestCaseReference *audioTests[] =  {
	&audioTest1, &audioTest2, &audioTest3, &audioTest4, NULL
};

/* Audio test suite (global) */
SDLTest_TestSuiteReference audioTestSuite = {
	"Audio",
	_audioSetUp,
	audioTests,
	NULL
};
