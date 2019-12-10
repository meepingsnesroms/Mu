#ifndef PXA260_CPU_H
#define PXA260_CPU_H

#include "pxa260_types.h"

#if defined(EMU_NO_SAFETY)
#include "../armv5te/emu.h"
#include "../armv5te/cpu.h"

#define cpuGetRegExternal(x, regNum) reg_pc(regNum)
#define cpuSetReg(x, regNum, value) set_reg(regNum, value)

static inline void cpuIrqReal(Boolean fiq, Boolean raise){	//unraise when acknowledged

   if(fiq){
      if(raise){
         waitingFiqs++;
      }
      else if(waitingFiqs){
         waitingFiqs--;
      }
      else{
         debugLog("Cannot unraise FIQ when none raised");
      }
   }
   else{
      if(raise){
         waitingIrqs++;
      }
      else if(waitingIrqs){
         waitingIrqs--;
      }
      else{
         debugLog("Cannot unraise IRQ when none raised");
      }
   }

   cpu_events &= ~(EVENT_FIQ | EVENT_IRQ);

   if(waitingFiqs)
      cpu_events |= EVENT_FIQ;

   if(waitingIrqs)
      cpu_events |= EVENT_IRQ;
}

#define cpuIrq(gone, fiq, raise) cpuIrqReal(fiq, raise)
#else
#include "../armv5te/uArm/CPU_2.h"
#endif

#endif

