#ifndef TPS65010_H
#define TPS65010_H

#include <stdint.h>

void tps65010Reset(void);
uint32_t tps65010StateSize(void);
void tps65010SaveState(uint8_t* data);
void tps65010LoadState(uint8_t* data);

uint8_t tps65010I2cExchange(uint8_t i2cBus);

#endif
