#pragma once

#include <stdint.h>
#include <stdbool.h>

extern uint8_t  ads7846BitsToNextControl;
extern uint8_t  ads7846ControlByte;
extern bool     ads7846PenIrqEnabled;
extern uint16_t ads7846OutputValue;

bool ads7846ExchangeBit(bool bitIn);
bool ads7846Busy();

void ads7846Reset();
