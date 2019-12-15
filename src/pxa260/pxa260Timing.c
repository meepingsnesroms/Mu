#include <stdint.h>
#include <string.h>

#include "pxa260.h"
#include "pxa260_TIMR.h"
#include "pxa260I2c.h"
#include "pxa260Ssp.h"
#include "pxa260Udc.h"
#include "pxa260Timing.h"
#include "pxa260_CPU.h"
#include "../tsc2101.h"
#include "../emulator.h"


#define PXA260_TIMING_NEVER 0xFFFFFFFF


static int32_t pxa260TimingLeftoverCycles;//doesnt need to go in save states
static int32_t pxa260CycleCountDelta;//doesnt need to go in save states

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
   pxa260TimingCallbacks[PXA260_TIMING_CALLBACK_TICK_CPU_TIMER] = pxa260TimingTickCpuTimer;
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
   if(wait < -pxa260CycleCountDelta){
      pxa260TimingLeftoverCycles = -pxa260CycleCountDelta - wait;
      pxa260CycleCountDelta = -wait;
   }
}

void pxa260TimingCancelEvent(uint8_t callbackId){
   pxa260TimingQueuedEvents[callbackId] = PXA260_TIMING_NEVER;
}

void pxa260TimingRun(int32_t cycles){
   uint8_t index;
   int32_t addCycles;

   pxa260TimingLeftoverCycles = 0;//used when an event is added while the CPU is running

   keepRunning:
   addCycles = pxa260TimingGetDurationUntilNextEvent(cycles);
   pxa260CycleCountDelta = -addCycles * palmClockMultiplier;

   while(pxa260CycleCountDelta < 0){
      cpuCycle(&pxa260CpuState);
      pxa260CycleCountDelta += 1;
   }

   //if more then the requested cycles are executed count those too
   addCycles += pxa260CycleCountDelta / palmClockMultiplier;

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
}

void pxa260TimingTickCpuTimer(void){
   pxa260timrTick(&pxa260Timer);
   pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_TICK_CPU_TIMER, TUNGSTEN_T3_CPU_PLL_FREQUENCY / TUNGSTEN_T3_CPU_CRYSTAL_FREQUENCY);
}
