#ifndef _PXA255_TIMR_H_
#define _PXA255_TIMR_H_

#include "pxa255_mem.h"
#include "pxa255_CPU.h"
#include "pxa255_IC.h"


/*
	PXA255 OS timers controller
	
	PURRPOSE: timers are useful for stuff :)

*/

#define PXA255_TIMR_BASE	0x40A00000UL
#define PXA255_TIMR_SIZE	0x00010000UL


typedef struct{

	Pxa255ic* ic;
	
	UInt32 OSMR[4];	//Match Register 0-3
	UInt32 OIER;	//Interrupt Enable
	UInt32 OWER;	//Watchdog enable
	UInt32 OSCR;	//Counter Register
	UInt32 OSSR;	//Status Register
	
}Pxa255timr;

void pxa255timrInit(Pxa255timr* timr, Pxa255ic* ic);
void pxa255timrTick(Pxa255timr* timr);


#endif

