#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "portability.h"
#include "armv5/CPU.h"
#include "m68k/m68k.h"


#define ARMV5_CYCLES_PER_OPCODE 2


bool armv5ServiceRequest;

static ArmCpu armv5Cpu;


static Boolean armv5MemoryAccess(ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean privileged, UInt8* fsr){
   //just gonna have ARM and m68k share the same bus for now, ARM may want fonts/bitmaps from ROM

   //no safety check is needed, they are done in the m68k memory accessors called from here

   if(write){
      switch(size){
         case 1:
            m68k_write_memory_8(vaddr, *(uint8_t*)buf);
            return true;

         case 2:
            m68k_write_memory_16(vaddr, SWAP_16(*(uint16_t*)buf));
            return true;

         case 4:
            m68k_write_memory_32(vaddr, SWAP_32(*(uint32_t*)buf));
            return true;

         default:
            debugLog("Invalid ARMv5 write: address:0x%08X, size:%d\n", vaddr, size * 8);
            return false;
      }
   }
   else{
      switch(size){
         case 1:
            *(uint8_t*)buf = m68k_read_memory_8(vaddr);
            return true;

         case 2:
            *(uint16_t*)buf = SWAP_16(m68k_read_memory_16(vaddr));
            return true;

         case 4:
            *(uint32_t*)buf = SWAP_32(m68k_read_memory_32(vaddr));
            return true;

         default:
            debugLog("Invalid ARMv5 read: address:0x%08X, size:%d\n", vaddr, size * 8);
            return false;
      }
   }
}

static Boolean armv5Hypercall(ArmCpu* cpu){
   armv5ServiceRequest = true;
   return true;
}

static void armv5EmulErr(ArmCpu* cpu, const char* errStr){
   debugLog("%s", errStr);
}

static void armv5SetFaultAddr(struct ArmCpu* cpu, UInt32 adr, UInt8 faultStatus){
   //TODO
}

void armv5Reset(void){
   cpuInit(&armv5Cpu, 0x00000000/*PC, set by 68k while emulating*/, armv5MemoryAccess, armv5EmulErr, armv5Hypercall, armv5SetFaultAddr);
   armv5ServiceRequest = false;
}

uint32_t armv5StateSize(void){
   uint32_t size = 0;

   //need to add armv5Cpu here
   size += sizeof(uint8_t);

   return size;
}

void armv5SaveState(uint8_t* data){
   uint32_t offset = 0;

   //need to add armv5Cpu here
   writeStateValue8(data + offset, armv5ServiceRequest);
   offset += sizeof(uint8_t);
}

void armv5LoadState(uint8_t* data){
   uint32_t offset = 0;

   //need to add armv5Cpu here
   armv5ServiceRequest = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
}

uint32_t armv5GetRegister(uint8_t reg){
   return cpuGetRegExternal(&armv5Cpu, reg);
}

void armv5SetRegister(uint8_t reg, uint32_t value){
   cpuSetReg(&armv5Cpu, reg, value);
}

int32_t armv5Execute(int32_t cycles){
   armv5ServiceRequest = false;

   //execution aborts on hypercall to request things from the 68k
   while(cycles > 0 && !armv5ServiceRequest){
      cpuCycleNoIrqs(&armv5Cpu);
      cycles -= ARMV5_CYCLES_PER_OPCODE;
   }

   return cycles;
}

uint32_t armv5GetPc(void){
   return cpuGetRegExternal(&armv5Cpu, 15/*pc*/);
}
