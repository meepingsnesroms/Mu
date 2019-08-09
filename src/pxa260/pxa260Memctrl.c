#include <stdint.h>

#include "../emulator.h"


//this file does nothing execpt manage the MEMCTRL register memory, timings are not emulated


#define MSC1 0x000C


uint32_t pxa260MemctrlRegisters[0x64 / 4];


void pxa260MemctrlReset(void){
   
}

uint32_t pxa260MemctrlReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){

      default:
         debugLog("32 bit PXA260 MEMCTRL register read:0x%04X\n", address);
         if(address / 4 < 0x64)
            return pxa260MemctrlRegisters[address / 4];
         return 0x00000000;
   }
}

void pxa260MemctrlWriteWord(uint32_t address, uint32_t value){
   address &= 0xFFFF;

   switch(address){

      default:
         debugLog("32 bit PXA260 MEMCTRL register write:0x%08X, value:0x%04X\n", address, value);
         if(address / 4 < 0x64)
            pxa260MemctrlRegisters[address / 4] = value;
         return;
   }
}
