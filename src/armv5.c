#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "armv5/CPU.h"


static ArmCpu armv5Cpu;
static uint32_t armv5StackLocation;


static Boolean armv5MemoryAccess(ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean privileged, UInt8* fsr){
   //ARM only has access to RAM, the OS 4 ROM has no data important to it and it would be wrong to access 68k registers from ARM

}

static Boolean armv5Hypercall(ArmCpu* cpu){
   //return true if handled, does nothing for now
   return true;
}

static void armv5EmulErr(ArmCpu* cpu, const char* errStr){
   debugLog(errStr);
}

static void armv5SetFaultAddr(struct ArmCpu* cpu, UInt32 adr, UInt8 faultStatus){

}

void armv5Init(void){
   //cant return an error, the only possible error was if int sizes are wrong which is already handled by (u)int(8/16/32/64)_t types
   cpuInit(&armv5Cpu, 0x00000000/*pc, set by 68k while emulating*/, armv5MemoryAccess, armv5EmulErr, armv5Hypercall, armv5SetFaultAddr);
   armv5StackLocation = 0x00000000;
}

void armv5Reset(void){

}

uint64_t armv5StateSize(void){
   uint64_t size = 0;

   return size;
}

void armv5SaveState(uint8_t* data){
   uint64_t offset = 0;

}

void armv5LoadState(uint8_t* data){
   uint64_t offset = 0;

}

void armv5SetStackLocation(uint32_t stackLocation){
   armv5StackLocation = stackLocation;
}

void armv5PceNativeCall(uint32_t functionPtr, uint32_t userdataPtr){
   //need to push some args to the stack here!

   cpuSetReg(&armv5Cpu, 15/*pc*/, functionPtr);
}

uint32_t armv5Execute(uint32_t cycles){
   //on a ARM device ARM is faster than 68k, each ARM opcode is equivalent to 2 68k cycles here

   while(cycles > 0){
      cpuCycleNoIrqs(&armv5Cpu);
      cycles--;
   }

   return cycles;
}

uint32_t armv5GetRegister(uint8_t reg){
   return cpuGetRegExternal(&armv5Cpu, reg);
}

uint32_t armv5GetPc(void){
   return cpuGetRegExternal(&armv5Cpu, 15/*pc*/);
}

uint64_t armv5ReadArbitraryMemory(uint32_t address, uint8_t size){
   return UINT64_MAX;//invalid access
}
