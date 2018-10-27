#pragma once

#include <stdint.h>
#include <stdbool.h>

void flx68000Init();
void flx68000Reset();
uint64_t flx68000StateSize();
void flx68000SaveState(uint8_t* data);
void flx68000LoadState(uint8_t* data);

void flx68000Execute();//runs the CPU for 1 CLK32 pulse
void flx68000SetIrq(uint8_t irqLevel);
void flx68000RefreshAddressing();
bool flx68000IsSupervisor();
void flx68000BusError(uint32_t address, bool isWrite);

uint32_t flx68000GetRegister(uint8_t reg);//only for debugging
uint32_t flx68000GetPc();//only for debugging
uint64_t flx68000ReadArbitraryMemory(uint32_t address, uint8_t size);//only for debugging
