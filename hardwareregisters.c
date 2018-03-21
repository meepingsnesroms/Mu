#include <stdio.h>

#include <boolean.h>

//#include "emulator.h"

void printHwAccess(unsigned int address, unsigned int value, unsigned int size, bool isWrite){
   if(isWrite){
      printf("Cpu Wrote %d bits of 0x%08X to 0x%08X.\n", size, value, address);
   }
   else{
      printf("Cpu Read %d bits from 0x%08X.\n", size, address);
   }
}

unsigned int getHwRegister8(unsigned int address){
   printHwAccess(address, 0, 8, false);
   return 0xAA;
}

unsigned int getHwRegister16(unsigned int address){
   printHwAccess(address, 0, 16, false);
   return 0xAAAA;
}

unsigned int getHwRegister32(unsigned int address){
   printHwAccess(address, 0, 32, false);
   return 0xAAAAAAAA;
}

void setHwRegister8(unsigned int address, unsigned int value){
   printHwAccess(address, value, 8, true);
}

void setHwRegister16(unsigned int address, unsigned int value){
   printHwAccess(address, value, 16, true);
}

void setHwRegister32(unsigned int address, unsigned int value){
   printHwAccess(address, value, 8, true);
}
