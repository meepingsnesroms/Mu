#ifndef PXA260_I2C_H
#define PXA260_I2C_H

#include <stdint.h>

#define PXA260_I2C_BASE	0x40300000
#define PXA260_I2C_SIZE	0x00010000

extern uint8_t pxa260I2cBus;

uint32_t pxa260I2cReadWord(uint32_t address);
void pxa260I2cWriteWord(uint32_t address, uint32_t value);

#endif
