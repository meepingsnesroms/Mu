#pragma once

#include <stdbool.h>

extern uint8_t  ads7846BitsToNextControl;
extern uint8_t  ads7846ControlByte;
extern bool     ads7846PenIrqEnabled;
extern uint16_t ads7846OutputValue;

void ads7846SendBit(bool bit);
bool ads7846RecieveBit();

void ads7846Reset();
