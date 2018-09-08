#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "portability.h"
#include "specs/pdiUsbD12Commands.h"


//this is only emulating the commands and USB transfer, the internals of the chip are not documented so any invalid behavior may not match that of the original chip
//running a new command before finishing the previous one will result in corruption


#define PDIUSBD12_TRANSFER_BUFFER_SIZE 130


static uint8_t pdiUsbD12FifoBuffer[PDIUSBD12_TRANSFER_BUFFER_SIZE];
static uint8_t pdiUsbD12ReadIndex;
static uint8_t pdiUsbD12WriteIndex;


static inline uint8_t pdiUsbD12FifoRead(){
   uint8_t value = pdiUsbD12FifoBuffer[pdiUsbD12ReadIndex];
   pdiUsbD12ReadIndex = (pdiUsbD12ReadIndex + 1) % PDIUSBD12_TRANSFER_BUFFER_SIZE;
   return value;
}

static inline void pdiUsbD12FifoWrite(uint8_t value){
   pdiUsbD12FifoBuffer[pdiUsbD12WriteIndex] = value;
   pdiUsbD12WriteIndex = (pdiUsbD12WriteIndex + 1) % PDIUSBD12_TRANSFER_BUFFER_SIZE;
}

static inline void pdiUsbD12FifoStartNewCommand(){
   pdiUsbD12ReadIndex = 0;
   pdiUsbD12WriteIndex = 0;
}

void pdiUsbD12Reset(){
   memset(pdiUsbD12FifoBuffer, 0x00, PDIUSBD12_TRANSFER_BUFFER_SIZE);
   pdiUsbD12ReadIndex = 0;
   pdiUsbD12WriteIndex = 0;
}

uint64_t pdiUsbD12StateSize(){
   uint64_t size = 0;

   size += PDIUSBD12_TRANSFER_BUFFER_SIZE;
   size += sizeof(uint8_t) * 2;

   return size;
}

void pdiUsbD12SaveState(uint8_t* data){
   uint64_t offset = 0;

   memcpy(data + offset, pdiUsbD12FifoBuffer, PDIUSBD12_TRANSFER_BUFFER_SIZE);
   offset += PDIUSBD12_TRANSFER_BUFFER_SIZE;
   writeStateValueUint8(data + offset, pdiUsbD12ReadIndex);
   offset += sizeof(uint8_t);
   writeStateValueUint8(data + offset, pdiUsbD12WriteIndex);
   offset += sizeof(uint8_t);
}

void pdiUsbD12LoadState(uint8_t* data){
   uint64_t offset = 0;

   memcpy(pdiUsbD12FifoBuffer, data + offset, PDIUSBD12_TRANSFER_BUFFER_SIZE);
   offset += PDIUSBD12_TRANSFER_BUFFER_SIZE;
   pdiUsbD12ReadIndex = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
   pdiUsbD12WriteIndex = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
}

uint8_t pdiUsbD12GetRegister(bool address){
   //just log all USB accesses for now
   //debugLog("USB read, address:0x%01X\n", address);

   if(!address){
      //0x0 data
      return pdiUsbD12FifoRead();
   }
   else{
      //0x1 commands
      //may just return 0x00(or random 0xXX) or may return current command(not read by Palm OS during boot up)
      debugLog("USB command readback, response unknown, address:0x%01X\n", address);
   }

   return 0x00;
}

void pdiUsbD12SetRegister(bool address, uint8_t value){
   //just log all USB accesses for now
   //debugLog("USB write, address:0x%01X, value:0x%02X\n", address, value);

   if(!address){
      //0x0 data
      pdiUsbD12FifoWrite(value);
   }
   else{
      //0x1 commands
      pdiUsbD12FifoStartNewCommand();

      switch(value){
         case READ_INTERRUPT_REGISTER:
            //just reply with no interrupt for now
            pdiUsbD12FifoWrite(0x00);
            pdiUsbD12FifoWrite(0x00);
            break;

         default:
            debugLog("USB unknown command, address:0x%01X, value:0x%02X\n", address, value);
            break;
      }
   }
}
