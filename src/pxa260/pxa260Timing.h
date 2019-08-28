#ifndef PXA260_TIMING_H
#define PXA260_TIMING_H

#include <stdint.h>

enum{
   PXA260_TIMING_CALLBACK_I2C_TRANSMIT_EMPTY = 0,
   PXA260_TIMING_CALLBACK_I2C_RECEIVE_FULL,
   PXA260_TIMING_CALLBACK_SSP_TRANSFER_COMPLETE,
   PXA260_TIMING_CALLBACK_TSC2101_SCAN,
   PXA260_TIMING_TOTAL_CALLBACKS
};

extern void (*pxa260TimingCallbacks[])(void);//saving a function pointer in a state is not portable and will crash even on the same system between program loads with ASLR
extern int32_t pxa260TimingQueuedEvents[];

void pxa260TimingInit(void);
void pxa260TimingReset(void);

void pxa260TimingTriggerEvent(uint8_t callbackId, int32_t wait);
void pxa260TimingCancelEvent(uint8_t callbackId);
void pxa260TimingRun(int32_t cycles);//this runs the CPU

#endif
