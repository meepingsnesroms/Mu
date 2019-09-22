#ifndef ARM_DEFINES_H
#define ARM_DEFINES_H

#include <stdint.h>

#define ALIGN(size) __attribute__((aligned(size)))

#define PXA260_VIRT_ADDR(addr) (0x50000000 + (addr))
#define PXA260_REG(base, register) (*(volatile uint32_t*)PXA260_VIRT_ADDR((base) | (register)))

//...
#define PXA260_I2C_BASE	0x40300000
//...
#define PXA260_SSP_BASE	0x41000000
//...

//I2C registers
#define IBMR 0x1680
#define IDBR 0x1688
#define ICR 0x1690
#define ISR 0x1698
#define ISAR 0x16A0

//SSP registers
#define SSCR0 0x0000
#define SSCR1 0x0004
#define SSSR 0x0008
#define SSDR 0x0010

#endif
