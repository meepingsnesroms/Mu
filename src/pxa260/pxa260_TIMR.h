#ifndef _PXA260_TIMR_H_
#define _PXA260_TIMR_H_

#include "pxa260_mem.h"
#include "pxa260_CPU.h"
#include "pxa260_IC.h"


/*
	PXA260 OS timers controller
	
	PURRPOSE: timers are useful for stuff :)

*/

#define PXA260_TIMR_BASE	0x40A00000UL
#define PXA260_TIMR_SIZE	0x00010000UL


typedef struct{

	Pxa255ic* ic;
	
	UInt32 OSMR[4];	//Match Register 0-3
	UInt32 OIER;	//Interrupt Enable
	UInt32 OWER;	//Watchdog enable
	UInt32 OSCR;	//Counter Register
	UInt32 OSSR;	//Status Register
	
}Pxa255timr;

Boolean pxa260timrPrvMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf);
void pxa260timrInit(Pxa255timr* timr, Pxa255ic* ic);
void pxa260timrTick(Pxa255timr* timr);


#endif

