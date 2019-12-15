#ifndef PXA260_PWR_CLK_H
#define PXA260_PWR_CLK_H

#include "pxa260_CPU.h"

typedef struct{
	UInt32 CCCR, CKEN, OSCR;	//clocks manager regs
	UInt32 pwrRegs[13];		//we care so little about these, we don't even name them
	Boolean turbo;
}Pxa260pwrClk;


#define PXA260_CLOCK_MANAGER_BASE	0x41300000UL
#define PXA260_CLOCK_MANAGER_SIZE	0x00001000UL

#define PXA260_POWER_MANAGER_BASE	0x40F00000UL
#define PXA260_POWER_MANAGER_SIZE	0x00001000UL

Boolean pxa260pwrClkPrvCoprocRegXferFunc(struct ArmCpu* unused, void* userData, Boolean two, Boolean read, UInt8 op1, UInt8 Rx, UInt8 CRn, UInt8 CRm, UInt8 op2);
Boolean pxa260pwrClkPrvClockMgrMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf);
Boolean pxa260pwrClkPrvPowerMgrMemAccessF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* buf);
void pxa260pwrClkInit(Pxa260pwrClk* pc);

#endif
