#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

enum{
   EMPTY_LIST = 0
};

uint32_t getGlobalVar(uint16_t id);
void setGlobalVar(uint16_t id, uint32_t value);

#endif
