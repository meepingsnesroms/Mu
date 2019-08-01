#ifndef PXA260_I2C_H
#define PXA260_I2C_H

#include <stdint.h>

#define PXA260_I2C_BASE	0x40300000
#define PXA260_I2C_SIZE	0x00010000

enum{
   I2C_0 = 0x00,
   I2C_1,
   I2C_START,
   I2C_STOP,
   I2C_FLOATING_BUS = 0x07
};

enum{
   I2C_WAIT_FOR_ADDR = 0,
   I2C_NOT_SELECTED,
   I2C_RECEIVING,
   I2C_SENDING
};

extern uint8_t  pxa260I2cBus;
extern uint8_t  pxa260I2cBuffer;
extern uint16_t pxa260I2cIcr;
extern uint16_t pxa260I2cIsr;
extern uint16_t pxa260I2cIsar;

void pxa260I2cReset(void);

uint32_t pxa260I2cReadWord(uint32_t address);
void pxa260I2cWriteWord(uint32_t address, uint32_t value);

void pxa260I2cTransmitEmpty(void);

#endif
