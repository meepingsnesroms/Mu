#pragma once

#include <stdint.h>
#include <stdbool.h>

extern bool ads7846PenIrqEnabled;

void ads7846Reset();
uint64_t ads7846StateSize();
void ads7846SaveState(uint8_t* data);
void ads7846LoadState(uint8_t* data);

void ads7846SetChipSelect(bool value);
bool ads7846ExchangeBit(bool bitIn);
bool ads7846Busy();
