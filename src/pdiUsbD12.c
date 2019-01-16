#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "portability.h"
#include "specs/pdiUsbD12CommandSpec.h"


//this is only emulating the commands and USB transfer, the internals of the chip are not documented so any invalid behavior may not match that of the original chip
//running a new command before finishing the previous one will result in corruption


#define PDIUSBD12_CMD_NONE 0xFF
#define PDIUSBD12_TRANSFER_BUFFER_SIZE 131

enum{
   PDIUSBD12_FIFO_TO_CPU = 0,
   PDIUSBD12_FIFO_FROM_CPU,
   PDIUSBD12_FIFO_TO_USB,
   PDIUSBD12_FIFO_FROM_USB,
   PDIUSBD12_FIFO_TOTAL_FIFOS
};


static uint8_t  pdiUsbD12Command;
static uint64_t pdiUsbD12CommandState;
static uint8_t  pdiUsbD12FifoBuffer[PDIUSBD12_TRANSFER_BUFFER_SIZE * PDIUSBD12_FIFO_TOTAL_FIFOS];
static uint16_t pdiUsbD12ReadPosition[PDIUSBD12_FIFO_TOTAL_FIFOS];
static uint16_t pdiUsbD12WritePosition[PDIUSBD12_FIFO_TOTAL_FIFOS];


static uint16_t pdiUsbD12FifoEntrys(uint8_t index){
   //check for wraparound
   if(pdiUsbD12WritePosition[index] < pdiUsbD12ReadPosition[index])
      return pdiUsbD12WritePosition[index] + PDIUSBD12_TRANSFER_BUFFER_SIZE - pdiUsbD12ReadPosition[index];
   return pdiUsbD12WritePosition[index] - pdiUsbD12ReadPosition[index];
}

static uint8_t pdiUsbD12FifoRead(uint8_t index){
   if(pdiUsbD12FifoEntrys(index) > 0)
      pdiUsbD12ReadPosition[index] = (pdiUsbD12ReadPosition[index] + 1) % PDIUSBD12_TRANSFER_BUFFER_SIZE;
   return pdiUsbD12FifoBuffer[PDIUSBD12_TRANSFER_BUFFER_SIZE * index + pdiUsbD12ReadPosition[index]];
}

static void pdiUsbD12FifoWrite(uint8_t index, uint8_t value){
   if(pdiUsbD12FifoEntrys(index) < PDIUSBD12_TRANSFER_BUFFER_SIZE - 1){
      pdiUsbD12FifoBuffer[PDIUSBD12_TRANSFER_BUFFER_SIZE * index + pdiUsbD12WritePosition[index]] = value;
      pdiUsbD12WritePosition[index] = (pdiUsbD12WritePosition[index] + 1) % PDIUSBD12_TRANSFER_BUFFER_SIZE;
   }
}

static void pdiUsbD12FifoFlush(uint8_t index){
   pdiUsbD12ReadPosition[index] = 0;
   pdiUsbD12WritePosition[index] = 0;
}

void pdiUsbD12Reset(void){
   uint8_t index;

   pdiUsbD12Command = PDIUSBD12_CMD_NONE;
   pdiUsbD12CommandState = 0;
   memset(pdiUsbD12FifoBuffer, 0x00, PDIUSBD12_TRANSFER_BUFFER_SIZE * PDIUSBD12_FIFO_TOTAL_FIFOS);
   for(index = 0; index < PDIUSBD12_FIFO_TOTAL_FIFOS; index++){
      pdiUsbD12ReadPosition[index] = 0;
      pdiUsbD12WritePosition[index] = 0;
   }
}

uint64_t pdiUsbD12StateSize(void){
   uint64_t size = 0;

   size += sizeof(uint8_t);
   size += sizeof(uint64_t);
   size += PDIUSBD12_TRANSFER_BUFFER_SIZE * PDIUSBD12_FIFO_TOTAL_FIFOS;
   size += sizeof(uint16_t) * PDIUSBD12_FIFO_TOTAL_FIFOS * 2;

   return size;
}

