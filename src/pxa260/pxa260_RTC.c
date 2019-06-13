#include "pxa260_RTC.h"
#include "pxa260_mem.h"

#include <sys/time.h>



static UInt32 rtcCurTime(void){

   struct timeval tv;

   gettimeofday(&tv, NULL);

   return tv.tv_sec;
}

void pxa260rtcPrvUpdate(Pxa255rtc* rtc){
	
	UInt32 time = rtcCurTime();
	
	if(rtc->lastSeenTime != time){	//do not triger alarm more than once per second please
		
		if((rtc->RTSR & 0x4) && (time + rtc->RCNR_offset == rtc->RTAR)){	//check alarm
			rtc->RTSR |= 1;
		}
		if(rtc->RTSR & 0x8){							//send HZ interrupt
			rtc->RTSR |= 2;
		}
	}
	pxa260icInt(rtc->ic, PXA260_I_RTC_ALM, (rtc->RTSR & 1) != 0);
	pxa260icInt(rtc->ic, PXA260_I_RTC_HZ, (rtc->RTSR & 2) != 0);
}

static Boolean pxa260rtcPrvMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf){

	Pxa255rtc* rtc = userData;
	UInt32 val = 0;
	
	if(size != 4) {
		err_str(__FILE__ ": Unexpected ");
	//	err_str(write ? "write" : "read");
	//	err_str(" of ");
	//	err_dec(size);
	//	err_str(" bytes to 0x");
	//	err_hex(pa);
	//	err_str("\r\n");
		return true;		//we do not support non-word accesses
	}
	
	pa = (pa - PXA260_RTC_BASE) >> 2;
	
	if(write){
		val = *(UInt32*)buf;
		
		switch(pa){
			case 0:
				rtc->RCNR_offset = rtcCurTime() - val;
				break;
			
			case 1:
				rtc->RTAR = val;
				pxa260rtcPrvUpdate(rtc);
				break;
			
			case 2:
				rtc->RTSR = (val &~ 3UL) | ((rtc->RTSR &~ val) & 3UL);
				pxa260rtcPrvUpdate(rtc);
				break;
			
			case 3:
				if(!(rtc->RTTR & 0x80000000UL)) rtc->RTTR = val;
				break;
		}
	}
	else{
		switch(pa){
			case 0:
				val = rtcCurTime() - rtc->RCNR_offset;
				break;
			
			case 1:
				val = rtc->RTAR;
				break;
			
			case 2:
				val = rtc->RTSR;
				break;
			
			case 3:
				val = rtc->RTTR;
				break;
		}
		*(UInt32*)buf = val;
	}
	
	return true;
}


Boolean pxa260rtcInit(Pxa255rtc* rtc, ArmMem* physMem, Pxa255ic* ic){
	
	__mem_zero(rtc, sizeof(Pxa255rtc));
	rtc->ic = ic;
	rtc->RCNR_offset = 0;
	rtc->RTTR = 0x7FFF;	//nice default value
	rtc->lastSeenTime = rtcCurTime();
	return memRegionAdd(physMem, PXA260_RTC_BASE, PXA260_RTC_SIZE, pxa260rtcPrvMemAccessF, rtc);
}

void pxa260rtcUpdate(Pxa255rtc* rtc){
	pxa260rtcPrvUpdate(rtc);
}
