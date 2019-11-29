#ifndef W86L488_H
#define W86L488_H

#include <stdint.h>

void w86l488Reset(void);
uint32_t w86l488StateSize(void);
void w86l488SaveState(uint8_t* data);
void w86l488LoadState(uint8_t* data);

uint16_t w86l488Read16(uint8_t address);
void w86l488Write16(uint8_t address, uint16_t value);

#endif
