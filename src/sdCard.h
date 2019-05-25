#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>
#include <stdint.h>

void sdCardReset(void);

void sdCardSetChipSelect(bool value);
bool sdCardExchangeBit(bool bit);
uint32_t sdCardExchangeXBitsOptimized(uint32_t bits, uint8_t size);

#endif
