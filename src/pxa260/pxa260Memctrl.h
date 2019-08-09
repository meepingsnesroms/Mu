#ifndef PXA260_MEMCTRL_H
#define PXA260_MEMCTRL_H

#include <stdint.h>

#define PXA260_MEMCTRL_BASE	0x48000000
#define PXA260_MEMCTRL_SIZE	0x00010000

extern uint32_t pxa260MemctrlRegisters[];

void pxa260MemctrlReset(void);

uint32_t pxa260MemctrlReadWord(uint32_t address);
void pxa260MemctrlWriteWord(uint32_t address, uint32_t value);

#endif
