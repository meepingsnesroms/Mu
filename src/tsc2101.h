#ifndef TSC2101_H
#define TSC2101_H

#include <stdint.h>
#include <stdbool.h>

void tsc2101Reset(bool isBoot);
uint32_t tsc2101StateSize(void);
void tsc2101SaveState(uint8_t* data);
void tsc2101LoadState(uint8_t* data);

void tsc2101SetPwrDn(bool value);
void tsc2101SetChipSelect(bool value);
bool tsc2101ExchangeBit(bool bit);
void tsc2101UpdateInterrupt(void);//called after touchscreen update to flush the values
//TODO: this chip seems to do audio output, need to add a 16bit optimized transfer function

void tsc2101Scan(void);

#endif
