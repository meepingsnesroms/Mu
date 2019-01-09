#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

enum{
   ARM_STACK_START = 0,
   ARM_EXIT_FUNC,
   ARM_CALL_68K_FUNC,
   ARM_EMUL_STATE
};

uint32_t getGlobalVar(uint16_t id);
void setGlobalVar(uint16_t id, uint32_t value);

#endif
