#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"


void pdiUsbD12Reset(){

}

uint64_t pdiUsbD12StateSize(){
   uint64_t size = 0;

   return size;
}

void pdiUsbD12SaveState(){
   uint64_t offset = 0;

}

void pdiUsbD12LoadState(){
   uint64_t offset = 0;

}

uint8_t pdiUsbD12GetRegister(bool address){
   //just log all USB accesses for now
   debugLog("USB read, address:0x%01X\n", address);

   return 0x00;
}

void pdiUsbD12SetRegister(bool address, uint8_t value){
   //just log all USB accesses for now
   debugLog("USB write, address:0x%01X, value:0x%02X\n", address, value);
}
