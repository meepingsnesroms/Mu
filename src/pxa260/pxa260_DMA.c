#include "pxa260.h"
#include "pxa260_DMA.h"

#define REG_DAR 	0
#define REG_SAR 	1
#define REG_TAR 	2
#define REG_CR		3
#define REG_CSR   4


static void pxa260dmaPrvChannelRegWrite(_UNUSED_ Pxa260dma* dma, UInt8 channel, UInt8 reg, UInt32 val){
	
	if(val){	//we start with zeros, so non-zero writes are all we care about
		
		const char* regs[] = {"DADDR", "SADDR", "TADDR", "CR", "CSR"};
		
		err_str("dma: writes unimpl!");
	//	err_str("PXA260 dma engine: writes unimpl! (writing 0x");
	//	err_hex(val);
	//	err_str(" to channel ");
	//	err_dec(channel);
	//	err_str(" reg ");
	//	err_str(regs[reg]);
	//	err_str(". Halting.\r\n");
		while(1);	
	}
}

static UInt32 pxa260dmaPrvChannelRegRead(_UNUSED_ Pxa260dma* dma, _UNUSED_ UInt8 channel, _UNUSED_ UInt8 reg){
	
	
	return 0;	
}

static Boolean pxa260dmaPrvMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf){

   Pxa260dma* dma = userData;
	UInt8 reg, set;
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
	
	pa = (pa - PXA260_DMA_BASE) >> 2;

   debugLog("PXA260 DMA access:0x%04X, write:%d, PC:0x%08X\n", pa, write, pxa260GetPc());
	
	if(write){
		val = *(UInt32*)buf;
		
		switch(pa >> 6){		//weird, but quick way to avoide repeated if-then-elses. this is faster
			case 0:
				if(pa < 16){
					reg = REG_CSR;
					set = pa;
					pxa260dmaPrvChannelRegWrite(dma, set, reg, val);
				}
				break;
				
			case 1:
				pa -= 64;
				if(pa < 40) dma->CMR[pa] = val;
				break;
			
			case 2:
				pa -= 128;
				set = pa >> 2;
				reg = pa & 3;
				pxa260dmaPrvChannelRegWrite(dma, set, reg, val);
				break;
		}
	}
	else{
		switch(pa >> 6){		//weird, but quick way to avoide repeated if-then-elses. this is faster
			case 0:
				if(pa < 16){
					reg = REG_CSR;
					set = pa;
					val = pxa260dmaPrvChannelRegRead(dma, set, reg);
				}
				break;
				
			case 1:
				pa -= 64;
				if(pa < 40) val = dma->CMR[pa];
				break;
			
			case 2:
				pa -= 128;
				set = pa >> 2;
				reg = pa & 3;
				val = pxa260dmaPrvChannelRegRead(dma, set, reg);
				break;
		}
		
		*(UInt32*)buf = val;
	}
	
	return true;
}


void pxa260dmaInit(Pxa260dma* dma, Pxa260ic* ic){
	
   __mem_zero(dma, sizeof(Pxa260dma));
	dma->ic = ic;
   //dma->mem = physMem;
	
   //return memRegionAdd(physMem, PXA260_DMA_BASE, PXA260_DMA_SIZE, pxa260dmaPrvMemAccessF, dma);
   return true;
}
