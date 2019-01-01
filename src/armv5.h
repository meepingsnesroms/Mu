#ifndef ARMV5_H
#define ARMV5_H

#include <stdint.h>
#include <stdbool.h>

void armv5Init(void);
void armv5Reset(void);
uint64_t armv5StateSize(void);
void armv5SaveState(uint8_t* data);
void armv5LoadState(uint8_t* data);

void armv5SetStackLocation(uint32_t stackLocation);
void armv5PceNativeCall(uint32_t functionPtr, uint32_t userdataPtr);
uint32_t armv5Execute(uint32_t cycles);//returns cycles left

uint32_t armv5GetRegister(uint8_t reg);//only for debugging
uint32_t armv5GetPc(void);//only for debugging
uint64_t armv5ReadArbitraryMemory(uint32_t address, uint8_t size);//only for debugging

#endif
