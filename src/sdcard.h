#pragma once

#include <stdint.h>

#include "emulator.h"

void sdCardInit();
void sdCardExit();
uint32_t sdCardReconfigure(uint64_t size);
buffer_t sdCardGetImage();
uint32_t sdCardSetFromImage(buffer_t image);
void sdCardSaveState(uint64_t sessionId, uint64_t stateId);
void sdCardLoadState(uint64_t sessionId, uint64_t stateId);
