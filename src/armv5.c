#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "portability.h"
#include "armv5/CPU.h"


#define ARMV5_CYCLES_PER_OPCODE 2


bool armv5ServiceRequest;

static ArmCpu armv5Cpu;


static Boolean armv5MemoryAccess(ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean privileged, UInt8* fsr){
   //ARM only has access to RAM, the OS 4 ROM has no data important to it and it would be wrong to access 68k registers from ARM
   vaddr &= chips[CHIP_DX_RAM].mask;

#if !defined(EMU_NO_SAFETY)
   if(size & (size - 1) || vaddr & (size - 1))//size is not a power of two or address isnt aligned to size
      return false;
#endif

#if defined(EMU_BIG_ENDIAN)
   if(write){
      switch(size){
         case 1:
            *(uint8_t*)(palmRam + vaddr) = *(uint8_t*)buf;
            return true;

         case 2:
            *(uint16_t*)(palmRam + vaddr) = SWAP_16(*(uint16_t*)buf);
            return true;

         case 4:
            *(uint32_t*)(palmRam + vaddr) = SWAP_32(*(uint32_t*)buf);
            return true;

         default:
            return false;
      }
   }
   else{
      switch(size){
         case 1:
            *(uint8_t*)buf = *(uint8_t*)(palmRam + vaddr);
            return true;

         case 2:
            *(uint16_t*)buf = SWAP_16(*(uint16_t*)(palmRam + vaddr));
            return true;

         case 4:
            *(uint32_t*)buf = SWAP_32(*(uint32_t*)(palmRam + vaddr));
            return true;

         default:
            return false;
      }
   }
#else
   if(write){
      switch(size){
         case 1:
            *(uint8_t*)(palmRam + (vaddr ^ 1)) = *(uint8_t*)buf;
            return true;

         case 2:
            *(uint16_t*)(palmRam + vaddr) = SWAP_16(*(uint16_t*)buf);
            return true;

         case 4:
            *(uint16_t*)(palmRam + vaddr) = SWAP_16(*(uint32_t*)buf & 0xFFFF);
            *(uint16_t*)(palmRam + vaddr + 2) = SWAP_16(*(uint32_t*)buf >> 16);
            return true;

         default:
            return false;
      }
   }
   else{
      switch(size){
         case 1:
            *(uint8_t*)buf = *(uint8_t*)(palmRam + (vaddr ^ 1));
            return true;

         case 2:
            *(uint16_t*)buf = SWAP_16(*(uint16_t*)(palmRam + vaddr));
            return true;

         case 4:
            *(uint32_t*)buf = SWAP_16(*(uint16_t*)(palmRam + vaddr + 2)) << 16 | SWAP_16(*(uint16_t*)(palmRam + vaddr));
            return true;

         default:
            return false;
      }
   }
#endif
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
