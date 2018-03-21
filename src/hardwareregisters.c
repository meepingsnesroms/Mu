#include <stdio.h>

#include <boolean.h>

#include "emulator.h"

void printUnknownHwAccess(unsigned int address, unsigned int value, unsigned int size, bool isWrite){
   if(isWrite){
      printf("Cpu Wrote %d bits of 0x%08X to register 0x%04X.\n", size, value, address);
   }
   else{
      printf("Cpu Read %d bits from register 0x%04X.\n", size, address);
   }
}


unsigned int getHwRegister8(unsigned int address){
   switch(address){
         
      default:
         printUnknownHwAccess(address, 0, 8, false);
         break;
   }
   
   return palmReg[address];
}

unsigned int getHwRegister16(unsigned int address){
   switch(address){
         
      default:
         printUnknownHwAccess(address, 0, 16, false);
         break;
   }
   
   return palmReg[address] << 8 | palmReg[address + 1];
}

unsigned int getHwRegister32(unsigned int address){
   switch(address){
         
      default:
         printUnknownHwAccess(address, 0, 32, false);
   }
   
   return palmReg[address] << 24 | palmReg[address + 1] << 16 | palmReg[address + 2] << 8 | palmReg[address + 3];
}


void setHwRegister8(unsigned int address, unsigned int value){
   switch(address){
         
      default:
         printUnknownHwAccess(address, value, 8, true);
         break;
   }
   
   palmReg[address] = value;
}

void setHwRegister16(unsigned int address, unsigned int value){
   switch(address){
         
      default:
         printUnknownHwAccess(address, value, 16, true);
         break;
   }
   
   palmReg[address] = value >> 8;
   palmReg[address + 1] = value & 0xFF;
}

void setHwRegister32(unsigned int address, unsigned int value){
   switch(address){
      
      default:
         printUnknownHwAccess(address, value, 32, true);
         break;
   }
   
   palmReg[address] = value >> 24;
   palmReg[address + 1] = (value >> 16) & 0xFF;
   palmReg[address + 2] = (value >> 8) & 0xFF;
   palmReg[address + 3] = value & 0xFF;
}
