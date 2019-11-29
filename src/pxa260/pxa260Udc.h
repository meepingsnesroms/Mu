#ifndef PXA260_UDC_H
#define PXA260_UDC_H

#include <stdint.h>

#define PXA260_UDC_BASE	0x40600000
#define PXA260_UDC_SIZE	0x00010000

extern uint8_t pxa260UdcUdccr;
extern uint8_t pxa260UdcUicr0;

void pxa260UdcReset(void);

uint32_t pxa260UdcReadWord(uint32_t address);
void pxa260UdcWriteWord(uint32_t address, uint32_t value);

void pxa260UdcDeviceResumeComplete(void);

#endif
