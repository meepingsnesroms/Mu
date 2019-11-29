#include <stdint.h>

#include "emulator.h"


void w86l488Reset(void){
   
}

uint32_t w86l488StateSize(void){
   uint32_t size = 0;

   return size;
}

void w86l488SaveState(uint8_t* data){
   uint32_t offset = 0;

}

void w86l488LoadState(uint8_t* data){
   uint32_t offset = 0;

}

uint16_t w86l488Read16(uint8_t address){
   debugLog("Unimplemented T3 SD chip read at address 0x%02X\n", address);
   return 0x0000;
}

void w86l488Write16(uint8_t address, uint16_t value){
   debugLog("Unimplemented T3 SD chip write at address 0x%02X, value:0x%04X\n", address, value);
}
