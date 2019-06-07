#ifndef _PXA255_PWR_CLK_H_
#define _PXA255_PWR_CLK_H_

#include "pxa255_mem.h"
#include "pxa255_CPU.h"

typedef struct{
	UInt32 CCCR, CKEN, OSCR;	//clocks manager regs
	UInt32 pwrRegs[13];		//we care so little about these, we don't even name them
	Boolean turbo;
}Pxa255pwrClk;


#define PXA255_CLOCK_MANAGER_BASE	0x41300000UL
#define PXA255_CLOCK_MANAGER_SIZE	0x00001000UL

#define PXA255_POWER_MANAGER_BASE	0x40F00000UL
#define PXA255_POWER_MANAGER_SIZE	0x00001000UL

Boolean pxa255pwrClkPrvCoprocRegXferFunc(void* userData, Boolean two, Boolean read, UInt8 op1, UInt8 Rx, UInt8 CRn, UInt8 CRm, UInt8 op2);
Boolean pxa255pwrClkPrvClockMgrMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf);
Boolean pxa255pwrClkPrvPowerMgrMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf);
void pxa255pwrClkInit(Pxa255pwrClk* pc);

#endif
