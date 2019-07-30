#include <stdint.h>

#include "../emulator.h"


uint8_t pxa260I2cBus;


uint32_t pxa260I2cReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){
      default:
         debugLog("Unimplemented I2C register read, reg:0x%04X\n", address);
         return 0x00000000;
   }
}

void pxa260I2cWriteWord(uint32_t address, uint32_t value){
   address &= 0xFFFF;

   switch(address){

      default:
         debugLog("Unimplemented I2C register write, reg:0x%04X, value:0x%08X\n", address, value);
         return;
   }
}
