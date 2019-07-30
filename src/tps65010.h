#ifndef TPS65010_H
#define TPS65010_H

#include <stdint.h>
#include <stdbool.h>

void tps65010Reset(void);
uint32_t tps65010StateSize(void);
void tps65010SaveState(uint8_t* data);
void tps65010LoadState(uint8_t* data);

void tps65010SetChipSelect(bool value);
bool tps65010ExchangeBit(bool bit);

#endif
