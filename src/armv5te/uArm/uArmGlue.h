#ifndef UARM_GLUE_H
#define UARM_GLUE_H

#include "CPU_2.h"

#ifdef __cplusplus
extern "C" {
#endif

Boolean	uArmMemAccess(struct ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean priviledged, UInt8* fsr);	//read/write
Boolean	uArmHypercall(struct ArmCpu* cpu);//return true if handled
void	uArmEmulErr	(struct ArmCpu* cpu, const char* err_str);
void	uArmSetFaultAddr(struct ArmCpu* cpu, UInt32 adr, UInt8 faultStatus);

void uArmInitCpXX(ArmCpu* cpu);

#ifdef __cplusplus
}
#endif

#endif
