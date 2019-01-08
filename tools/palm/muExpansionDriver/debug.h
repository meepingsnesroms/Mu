#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>

#ifdef DEBUG
void debugLog(const char* format, ...);
#else
#define debugLog(...)
#endif

#endif
