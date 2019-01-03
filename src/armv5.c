#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "portability.h"
#include "armv5/CPU.h"


static ArmCpu armv5Cpu;
static uint32_t armv5StackLocation;


static Boolean armv5MemoryAccess(ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean privileged, UInt8* fsr){
   //ARM only has access to RAM, the OS 4 ROM has no data important to it and it would be wrong to access 68k registers from ARM
   vaddr &= chips[CHIP_DX_RAM].mask;

#if !defined(EMU_NO_SAFETY)
   if(size & (size - 1))//size is not a power of two
      return false;
   if(vaddr & (size - 1))//bad alignment
      return false;
#endif

#if defined(EMU_BIG_ENDIAN)
   if(write){
      switch(size){
         case 1:
            *(uint8_t*)(palmRam + vaddr) = *(uint8_t*)buf;
            break;

         case 2:
            *(uint16_t*)(palmRam + vaddr) = SWAP_16(*(uint16_t*)buf);
            break;

         case 4:
            *(uint32_t*)(palmRam + vaddr) = SWAP_32(*(uint32_t*)buf);
            break;

         default:
            return false;
      }
   }
   else{
      switch(size){
         case 1:
            *(uint8_t*)buf = *(uint8_t*)(palmRam + vaddr);
            break;

         case 2:
            *(uint16_t*)buf = SWAP_16(*(uint16_t*)(palmRam + vaddr));
            break;

         case 4:
            *(uint32_t*)buf = SWAP_32(*(uint32_t*)(palmRam + vaddr));
            break;

         default:
            return false;
      }
   }
#else
   if(write){
      switch(size){
         case 1:
            *(uint8_t*)(palmRam + (vaddr ^ 1)) = *(uint8_t*)buf;
            break;

         case 2:
            *(uint16_t*)(palmRam + vaddr) = SWAP_16(*(uint16_t*)buf);
            break;

         case 4:
            *(uint16_t*)(palmRam + vaddr) = SWAP_16(*(uint32_t*)buf & 0xFFFF);
            *(uint16_t*)(palmRam + vaddr + 2) = SWAP_16(*(uint32_t*)buf >> 16);
            break;

         default:
            return false;
      }
   }
   else{
      switch(size){
         case 1:
            *(uint8_t*)buf = *(uint8_t*)(palmRam + (vaddr ^ 1));
            break;

         case 2:
            *(uint16_t*)buf = SWAP_16(*(uint16_t*)(palmRam + vaddr));
            break;

         case 4:
            *(uint32_t*)buf = SWAP_16(*(uint16_t*)(palmRam + vaddr + 2)) << 16 | SWAP_16(*(uint16_t*)(palmRam + vaddr));
            break;

         default:
            return false;
      }
   }
#endif

   return true;
}

static Boolean armv5Hypercall(ArmCpu* cpu){
   //return true if handled, does nothing for now
   return true;
}

static void armv5EmulErr(ArmCpu* cpu, const char* errStr){
   debugLog("%s", errStr);
}

static void armv5SetFaultAddr(struct ArmCpu* cpu, UInt32 adr, UInt8 faultStatus){
   //TODO
}

void armv5Init(void){
   //cant return an error, the only possible error was if int sizes are wrong which is already handled by (u)int(8/16/32/64)_t types
   cpuInit(&armv5Cpu, 0x00000000/*pc, set by 68k while emulating*/, armv5MemoryAccess, armv5EmulErr, armv5Hypercall, armv5SetFaultAddr);
   armv5StackLocation = 0x00000000;
}

void armv5Reset(void){
   armv5StackLocation = 0x00000000;
}

uint64_t armv5StateSize(void){
   uint64_t size = 0;

   //need to add armv5Cpu here
   size += sizeof(uint32_t);//armv5StackLocation

   return size;
}

void armv5SaveState(uint8_t* data){
   uint64_t offset = 0;

   //need to add armv5Cpu here
   writeStateValue32(data + offset, armv5StackLocation);
   offset += sizeof(uint32_t);
}

void armv5LoadState(uint8_t* data){
   uint64_t offset = 0;

   //need to add armv5Cpu here
   armv5StackLocation = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
}

void armv5SetStackLocation(uint32_t stackLocation){
   armv5StackLocation = stackLocation;
}

void armv5PceNativeCall(uint32_t functionPtr, uint32_t userdataPtr){
   //need to push some args and a return to 68k mode function to the stack here!

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
