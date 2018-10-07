#ifndef PALM_GLOBAL_DEFINES_HEADER
#define PALM_GLOBAL_DEFINES_HEADER

#include <PalmOS.h>
#include <stdint.h>

#define CODE_SECTION(codeSection) __attribute__((section(codeSection)))
#define ALIGN(size) __attribute__((aligned(size)))

#define PalmOS35 sysMakeROMVersion(3, 5, 0, sysROMStageRelease, 0)
#define PalmOS50 sysMakeROMVersion(5, 0, 0, sysROMStageRelease, 0)

#define readArbitraryMemory8(address) (*((volatile uint8_t*)(address)))
#define readArbitraryMemory16(address) (*((volatile uint16_t*)(address)))
#define readArbitraryMemory32(address) (*((volatile uint32_t*)(address)))
#define writeArbitraryMemory8(address, value) (*((volatile uint8_t*)(address)) = (value))
#define writeArbitraryMemory16(address, value) (*((volatile uint16_t*)(address)) = (value))
#define writeArbitraryMemory32(address, value) (*((volatile uint32_t*)(address)) = (value))

#endif
