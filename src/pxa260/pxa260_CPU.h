#ifndef _CPU_H_
#define _CPU_H_

#include "pxa260_types.h"
#include "../armv5te/cpu.h"

struct ArmCpu;

typedef Boolean	(*ArmCoprocRegXferF)	(struct ArmCpu* cpu, void* userData, Boolean two/* MCR2/MRC2 ? */, Boolean MRC, UInt8 op1, UInt8 Rx, UInt8 CRn, UInt8 CRm, UInt8 op2);
typedef Boolean	(*ArmCoprocDatProcF)	(struct ArmCpu* cpu, void* userData, Boolean two/* CDP2 ? */, UInt8 op1, UInt8 CRd, UInt8 CRn, UInt8 CRm, UInt8 op2);
typedef Boolean	(*ArmCoprocMemAccsF)	(struct ArmCpu* cpu, void* userData, Boolean two /* LDC2/STC2 ? */, Boolean N, Boolean store, UInt8 CRd, UInt32 addr, UInt8* option /* NULL if none */);
typedef Boolean (*ArmCoprocTwoRegF)	(struct ArmCpu* cpu, void* userData, Boolean MRRC, UInt8 op, UInt8 Rd, UInt8 Rn, UInt8 CRm);

typedef Boolean	(*ArmCpuMemF)		(struct ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean priviledged, UInt8* fsr);	//read/write
typedef Boolean	(*ArmCpuHypercall)	(struct ArmCpu* cpu);		//return true if handled
typedef void	(*ArmCpuEmulErr)	(struct ArmCpu* cpu, const char* err_str);

typedef void	(*ArmSetFaultAdrF)	(struct ArmCpu* cpu, UInt32 adr, UInt8 faultStatus);

/*

	coprocessors:
				
				0    - DSP (pxa only)
				0, 1 - WMMX (pxa only)
				11   - VFP (arm standard)
				15   - system control (arm standard)
*/


typedef struct{
	
	ArmCoprocRegXferF regXfer;
	ArmCoprocDatProcF dataProcessing;
	ArmCoprocMemAccsF memAccess;
	ArmCoprocTwoRegF  twoRegF;
	void* userData;
	
}ArmCoprocessor;

typedef struct{

	UInt32 R13, R14;
	UInt32 SPSR;			//usr mode doesn't have an SPSR
}ArmBankedRegs;


typedef struct ArmCpu{

	UInt32		regs[16];		//current active regs as per current mode
	UInt32		CPSR, SPSR;

	ArmBankedRegs	bank_usr;		//usr regs when in another mode
	ArmBankedRegs	bank_svc;		//svc regs when in another mode
	ArmBankedRegs	bank_abt;		//abt regs when in another mode
	ArmBankedRegs	bank_und;		//und regs when in another mode
	ArmBankedRegs	bank_irq;		//irq regs when in another mode
	ArmBankedRegs	bank_fiq;		//fiq regs when in another mode
	UInt32		extra_regs[5];		//fiq regs when not in fiq mode, usr regs when in fiq mode. R8-12

	UInt16		waitingIrqs;
	UInt16		waitingFiqs;
	UInt16		CPAR;

	ArmCoprocessor	coproc[16];		//coprocessors

	// various other cpu config options
	UInt32		vectorBase;		//address of vector base

	ArmCpuMemF	memF;
	ArmCpuEmulErr	emulErrF;
	ArmCpuHypercall	hypercallF;
	ArmSetFaultAdrF	setFaultAdrF;

	void*		userData;		//shared by all callbacks
}ArmCpu;

//UInt32 cpuGetRegExternal(ArmCpu* cpu, UInt8 reg);
//void cpuSetReg(ArmCpu* cpu, UInt8 reg, UInt32 val);
#define cpuGetRegExternal(x, regNum) reg(regNum)
#define cpuSetReg(x, regNum, value) set_reg(regNum, value)

//void cpuIrq(ArmCpu* cpu, Boolean fiq, Boolean raise);	//unraise when acknowledged
/*
static inline void cpuIrq(ArmCpu* cpu, Boolean fiq, Boolean raise){
   if(raise)
      cpu_exception(fiq ? EX_FIQ : EX_IRQ);
   //TODO: may need to fix not doing anything when cleared
}
*/
#define cpuIrq(x, fiq, raise) if(raise)cpu_exception(fiq ? EX_FIQ : EX_IRQ)

//void cpuCoprocessorRegister(ArmCpu* cpu, UInt8 cpNum, ArmCoprocessor* coproc);
//void cpuCoprocessorUnregister(ArmCpu* cpu, UInt8 cpNum);
#define cpuCoprocessorRegister(x, y, z)
#define cpuCoprocessorUnregister(x, y)
//TODO: may need to actually implement these

#endif

