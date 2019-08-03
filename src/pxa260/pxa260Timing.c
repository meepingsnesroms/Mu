#include <stdint.h>
#include <string.h>

#include "../armv5te/emu.h"
#include "../armv5te/cpu.h"
#include "pxa260I2c.h"
#include "pxa260Timing.h"


static int32_t pxa260TimingLeftoverCycles;

void (*pxa260TimingCallbacks[PXA260_TIMING_TOTAL_CALLBACKS])(void);
event_t pxa260TimingQueuedEvents[20];


static int32_t pxa260TimingGetDurationUntilNextEvent(int32_t duration/*call with how long you want to run*/){
   uint8_t index;

   for(index = 0; index < sizeof(pxa260TimingQueuedEvents) / sizeof(event_t); index++)
      if(pxa260TimingQueuedEvents[index].active && pxa260TimingQueuedEvents[index].wait < duration)
         duration = pxa260TimingQueuedEvents[index].wait;

   return duration;
}

void pxa260TimingInit(void){
   pxa260TimingCallbacks[PXA260_TIMING_CALLBACK_I2C_TRANSMIT_EMPTY] = pxa260I2cTransmitEmpty;
   pxa260TimingCallbacks[PXA260_TIMING_CALLBACK_I2C_RECEIVE_FULL] = pxa260I2cReceiveFull;
}

void pxa260TimingReset(void){
   memset(pxa260TimingQueuedEvents, 0x00, sizeof(pxa260TimingQueuedEvents));
}

void pxa260TimingQueueEvent(int32_t wait, uint8_t callbackId){
   uint8_t index;

   for(index = 0; index < sizeof(pxa260TimingQueuedEvents) / sizeof(event_t); index++){
      if(!pxa260TimingQueuedEvents[index].active){
         //found empty slot, enqueue event
         pxa260TimingQueuedEvents[index].wait = wait;
         pxa260TimingQueuedEvents[index].callbackId = callbackId;
         pxa260TimingQueuedEvents[index].active = true;
         if(wait < -cycle_count_delta){
            pxa260TimingLeftoverCycles = -cycle_count_delta - wait;
            cycle_count_delta = -wait;
         }
         return;
      }
   }

   debugLog("Unable to find CPU event slot!!!");
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
         cpu_events &= ~EVENT_WAITING;//this might need to be move above?

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

   for(index = 0; index < sizeof(pxa260TimingQueuedEvents) / sizeof(event_t); index++){
      if(pxa260TimingQueuedEvents[index].active){
         pxa260TimingQueuedEvents[index].wait -= addCycles;
         if(pxa260TimingQueuedEvents[index].wait <= 0){
            //execute event
            pxa260TimingCallbacks[pxa260TimingQueuedEvents[index].callbackId]();
            pxa260TimingQueuedEvents[index].active = false;
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
