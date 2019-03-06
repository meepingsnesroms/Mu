#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

enum{
   ARM_STACK_START = 0,
   ARM_EXIT_FUNC,
   ARM_CALL_68K_FUNC,
   M68K_CALL_WITH_BLOB_FUNC,
   RESOURCE_GLOBALS_INITIALIZED,
   TUNGSTEN_W_DRIVERS_INSTALLED,
   CURRENT_RESOLUTION,
   ORIGINAL_FRAMEBUFFER
};

uint32_t getGlobalVar(uint16_t id);
void setGlobalVar(uint16_t id, uint32_t value);

#endif
