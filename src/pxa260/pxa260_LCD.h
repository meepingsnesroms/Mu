#ifndef _PXA260_LCD_H_
#define _PXA260_LCD_H_

#include "pxa260_CPU.h"
#include "pxa260_IC.h"

uint16_t* pxa260Framebuffer;

/*
	PXA260 OS LCD controller
	
	PURRPOSE: it's nice to have a framebuffer

*/

#define PXA260_LCD_BASE		0x44000000UL
#define PXA260_LCD_SIZE		0x00001000UL




#define LCD_STATE_IDLE		0
#define LCD_STATE_DMA_0_START	1
#define LCD_STATE_DMA_0_END	2

typedef struct{

   Pxa260ic* ic;
	
	//registers
	UInt32 lccr0, lccr1, lccr2, lccr3, fbr0, fbr1, liicr, trgbr, tcr;
	UInt32 fdadr0, fsadr0, fidr0, ldcmd0;
	UInt32 fdadr1, fsadr1, fidr1, ldcmd1;
	UInt16 lcsr;	//yes, 16-bit :)
	
	//for our use
	UInt16 intMask;
	
	UInt8 state		: 6;
	UInt8 intWasPending	: 1;
	UInt8 enbChanged	: 1;
	
	UInt8 palette[512];

	UInt32 frameNum;
	
}Pxa260lcd;

Boolean pxa260lcdPrvMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf);
void pxa260lcdInit(Pxa260lcd* lcd, Pxa260ic* ic);
void pxa260lcdFrame(Pxa260lcd* lcd);

#endif

