#ifndef ARMV5_H
#define ARMV5_H

#include <stdint.h>

void armv5SetStack(uint8_t* location, uint32_t size);
void armv5StackPush(uint32_t value);
uint32_t armv5StackPop(void);
void armv5SetRegister(uint8_t reg, uint32_t value);
uint32_t armv5GetRegister(uint8_t reg);

#endif
