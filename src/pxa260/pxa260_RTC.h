#ifndef _PXA260_RTC_H_
#define _PXA260_RTC_H_

#include "pxa260_mem.h"
#include "pxa260_CPU.h"
#include "pxa260_IC.h"


/*
	PXA260 OS RTC controller
	
	PURRPOSE: it's nice to know what time it is

*/

#define PXA260_RTC_BASE		0x40900000UL
#define PXA260_RTC_SIZE		0x00001000UL


typedef struct{

	Pxa255ic* ic;
	
	UInt32 RCNR_offset;	//RTC counter offset from our local time
	UInt32 RTAR;		//RTC alarm
	UInt32 RTSR;		//RTC status
	UInt32 RTTR;		//RTC trim - we ignore this alltogether
	UInt32 lastSeenTime;	//for HZ interrupt
	
}Pxa255rtc;

Boolean pxa260rtcInit(Pxa255rtc* rtc, ArmMem* physMem, Pxa255ic* ic);
void pxa260rtcUpdate(Pxa255rtc* rtc);


#endif

