#ifndef PALM_GLOBAL_DEFINES_H
#define PALM_GLOBAL_DEFINES_H

#include <stdint.h>

#define CODE_SECTION(codeSection) __attribute__((section(codeSection)))
#define NO_RETURN __attribute__((noreturn))
#define ALIGN(size) __attribute__((aligned(size)))
#define PACKED __attribute__((packed))
#define FIXED_ADDRESS_VAR(a, t) (*((t volatile *)a))

#define readArbitraryMemory8(address) (*((volatile uint8_t*)(address)))
#define readArbitraryMemory16(address) (*((volatile uint16_t*)(address)))
#define readArbitraryMemory32(address) (*((volatile uint32_t*)(address)))
#define writeArbitraryMemory8(address, value) (*((volatile uint8_t*)(address)) = (value))
#define writeArbitraryMemory16(address, value) (*((volatile uint16_t*)(address)) = (value))
#define writeArbitraryMemory32(address, value) (*((volatile uint32_t*)(address)) = (value))

#undef min
#undef max
#undef clamp
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define clamp(x, y, z) max(x, min(y, z))

#define stopExecution() while(1)

#endif
