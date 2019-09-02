#include "pxa260.h"
#include "pxa260_TIMR.h"
#include "pxa260_mem.h"
#include "../emulator.h"


static void pxa260timrPrvRaiseLowerInts(Pxa260timr* timr){
	
	pxa260icInt(timr->ic, PXA260_I_TIMR0, (timr->OSSR & 1) != 0);
	pxa260icInt(timr->ic, PXA260_I_TIMR1, (timr->OSSR & 2) != 0);
	pxa260icInt(timr->ic, PXA260_I_TIMR2, (timr->OSSR & 4) != 0);
	pxa260icInt(timr->ic, PXA260_I_TIMR3, (timr->OSSR & 8) != 0);
}

static void pxa260timrPrvCheckMatch(Pxa260timr* timr, UInt8 idx){
	
	UInt8 v = 1UL << idx;
	
	if((timr->OSCR == timr->OSMR[idx]) && (timr->OIER & v)){
		timr->OSSR |= v;
	}
}

static void pxa260timrPrvUpdate(Pxa260timr* timr){
	
	pxa260timrPrvCheckMatch(timr, 0);
	pxa260timrPrvCheckMatch(timr, 1);
	pxa260timrPrvCheckMatch(timr, 2);
	pxa260timrPrvCheckMatch(timr, 3);
}

Boolean pxa260timrPrvMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf){

	Pxa260timr* timr = userData;
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
	
	pa = (pa - PXA260_TIMR_BASE) >> 2;

   //debugLog("PXA260 TIMR access:0x%04X, write:%d\n", pa, write);
	
	if(write){
		val = *(UInt32*)buf;
		
		switch(pa){
			case 0:
			case 1:
			case 2:
			case 3:
				timr->OSMR[pa] = val;
				break;
			
			case 4:
				timr->OSCR = val;
				break;
			
			case 5:
				timr->OSSR = timr->OSSR &~ val;
				pxa260timrPrvRaiseLowerInts(timr);
				break;
			
			case 6:
				timr->OWER = val;
				break;
			
			case 7:
				timr->OIER = val;
				pxa260timrPrvUpdate(timr);
				pxa260timrPrvRaiseLowerInts(timr);
				break;
		}
	}
	else{
		switch(pa){
			case 0:
			case 1:
			case 2:
			case 3:
				val = timr->OSMR[pa];
				break;
			
			case 4:
				val = timr->OSCR;
				break;
			
			case 5:
				val = timr->OSSR;
				break;
			
			case 6:
				val = timr->OWER;
				break;
			
			case 7:
				val = timr->OIER;
				break;
		}
		*(UInt32*)buf = val;
	}
	
	return true;
}


void pxa260timrInit(Pxa260timr* timr, Pxa260ic* ic){
	
	__mem_zero(timr, sizeof(Pxa260timr));
	timr->ic = ic;
}

void pxa260timrTick(Pxa260timr* timr){
	
	timr->OSCR++;
	pxa260timrPrvUpdate(timr);
	pxa260timrPrvRaiseLowerInts(timr);
}
