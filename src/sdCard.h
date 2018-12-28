#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>

void sdCardReset(void);

void sdCardSetChipSelect(bool value);
bool sdCardExchangeBit(bool bit);

#endif
