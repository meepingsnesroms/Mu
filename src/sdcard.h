#pragma once

#include <stdint.h>

void sdCardInit();
void sdCardExit();
uint32_t sdCardReconfigure(uint64_t size);
uint32_t sdCardSetFromImage(uint8_t* data, uint64_t size);
void sdCardSaveState(uint64_t sessionId, uint64_t stateId);
void sdCardLoadState(uint64_t sessionId, uint64_t stateId);
