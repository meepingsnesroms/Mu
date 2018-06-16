#pragma once

#include <stdint.h>
#include <stdbool.h>

extern bool lowPowerStopActive;

void patchTo68328();
void triggerBusError(uint32_t address, bool isWrite);
