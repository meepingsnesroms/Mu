#pragma once

#include <stdint.h>
#include <stdbool.h>

extern bool m68328LowPowerStop;

void m68328Init();
void m68328Reset();
uint64_t m68328StateSize();
void m68328SaveState(uint8_t* data);
void m68328LoadState(uint8_t* data);

void m68328BusError(uint32_t address, bool isWrite);
