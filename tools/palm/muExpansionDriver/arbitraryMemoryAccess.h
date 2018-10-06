#ifndef ARBITRARY_MEMORY_ACCESS_HEADER
#define ARBITRARY_MEMORY_ACCESS_HEADER

#include <stdint.h>

#define readArbitraryMemory8(address) (*((volatile uint8_t*)(address)))
#define readArbitraryMemory16(address) (*((volatile uint16_t*)(address)))
#define readArbitraryMemory32(address) (*((volatile uint32_t*)(address)))
#define writeArbitraryMemory8(address, value) (*((volatile uint8_t*)(address)) = (value))
#define writeArbitraryMemory16(address, value) (*((volatile uint16_t*)(address)) = (value))
#define writeArbitraryMemory32(address, value) (*((volatile uint32_t*)(address)) = (value))

#endif
