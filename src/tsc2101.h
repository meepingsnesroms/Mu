#ifndef TSC2101_H
#define TSC2101_H

#include <stdint.h>
#include <stdbool.h>

extern uint16_t tsc2101CurrentWord;
extern uint8_t  tsc2101CurrentWordBitsRemaining;
extern uint8_t  tsc2101CurrentPage;
extern uint8_t  tsc2101CurrentRegister;
extern bool     tsc2101CommandFinished;
extern bool     tsc2101Read;

void tsc2101Reset(void);
uint32_t tsc2101StateSize(void);
void tsc2101SaveState(uint8_t* data);
void tsc2101LoadState(uint8_t* data);

void tsc2101SetChipSelect(bool value);
bool tsc2101ExchangeBit(bool bit);

#endif
