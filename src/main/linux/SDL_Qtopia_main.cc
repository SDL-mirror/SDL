
/* Include the SDL main definition header */
#include "SDL_main.h"
#ifdef main
#undef main
#endif
#ifdef QWS
#include <qpe/qpeapplication.h>
#endif

extern int SDL_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
#ifdef QWS
  // This initializes the Qtopia application. It needs to be done here
  // because it parses command line options.
  QPEApplication app(argc, argv);
  QWidget dummy;
  app.showMainWidget(&dummy);
#endif
  // Exit here because if return is used, the application
  // doesn't seem to quit correctly.
  exit(SDL_main(argc, argv));
}
