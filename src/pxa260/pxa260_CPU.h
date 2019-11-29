#ifndef PXA260_CPU_H
#define PXA260_CPU_H

#include "pxa260_types.h"

#if defined(EMU_NO_SAFETY)
#include "../armv5te/emu.h"
#include "../armv5te/cpu.h"

#define cpuGetRegExternal(x, regNum) reg_pc(regNum)
#define cpuSetReg(x, regNum, value) set_reg(regNum, value)

#define cpuIrq(x, fiq, raise) (raise ? (cpu_events |= (fiq ? EVENT_FIQ : EVENT_IRQ)) : (cpu_events &= (fiq ? ~EVENT_FIQ : ~EVENT_IRQ)))
#else
#include "../armv5te/uArm/CPU_2.h"
#endif

#endif

