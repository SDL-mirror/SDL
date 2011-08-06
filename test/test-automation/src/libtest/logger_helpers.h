#ifndef _LOGGER_HELPERS_G
#define _LOGGER_HELPERS_G

#include <time.h>

char *IntToString(const int integer);

char *IntToHexString(const Uint64 integer);

char *DoubleToString(const double decimal);

char *TimestampToString(const time_t timestamp);

char *ToLowerCase(const char *string);

int ValidateString(const char *string);

#endif
