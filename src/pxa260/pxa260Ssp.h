#ifndef PXA260_SSP_H
#define PXA260_SSP_H

#include <stdint.h>
#include <stdbool.h>

#define PXA260_SSP_BASE	0x41000000
#define PXA260_SSP_SIZE	0x00010000

extern uint32_t pxa260SspSscr0;
extern uint32_t pxa260SspSscr1;
extern uint16_t pxa260SspRxFifo[];
extern uint16_t pxa260SspTxFifo[];
extern uint8_t  pxa260SspRxReadPosition;
extern uint8_t  pxa260SspRxWritePosition;
extern bool     pxa260SspRxOverflowed;
extern uint8_t  pxa260SspTxReadPosition;
extern uint8_t  pxa260SspTxWritePosition;
extern bool     pxa260SspTransfering;

void pxa260SspReset(void);

uint32_t pxa260SspReadWord(uint32_t address);
void pxa260SspWriteWord(uint32_t address, uint32_t value);

void pxa260SspTransferComplete(void);

#endif
