#include <stdint.h>

#include "../emulator.h"


void pxa260UdcReset(void){

}

uint32_t pxa260UdcReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){

      default:
         debugLog("Unimplimented 32 bit PXA260 UDC register read:0x%04X\n", address);
         return 0x00000000;
   }
}

void pxa260UdcWriteWord(uint32_t address, uint32_t value){
   address &= 0xFFFF;

   switch(address){

      default:
         debugLog("Unimplimented 32 bit PXA260 UDC register write:0x%04X, value:0x%02X\n", address, value);
         return;
   }
}