void pdiUsbD12SaveState(uint8_t* data){
   uint64_t offset = 0;
   uint8_t index;

   writeStateValue8(data + offset, pdiUsbD12Command);
   offset += sizeof(uint8_t);
   writeStateValue64(data + offset, pdiUsbD12CommandState);
   offset += sizeof(uint64_t);
   memcpy(data + offset, pdiUsbD12FifoBuffer, PDIUSBD12_TRANSFER_BUFFER_SIZE * PDIUSBD12_FIFO_TOTAL_FIFOS);
   offset += PDIUSBD12_TRANSFER_BUFFER_SIZE;
   for(index = 0; index < PDIUSBD12_FIFO_TOTAL_FIFOS; index++){
      writeStateValue16(data + offset, pdiUsbD12ReadPosition[index]);
      offset += sizeof(uint16_t);
      writeStateValue16(data + offset, pdiUsbD12WritePosition[index]);
      offset += sizeof(uint16_t);
  }
}

void pdiUsbD12LoadState(uint8_t* data){
   uint64_t offset = 0;
   uint8_t index;

   pdiUsbD12Command = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   pdiUsbD12CommandState = readStateValue64(data + offset);
   offset += sizeof(uint64_t);
   memcpy(pdiUsbD12FifoBuffer, data + offset, PDIUSBD12_TRANSFER_BUFFER_SIZE * PDIUSBD12_FIFO_TOTAL_FIFOS);
   offset += PDIUSBD12_TRANSFER_BUFFER_SIZE;
   for(index = 0; index < PDIUSBD12_FIFO_TOTAL_FIFOS; index++){
      pdiUsbD12ReadPosition[index] = readStateValue16(data + offset);
      offset += sizeof(uint16_t);
      pdiUsbD12WritePosition[index] = readStateValue16(data + offset);
      offset += sizeof(uint16_t);
   }
}

uint8_t pdiUsbD12GetRegister(bool address){
   uint8_t value;

   if(!address){
      //0x0 data
      switch(pdiUsbD12Command){
         case READ_WRITE_BUFFER:
            switch(pdiUsbD12CommandState){
               case 0:
                  pdiUsbD12CommandState++;
                  value = 0xE7;//random byte
                  break;

               case 1:
                  debugLog("Invalid behavior, READ_WRITE_BUFFER read\n");
                  pdiUsbD12CommandState++;
                  value = 0x00;//bytes to read, need to actually implement this
                  break;

               default:
                  value = pdiUsbD12FifoRead(PDIUSBD12_FIFO_FROM_USB);//the actual data
                  break;
            }
            break;

         case READ_INTERRUPT_REGISTER:
            //most functions belong here
            value = pdiUsbD12FifoRead(PDIUSBD12_FIFO_TO_CPU);
            break;

         case PDIUSBD12_CMD_NONE:
            value = 0x00;
            break;

         default:
            debugLog("USB command 0x%02X, read not handeled\n", pdiUsbD12Command);
            value = 0x00;
            break;
      }
   }
   else{
      //0x1 commands
      debugLog("USB read command\n");
      //may just return 0x00(or random byte) or may return current command(not read by Palm OS during boot up)
      value = 0x00;
   }

   return value;
}

void pdiUsbD12SetRegister(bool address, uint8_t value){
   if(!address){
      //0x0 data
      switch(pdiUsbD12Command){
         case READ_WRITE_BUFFER:
            switch(pdiUsbD12CommandState){
               case 0:
                  //random byte
                  pdiUsbD12CommandState++;
                  break;

               case 1:
                  //bytes to write, actually need to implement this
                  debugLog("Invalid behavior, READ_WRITE_BUFFER write\n");
                  pdiUsbD12CommandState++;
                  break;

               default:
                  pdiUsbD12FifoWrite(PDIUSBD12_FIFO_TO_USB, value);
                  break;
            }
            break;

         case PDIUSBD12_CMD_NONE:
            break;

         default:
            debugLog("USB command 0x%02X, write not handeled\n", pdiUsbD12Command);
            break;
      }
   }
   else{
      //0x1 commands
      pdiUsbD12Command = value;
      pdiUsbD12FifoFlush(PDIUSBD12_FIFO_TO_CPU);
      pdiUsbD12FifoFlush(PDIUSBD12_FIFO_FROM_CPU);

      switch(value){
         case READ_INTERRUPT_REGISTER:
            //just reply with no interrupt for now
            pdiUsbD12FifoWrite(PDIUSBD12_FIFO_TO_CPU, 0x00);
            pdiUsbD12FifoWrite(PDIUSBD12_FIFO_TO_CPU, 0x00);
            pdiUsbD12CommandState = 0;
            break;

         default:
            debugLog("USB unknown command, value:0x%02X\n", value);
            break;
      }
   }
}
