#ifndef PXA260_IC_H
#define PXA260_IC_H

#include "pxa260_CPU.h"
#include <stdio.h> 

/*
	PXA260 interrupt controller
	
	PURRPOSE: raises IRQ, FIQ as needed

*/

#define PXA260_IC_BASE	0x40D00000UL
#define PXA260_IC_SIZE	0x00010000UL


#define PXA260_I_RTC_ALM	31
#define PXA260_I_RTC_HZ		30
#define PXA260_I_TIMR3		29
#define PXA260_I_TIMR2		28
#define PXA260_I_TIMR1		27
#define PXA260_I_TIMR0		26
#define PXA260_I_DMA		25
#define PXA260_I_SSP		24
#define PXA260_I_MMC		23
#define PXA260_I_FFUART		22
#define PXA260_I_BTUART		21
#define PXA260_I_STUART		20
#define PXA260_I_ICP		19
#define PXA260_I_I2C		18
#define PXA260_I_LCD		17
#define PXA260_I_NET_SSP	16
#define PXA260_I_AC97		14
#define PXA260_I_I2S		13
#define PXA260_I_PMU		12
#define PXA260_I_USB		11
#define PXA260_I_GPIO_all	10
#define PXA260_I_GPIO_1		9
#define PXA260_I_GPIO_0		8
#define PXA260_I_HWUART		7


typedef struct{

	UInt32 ICMR;	//Mask Register
	UInt32 ICLR;	//Level Register
	UInt32 ICCR;	//Control Register
	UInt32 ICPR;	//Pending register
	
	Boolean wasIrq, wasFiq;
	
}Pxa260ic;

Boolean pxa260icPrvMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf);
void pxa260icInit(Pxa260ic* ic);
void pxa260icInt(Pxa260ic* ic, UInt8 intNum, Boolean raise);		//interrupt caused by emulated hardware/ interrupt handled by guest


#endif

