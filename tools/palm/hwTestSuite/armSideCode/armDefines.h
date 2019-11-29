#ifndef ARM_DEFINES_H
#define ARM_DEFINES_H

#include <stdint.h>

#define ALIGN(size) __attribute__((aligned(size)))
#define USED __attribute__((used))
#define ENTRYPOINT(func) __attribute__((naked,section(".vectors"))) void vecs(void){asm volatile("B "func"\n\t");}

#define PXA260_VIRT_ADDR(addr) (0x50000000 + (addr))
#define PXA260_REG(base, register) (*(volatile uint32_t*)PXA260_VIRT_ADDR((base) | (register)))

//...
#define PXA260_I2C_BASE	0x40300000
//...
#define PXA260_GPIO_BASE 0x40E00000
//...
#define PXA260_SSP_BASE	0x41000000
//...

//I2C registers
#define IBMR 0x1680
#define IDBR 0x1688
#define ICR 0x1690
#define ISR 0x1698
#define ISAR 0x16A0

//GPIO registers
#define GPLR0 0x0000
#define GPLR1 0x0004
#define GPLR2 0x0008
#define GPDR0 0x000C
#define GPDR1 0x0010
#define GPDR2 0x0014
#define GPSR0 0x0018
#define GPSR1 0x001C
#define GPSR2 0x0020
#define GPCR0 0x0024
#define GPCR1 0x0028
#define GPCR2 0x002C
#define GRER0 0x0030
#define GRER1 0x0034
#define GRER2 0x0038
#define GFER0 0x003C
#define GFER1 0x0040
#define GFER2 0x0044
#define GEDR0 0x0048
#define GEDR1 0x004C
#define GEDR2 0x0050
#define GAFR0_L 0x0054
#define GAFR0_U 0x0058
#define GAFR1_L 0x005C
#define GAFR1_U 0x0060
#define GAFR2_L 0x0064
#define GAFR2_U 0x0068

//SSP registers
#define SSCR0 0x0000
#define SSCR1 0x0004
#define SSSR 0x0008
#define SSDR 0x0010

#endif
