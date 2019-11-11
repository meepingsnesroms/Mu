#ifndef PXA260_RTC_H
#define PXA260_RTC_H

#include "pxa260_CPU.h"
#include "pxa260_IC.h"


/*
	PXA260 OS RTC controller
	
	PURRPOSE: it's nice to know what time it is

*/

#define PXA260_RTC_BASE		0x40900000UL
#define PXA260_RTC_SIZE		0x00001000UL


typedef struct{

   Pxa260ic* ic;
	
	UInt32 RCNR_offset;	//RTC counter offset from our local time
	UInt32 RTAR;		//RTC alarm
	UInt32 RTSR;		//RTC status
	UInt32 RTTR;		//RTC trim - we ignore this alltogether
	UInt32 lastSeenTime;	//for HZ interrupt
	
}Pxa260rtc;

void pxa260rtcInit(Pxa260rtc* rtc, Pxa260ic* ic);
void pxa260rtcUpdate(Pxa260rtc* rtc);


#endif

