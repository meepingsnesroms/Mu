#ifndef ADS7846_H
#define ADS7846_H

#include <stdint.h>
#include <stdbool.h>

extern bool ads7846PenIrqEnabled;

void ads7846Reset(void);
uint32_t ads7846StateSize(void);
void ads7846SaveState(uint8_t* data);
void ads7846LoadState(uint8_t* data);

void ads7846SetChipSelect(bool value);
bool ads7846ExchangeBit(bool bitIn);

#endif
