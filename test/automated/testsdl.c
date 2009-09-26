/*
 * SDL test suite framework code.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#include "SDL.h"
#include "SDL_at.h"

#include "platform/platform.h"
#include "rwops/rwops.h"
#include "surface/surface.h"
#include "render/render.h"
#include "audio/audio.h"

#include <stdio.h> /* printf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* getopt */
#include <getopt.h> /* getopt_long */
#include <string.h> /* strcmp */


/*
 * Tests to run.
 */
static int run_manual      = 0; /**< Run manual tests. */
/* Manual. */
/* Automatic. */
static int run_platform    = 1; /**< Run platform tests. */
static int run_rwops       = 1; /**< Run RWops tests. */
static int run_surface     = 1; /**< Run surface tests. */
static int run_render      = 1; /**< Run render tests. */
static int run_audio       = 1; /**< Run audio tests. */

/*
 * Prototypes.
 */
static void print_usage( const char *name );
static void parse_options( int argc, char *argv[] );


/**
 * @brief Displays program usage.
 */
static void print_usage( const char *name )
{
   printf("Usage: %s [OPTIONS]\n", name);
   printf("Options are:\n");
   printf("   -m, --manual    enables tests that require user interaction\n");
   printf("   --noplatform    do not run the platform tests\n");
   printf("   --norwops       do not run the rwops tests\n");
   printf("   --nosurface     do not run the surface tests\n");
   printf("   --norender      do not run the render tests\n");
   printf("   --noaudio       do not run the audio tests\n");
   printf("   -v, --verbose   increases verbosity level by 1 for each -v\n");
   printf("   -q, --quiet     only displays errors\n");
   printf("   -h, --help      display this message and exit\n");
}


/**
 * @brief Handles the options.
 */
static void parse_options( int argc, char *argv[] )
{
   static struct option long_options[] = {
      { "manual", no_argument, 0, 'm' },
      { "noplatform", no_argument, 0, 0 },
      { "norwops", no_argument, 0, 0 },
      { "nosurface", no_argument, 0, 0 },
      { "norender", no_argument, 0, 0 },
      { "noaudio", no_argument, 0, 0 },
      { "verbose", no_argument, 0, 'v' },
      { "quiet", no_argument, 0, 'q' },
      { "help", no_argument, 0, 'h' },
      {NULL,0,0,0}
   };
   int option_index = 0;
   int c = 0;
   int i;
   const char *str;

   /* Iterate over options. */
   while ((c = getopt_long( argc, argv,
               "vqh",
               long_options, &option_index)) != -1) {

      /* Handle options. */
      switch (c) {
         case 0:
            str = long_options[option_index].name;
            if (strcmp(str,"noplatform")==0)
               run_platform = 0;
            else if (strcmp(str,"norwops")==0)
               run_rwops = 0;
            else if (strcmp(str,"nosurface")==0)
               run_surface = 0;
            else if (strcmp(str,"norender")==0)
               run_render = 0;
            else if (strcmp(str,"noaudio")==0)
               run_audio = 0;
            break;

         /* Manual. */
         case 'm':
            run_manual = 1;
            break;

         /* Verbosity. */
         case 'v':
            SDL_ATgeti( SDL_AT_VERBOSE, &i );
            SDL_ATseti( SDL_AT_VERBOSE, i+1 );
            break;

         /* Quiet. */
         case 'q':
            SDL_ATseti( SDL_AT_QUIET, 1 );
            break;

         /* Help. */
         case 'h':
            print_usage( argv[0] );
            exit(EXIT_SUCCESS);
      }
   }

}


/**
 * @brief Main entry point.
 */
int main( int argc, char *argv[] )
{
   int failed;
   int rev;
   SDL_version ver;

   /* Get options. */
   parse_options( argc, argv );

   /* Defaults. */
   failed = 0;

   /* Print some text if verbose. */
   SDL_GetVersion( &ver );
   rev = SDL_GetRevision();
   SDL_ATprintVerbose( 1, "Running tests with SDL %d.%d.%d revision %d\n",
         ver.major, ver.minor, ver.patch, rev );

   /* Automatic tests. */
   if (run_platform)
      failed += test_platform();
   if (run_rwops)
      failed += test_rwops();
   if (run_surface)
      failed += test_surface();
   if (run_render)
      failed += test_render();
   if (run_audio)
      failed += test_audio();

   /* Manual tests. */
   if (run_manual) {
   }

   /* Display more information if failed. */
   if (failed > 0) {
      SDL_ATprintErr( "Tests run with SDL %d.%d.%d revision %d\n",
            ver.major, ver.minor, ver.patch, rev );
      SDL_ATprintErr( "System is running %s and is %s endian\n",
            SDL_GetPlatform(),
#ifdef SDL_LIL_ENDIAN
            "little"
#else
            "big"
#endif
            );
   }

   return failed;
}

