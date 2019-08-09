#ifndef _PXA260_GPIO_H_
#define _PXA260_GPIO_H_

#include "pxa260_mem.h"
#include "pxa260_CPU.h"
#include "pxa260_IC.h"

/*
	PXA260 OS GPIO controller

*/

#define PXA260_GPIO_BASE		0x40E00000UL
#define PXA260_GPIO_SIZE		0x00001000UL


typedef struct{

   Pxa260ic* ic;
	
	UInt32 latches[3];		//what pxa wants to be outputting
   UInt32 inputs[3];		   //what pxa is receiving	[only set by the pxa260gpioSetState() API]
   UInt32 levels[3];		   //what pxa sees (it differs from above for IN pins)
	UInt32 dirs[3];			//1 = output
	UInt32 riseDet[3];		//1 = rise detect
	UInt32 fallDet[3];		//1 = fall detect
	UInt32 detStatus[3];		//1 = detect happened
	UInt32 AFRs[6];			//1, 2, 3 = alt funcs
	
}Pxa260gpio;

#define PXA260_GPIO_LOW			0		//these values make it look like all HiZ, AFR, and nonexistent pins have pullups to those who dumbly assume gpioGEt returns a boolean
#define PXA260_GPIO_HIGH		1
#define PXA260_GPIO_HiZ			2
#define PXA260_GPIO_AFR1		3
#define PXA260_GPIO_AFR2		4
#define PXA260_GPIO_AFR3		5
#define PXA260_GPIO_NOT_PRESENT		6

Boolean pxa260gpioPrvMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf);
void pxa260gpioInit(Pxa260gpio* gpio, Pxa260ic* ic);

//for external use :)
UInt8 pxa260gpioGetState(Pxa260gpio* gpio, UInt8 gpioNum);
void pxa260gpioSetState(Pxa260gpio* gpio, UInt8 gpioNum, Boolean on);	//we can only set value (and only of input pins), not direction

#endif

