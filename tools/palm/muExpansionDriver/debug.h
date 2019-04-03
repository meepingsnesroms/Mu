#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include <stdarg.h>

enum{
   MEMORY_WATCH_NONE = 0,
   MEMORY_WATCH_CODE,
   MEMORY_WATCH_DATA
};

#ifdef DEBUG
void debugLog(const char* format, ...);
#else
#define debugLog(...)
#endif

uint16_t setWatchRegion(uint32_t address, uint32_t size, uint8_t type);
void clearWatchRegion(uint16_t index);

#endif
