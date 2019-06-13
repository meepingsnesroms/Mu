#ifndef _PXA260_DSP_H_
#define _PXA260_DSP_H_

#include "pxa260_mem.h"
#include "pxa260_CPU.h"



typedef struct{
	
	UInt64 acc0;
	
}Pxa255dsp;



Boolean pxa260dspInit(Pxa255dsp* dsp, ArmCpu* cpu);


#endif
