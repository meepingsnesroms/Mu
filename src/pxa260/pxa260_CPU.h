#ifndef _CPU_H_
#define _CPU_H_

#include "pxa260_types.h"
#include "../armv5te/emu.h"
#include "../armv5te/cpu.h"

//UInt32 cpuGetRegExternal(ArmCpu* cpu, UInt8 reg);
//void cpuSetReg(ArmCpu* cpu, UInt8 reg, UInt32 val);
#define cpuGetRegExternal(x, regNum) reg_pc(regNum)
#define cpuSetReg(x, regNum, value) set_reg(regNum, value)

//void cpuIrq(ArmCpu* cpu, Boolean fiq, Boolean raise);	//unraise when acknowledged
//#define cpuIrq(x, fiq, raise) if(raise)cpu_exception(fiq ? EX_FIQ : EX_IRQ)
#define cpuIrq(x, fiq, raise) (raise ? (cpu_events |= (fiq ? EVENT_FIQ : EVENT_IRQ)) : (cpu_events &= (fiq ? ~EVENT_FIQ : ~EVENT_IRQ)))

//void cpuCoprocessorRegister(ArmCpu* cpu, UInt8 cpNum, ArmCoprocessor* coproc);
//void cpuCoprocessorUnregister(ArmCpu* cpu, UInt8 cpNum);
#define cpuCoprocessorRegister(x, y, z)
#define cpuCoprocessorUnregister(x, y)
//TODO: may need to actually implement these

#endif

