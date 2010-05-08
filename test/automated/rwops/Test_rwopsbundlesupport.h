#ifdef __APPLE__

#include <stdio.h>

#ifndef SDL_rwopsbundlesupport_h
#define SDL_rwopsbundlesupport_h
FILE* Test_OpenFPFromBundleOrFallback(const char *file, const char *mode);
FILE* Test_OpenFPFromTemporaryDir(const char *file, const char *mode);

#endif
#endif
