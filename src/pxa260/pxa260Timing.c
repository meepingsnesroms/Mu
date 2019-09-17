#include <stdint.h>
#include <string.h>
#include <setjmp.h>

#include "pxa260I2c.h"
#include "pxa260Ssp.h"
#include "pxa260Udc.h"
#include "pxa260Timing.h"
#include "../tsc2101.h"
#include "../armv5te/os/os.h"
#include "../armv5te/emu.h"
#include "../armv5te/cpu.h"


#define PXA260_TIMING_NEVER 0xFFFFFFFF


static int32_t pxa260TimingLeftoverCycles;//doesnt need to go in save states

void    (*pxa260TimingCallbacks[PXA260_TIMING_TOTAL_CALLBACKS])(void);
int32_t pxa260TimingQueuedEvents[PXA260_TIMING_TOTAL_CALLBACKS];


static int32_t pxa260TimingGetDurationUntilNextEvent(int32_t duration/*call with how long you want to run*/){
   uint8_t index;

   for(index = 0; index < PXA260_TIMING_TOTAL_CALLBACKS; index++)
      if(pxa260TimingQueuedEvents[index] != PXA260_TIMING_NEVER && pxa260TimingQueuedEvents[index] < duration)
         duration = pxa260TimingQueuedEvents[index];

   return duration;
}

void pxa260TimingInit(void){
   pxa260TimingCallbacks[PXA260_TIMING_CALLBACK_I2C_TRANSMIT_EMPTY] = pxa260I2cTransmitEmpty;
   pxa260TimingCallbacks[PXA260_TIMING_CALLBACK_I2C_RECEIVE_FULL] = pxa260I2cReceiveFull;
   pxa260TimingCallbacks[PXA260_TIMING_CALLBACK_SSP_TRANSFER_COMPLETE] = pxa260SspTransferComplete;
   pxa260TimingCallbacks[PXA260_TIMING_CALLBACK_UDC_DEVICE_RESUME_COMPLETE] = pxa260UdcDeviceResumeComplete;
   pxa260TimingCallbacks[PXA260_TIMING_CALLBACK_TSC2101_SCAN] = tsc2101Scan;
}

void pxa260TimingReset(void){
   uint8_t index;

   for(index = 0; index < PXA260_TIMING_TOTAL_CALLBACKS; index++)
      pxa260TimingQueuedEvents[index] = PXA260_TIMING_NEVER;
}

void pxa260TimingTriggerEvent(uint8_t callbackId, int32_t wait){
   pxa260TimingQueuedEvents[callbackId] = wait;
   //dont need to check if in handler since cycle_count_delta is 0 or positive when in handlers are called
   if(wait < -cycle_count_delta){
      pxa260TimingLeftoverCycles = -cycle_count_delta - wait;
      cycle_count_delta = -wait;
   }
}

void pxa260TimingCancelEvent(uint8_t callbackId){
   pxa260TimingQueuedEvents[callbackId] = PXA260_TIMING_NEVER;
}

void pxa260TimingRun(int32_t cycles){
#if OS_HAS_PAGEFAULT_HANDLER
   os_exception_frame_t seh_frame = {NULL, NULL};
#endif
   uint8_t index;
   int32_t addCycles;

#if OS_HAS_PAGEFAULT_HANDLER
   os_faulthandler_arm(&seh_frame);
#endif

   while(setjmp(restart_after_exception)){};
   exiting = false;
   pxa260TimingLeftoverCycles = 0;//used when an event is added while the CPU is running

   keepRunning:
   addCycles = pxa260TimingGetDurationUntilNextEvent(cycles);
   cycle_count_delta = -addCycles;

   while (!exiting && cycle_count_delta < 0) {
         if (cpu_events & (EVENT_FIQ | EVENT_IRQ)) {
             // Align PC in case the interrupt occurred immediately after a jump
             if (arm.cpsr_low28 & 0x20)
                 arm.reg[15] &= ~1;
             else
                 arm.reg[15] &= ~3;

             if (cpu_events & EVENT_WAITING)
                 arm.reg[15] += 4; // Skip over wait instruction

             arm.reg[15] += 4;
             cpu_exception((cpu_events & EVENT_FIQ) ? EX_FIQ : EX_IRQ);
         }
         cpu_events &= ~EVENT_WAITING;//the wait opcode will be executed again if still waiting, that will clear the remaining cycle count and exit the function again

         if (arm.cpsr_low28 & 0x20)
             cpu_thumb_loop();
         else
             cpu_arm_loop();
   }

   //if more then the requested cycles are executed count those too
   addCycles += cycle_count_delta;

   //remove the unused cycles
   addCycles -= pxa260TimingLeftoverCycles;
   pxa260TimingLeftoverCycles = 0;

   for(index = 0; index < PXA260_TIMING_TOTAL_CALLBACKS; index++){
      if(pxa260TimingQueuedEvents[index] != PXA260_TIMING_NEVER){
         pxa260TimingQueuedEvents[index] -= addCycles;
         if(pxa260TimingQueuedEvents[index] <= 0){
            //execute event
            pxa260TimingQueuedEvents[index] = PXA260_TIMING_NEVER;//set to never before calling function because it may retrigger the event and we dont want the new one cleared
            pxa260TimingCallbacks[index]();
         }
      }
   }

   cycles -= addCycles;
   if(cycles > 0)
      goto keepRunning;

#if OS_HAS_PAGEFAULT_HANDLER
   os_faulthandler_unarm(&seh_frame);
#endif
}
