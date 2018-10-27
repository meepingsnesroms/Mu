#pragma once

#include <stdint.h>
#include <stdbool.h>

void m68328Init();
void m68328Reset();
uint64_t m68328StateSize();
void m68328SaveState(uint8_t* data);
void m68328LoadState(uint8_t* data);

void m68328Execute();//runs the CPU for 1 CLK32 pulse
void m68328SetIrq(uint8_t irqLevel);

bool m68328IsSupervisor();
uint32_t m68328GetRegister(uint8_t reg);
uint32_t m68328GetPc();
void m68328BusError(uint32_t address, bool isWrite);
