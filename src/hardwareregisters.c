#include <stdio.h>

#include <boolean.h>

#include "emulator.h"
#include "hardwareRegisterNames.h"


static inline unsigned int registerArrayRead8(unsigned int address){
   return palmReg[address];
}

static inline unsigned int registerArrayRead16(unsigned int address){
   return palmReg[address] << 8 | palmReg[address + 1];
}

static inline unsigned int registerArrayRead32(unsigned int address){
   return palmReg[address] << 24 | palmReg[address + 1] << 16 | palmReg[address + 2] << 8 | palmReg[address + 3];
}

static inline void registerArrayWrite8(unsigned int address, unsigned int value){
   palmReg[address] = value;
}

static inline void registerArrayWrite16(unsigned int address, unsigned int value){
   palmReg[address] = value >> 8;
   palmReg[address + 1] = value & 0xFF;
}

static inline void registerArrayWrite32(unsigned int address, unsigned int value){
   palmReg[address] = value >> 24;
   palmReg[address + 1] = (value >> 16) & 0xFF;
   palmReg[address + 2] = (value >> 8) & 0xFF;
   palmReg[address + 3] = value & 0xFF;
}


void printUnknownHwAccess(unsigned int address, unsigned int value, unsigned int size, bool isWrite){
   if(isWrite){
      printf("Cpu Wrote %d bits of 0x%08X to register 0x%04X.\n", size, value, address);
   }
   else{
      printf("Cpu Read %d bits from register 0x%04X.\n", size, address);
   }
}


unsigned int getPllfsr16(){
   unsigned int currentPllfsr = registerArrayRead16(PLLFSR);
   return palmCrystal ? currentPllfsr | 0x8000 : currentPllfsr & 0x7FFF;
}

void setPllfsr16(unsigned int value){
   if(!(registerArrayRead16(PLLFSR) & 0x4000)){
      //frequency protect bit not set
      registerArrayWrite16(PLLFSR, value & 0x4FFF);
      uint32_t p = value & 0x00FF;
      uint32_t q = (value & 0x0F00) >> 8;
      uint32_t newCrystalCycles = 2 * (14 * (p + 1) + q + 1);
      uint32_t newFrequency = newCrystalCycles * 32768;
      printf("New CPU frequency of:%d cycles per second.\n", newFrequency);
      printf("New clk32 cycle count of :%d.\n", newCrystalCycles);
      
      palmCpuFrequency = newFrequency;
   }
}


unsigned int getHwRegister8(unsigned int address){
   switch(address){
         
      default:
         printUnknownHwAccess(address, 0, 8, false);
         return registerArrayRead8(address);
   }
}

unsigned int getHwRegister16(unsigned int address){
   switch(address){
         
      case PLLFSR:
         return getPllfsr16();
         
      default:
         printUnknownHwAccess(address, 0, 16, false);
         return registerArrayRead16(address);
   }
}

unsigned int getHwRegister32(unsigned int address){
   switch(address){
         
      default:
         printUnknownHwAccess(address, 0, 32, false);
         return registerArrayRead32(address);
   }
}


void setHwRegister8(unsigned int address, unsigned int value){
   switch(address){
         
      default:
         printUnknownHwAccess(address, value, 8, true);
         registerArrayWrite8(address, value);
         break;
   }
}

void setHwRegister16(unsigned int address, unsigned int value){
   switch(address){
         
      case PLLFSR:
         setPllfsr16(value);
         break;
         
      default:
         printUnknownHwAccess(address, value, 16, true);
         registerArrayWrite16(address, value);
         break;
   }
}

void setHwRegister32(unsigned int address, unsigned int value){
   switch(address){
      
      default:
         printUnknownHwAccess(address, value, 32, true);
         registerArrayWrite32(address, value);
         break;
   }
}


void initHwRegisters(){
   registerArrayWrite8(SCR, 0x1C);
   registerArrayWrite32(IDR, 0x56000000);
   registerArrayWrite16(IODCR, 0x1FFF);
   registerArrayWrite16(CSA, 0x00B0);
   registerArrayWrite16(CSD, 0x0200);
   registerArrayWrite16(EMUCS, 0x0060);
   registerArrayWrite16(PLLCR, 0x24B3);
   registerArrayWrite16(PLLFSR, 0x0347);
}
