#ifndef PXA260_TIMING_H
#define PXA260_TIMING_H

#include <stdint.h>

enum{
   PXA260_TIMING_CALLBACK_I2C_TRANSMIT_EMPTY = 0,
   PXA260_TIMING_CALLBACK_I2C_RECEIVE_FULL,
   PXA260_TIMING_TOTAL_CALLBACKS
};

typedef struct{
   int32_t wait;
   uint8_t callbackId;
   bool active;
}event_t;

extern void (*pxa260TimingCallbacks[])(void);//saving a function pointer in a state is not portable and will crash even on the same system between program loads with ASLR
extern event_t pxa260TimingQueuedEvents[];

void pxa260TimingInit(void);
void pxa260TimingReset(void);

void pxa260TimingQueueEvent(int32_t wait, uint8_t callbackId);
void pxa260TimingRun(int32_t cycles);//this runs the CPU

#endif
