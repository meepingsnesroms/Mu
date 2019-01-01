#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "armv5/CPU.h"


static ArmCpu slaveCpu;


static Boolean armv5MemoryAccess(ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean privileged, UInt8* fsr){
   //ARM only has access to RAM, the OS 4 ROM has no data important to it and it would be wrong to access 68k registers from ARM

}

static Boolean armv5Hypercall(ArmCpu* cpu){
   //return true if handled, does nothing for now
   return true;
}

void armv5EmulErr(ArmCpu* cpu, const char* errStr){
   debugLog(errStr);
}

void armv5SetFaultAddr(struct ArmCpu* cpu, UInt32 adr, UInt8 faultStatus){

}

void armv5Init(void){
   //cant return an error, the only possible error was if int sizes are wrong which is already handled by (u)int(8/16/32/64)_t types
   cpuInit(&slaveCpu, 0x00000000/*pc, set by 68k while emulating*/, armv5MemoryAccess, armv5EmulErr, armv5Hypercall, armv5SetFaultAddr);
}

void armv5Reset(void){

}

uint64_t armv5StateSize(void){

}

void armv5SaveState(uint8_t* data){

}

void armv5LoadState(uint8_t* data){

}

void armv5Execute(uint32_t cycles){
   while(cycles > 0){
      cpuCycleNoIrqs(&slaveCpu);
      cycles--;
   }
}

uint32_t armv5GetRegister(uint8_t reg){

}

uint32_t armv5GetPc(void){

}

uint64_t armv5ReadArbitraryMemory(uint32_t address, uint8_t size){

}
