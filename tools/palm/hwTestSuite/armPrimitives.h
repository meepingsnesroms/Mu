#ifndef ARM_PRIMITIVES_H
#define ARM_PRIMITIVES_H

#include <stdint.h>

uint8_t armRead8(uint32_t address);
uint16_t armRead16(uint32_t address);
uint32_t armRead32(uint32_t address);
void armWrite8(uint32_t address, uint8_t value);
void armWrite16(uint32_t address, uint16_t value);
void armWrite32(uint32_t address, uint32_t value);
void callArmTests(uint32_t* data);/*must call with 32 bit aligned data*/

#endif
