#ifndef PXA260_SSP_H
#define PXA260_SSP_H

#include <stdint.h>

#define PXA260_SSP_BASE	0x41000000
#define PXA260_SSP_SIZE	0x00010000

extern uint32_t pxa260SspSscr0;
extern uint32_t pxa260SspSscr1;
extern uint32_t pxa260SspSssr;
extern uint32_t pxa260SspSsdr;

void pxa260SspReset(void);

uint32_t pxa260SspReadWord(uint32_t address);
void pxa260SspWriteWord(uint32_t address, uint32_t value);

#endif
