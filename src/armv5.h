#ifndef ARMV5_H
#define ARMV5_H

#include <stdint.h>
#include <stdbool.h>

extern bool armv5ServiceRequest;

void armv5Reset(void);
uint32_t armv5StateSize(void);
void armv5SaveState(uint8_t* data);
void armv5LoadState(uint8_t* data);

uint32_t armv5GetRegister(uint8_t reg);
void armv5SetRegister(uint8_t reg, uint32_t value);
int32_t armv5Execute(int32_t cycles);//returns cycles left

uint32_t armv5GetPc(void);//only for debugging

#endif
