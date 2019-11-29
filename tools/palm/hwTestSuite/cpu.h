#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "testSuite.h"

#define CPU_NONE  0x00
#define CPU_M68K  0x01
#define CPU_ARM   0x02
#define CPU_BOTH  (CPU_M68K | CPU_ARM)
#define CPU_TYPES 0x0F

#define CPU_M68K_328   0x10
#define CPU_M68K_EZ    0x20
#define CPU_M68K_VZ    0x30
#define CPU_M68K_SZ    0x40
#define CPU_M68K_TYPES 0xF0

void turnInterruptsOff(void);
void turnInterruptsOn(void);
void wasteXOpcodes(uint32_t opcodes);
uint8_t getPhysicalCpuType(void);
uint8_t getSupportedInstructionSets(void);
const char* getCpuString(void);
var enterUnsafeMode(void);
var exitUnsafeMode(void);

#endif
