#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "portability.h"


#define PDIUSBD12_TRANSFER_BUFFER_SIZE 130


static uint8_t pdiUsbD12TransferBuffer[PDIUSBD12_TRANSFER_BUFFER_SIZE];
static uint8_t pdiUsbD12TransferIndex;


void pdiUsbD12Reset(){
   memset(pdiUsbD12TransferBuffer, 0x00, PDIUSBD12_TRANSFER_BUFFER_SIZE);
   pdiUsbD12TransferIndex = 0;
}

uint64_t pdiUsbD12StateSize(){
   uint64_t size = 0;

   size += PDIUSBD12_TRANSFER_BUFFER_SIZE;
   size += sizeof(uint8_t);

   return size;
}

void pdiUsbD12SaveState(uint8_t* data){
   uint64_t offset = 0;

   memcpy(data + offset, pdiUsbD12TransferBuffer, PDIUSBD12_TRANSFER_BUFFER_SIZE);
   offset += PDIUSBD12_TRANSFER_BUFFER_SIZE;
   writeStateValueUint8(data + offset, pdiUsbD12TransferIndex);
   offset += sizeof(uint8_t);
}

void pdiUsbD12LoadState(uint8_t* data){
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
