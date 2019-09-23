#ifndef _PXA260_DSP_H_
#define _PXA260_DSP_H_

#include "pxa260_types.h"

typedef struct{
	UInt64 acc0;
}Pxa260dsp;

Boolean pxa260dspAccess(void* userData, Boolean MRRC, UInt8 op, UInt8 RdLo, UInt8 RdHi, UInt8 acc);
void pxa260dspInit(Pxa260dsp* dsp);

#endif
