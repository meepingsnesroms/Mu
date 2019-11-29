#include <stdint.h>
#include <stdbool.h>

#include "pxa260.h"
#include "pxa260_IC.h"
#include "pxa260Timing.h"
#include "pxa260I2c.h"
#include "../emulator.h"
#include "../tps65010.h"


#define PXA260_I2C_TRANSFER_DURATION 200

#define IBMR 0x1680
#define IDBR 0x1688
#define ICR 0x1690
#define ISR 0x1698
#define ISAR 0x16A0


uint8_t  pxa260I2cBus;
uint8_t  pxa260I2cBuffer;
uint16_t pxa260I2cIcr;
uint16_t pxa260I2cIsr;
uint8_t  pxa260I2cIsar;


static void pxa260I2cUpdateInterrupt(void){
   if(pxa260I2cIcr & 0x0100 && pxa260I2cIsr & 0x0040 || pxa260I2cIcr & 0x0200 && pxa260I2cIsr & 0x0080)
      pxa260icInt(&pxa260Ic, PXA260_I_I2C, true);
   else
      pxa260icInt(&pxa260Ic, PXA260_I_I2C, false);
}

void pxa260I2cReset(void){
   pxa260I2cBus = I2C_FLOATING_BUS;
   pxa260I2cBuffer = 0x00;
   pxa260I2cIcr = 0x0000;
   pxa260I2cIsr = 0x0000;
   pxa260I2cIsar = 0x00;
}

uint32_t pxa260I2cReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){
      case IBMR:
         debugLog("PXA260 direct I2C bus read\n");
         return pxa260I2cBus & 0x03;

      case IDBR:
         //debugLog("PXA260 I2C IDBR read, PC:0x%08X\n", pxa260GetPc());
         return pxa260I2cBuffer;

      case ICR:
         //debugLog("PXA260 I2C ICR read, PC:0x%08X\n", pxa260GetPc());
         return pxa260I2cIcr;

      case ISR:
         //TODO: not fully implemeted but the rest seems to be slave mode
         //debugLog("PXA260 I2C ISR read, PC:0x%08X\n", pxa260GetPc());
         return pxa260I2cIsr;

      case ISAR:
         return pxa260I2cIsar;

      default:
         debugLog("Unimplemented I2C register read, reg:0x%04X\n", address);
         return 0x00000000;
   }
}

void pxa260I2cWriteWord(uint32_t address, uint32_t value){
   address &= 0xFFFF;

   switch(address){
      case IDBR:
         //debugLog("PXA260 I2C IDBR write:0x%02X, PC:0x%08X\n", value & 0xFF, pxa260GetPc());
         pxa260I2cBuffer = value & 0xFF;
         return;

      case ICR:
         //TODO: this is incomplete, as of 11/20/2019 I dont know whats wrong here, may be fine
         //debugLog("PXA260 I2C ICR write 0x%04X, PC:0x%08X\n", value & 0xFFFF, pxa260GetPc());

         if(!(pxa260I2cIcr & 0x0008) && value & 0x0040){
            //I2C unit enabled and not transfering right now, its ok to start a transfer
            if(value & 0x0001){
               tps65010I2cExchange(I2C_START);

               //add unit busy bit
               pxa260I2cIsr |= 0x0004;

               //add read/write bit
               pxa260I2cIsr |= pxa260I2cBuffer & 0x0001;
            }
            if(value & 0x0008){
               //must always send 1 byte to start exchange so if start is true force write
               if(pxa260I2cIsr & 0x0001 && !(value & 0x0001)){
                  //receive
                  uint8_t index;

                  for(index = 0; index < 8; index++){
                     pxa260I2cBuffer <<= 1;
                     pxa260I2cBuffer |= !!(tps65010I2cExchange(I2C_FLOATING_BUS) & I2C_1);
                  }

                  //debugLog("I2C transfer(receive) attempted: 0x%02X\n", pxa260I2cBuffer);

                  pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_I2C_RECEIVE_FULL, PXA260_I2C_TRANSFER_DURATION);
               }
               else{
                  //send
                  uint8_t index;

                  //debugLog("I2C transfer(send) attempted: 0x%02X\n", pxa260I2cBuffer);

                  for(index = 0; index < 8; index++)
                     tps65010I2cExchange((pxa260I2cBuffer & 1 << 7 - index) ? I2C_1 : I2C_0);

                  pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_I2C_TRANSMIT_EMPTY, PXA260_I2C_TRANSFER_DURATION);
               }
            }
            if(value & 0x0002)
               tps65010I2cExchange(I2C_STOP);
         }

         //cant clear current transfer flag
         pxa260I2cIcr = value & 0xFFFF | pxa260I2cIcr & 0x0008;

         return;

      case ISR:{
            //debugLog("PXA260 I2C ISR write :0x%04X\n", value & 0xFFFF);

            //clear all clear on write 1 bits
            pxa260I2cIsr = pxa260I2cIsr & ~(value & 0x07F0);

            pxa260I2cUpdateInterrupt();
         }
         return;

      case ISAR:
         pxa260I2cIsar = value & 0x7F;
         return;

      default:
         debugLog("Unimplemented I2C register write, reg:0x%04X, value:0x%08X\n", address, value);
         return;
   }
}

void pxa260I2cTransmitEmpty(void){
   //clear transfer byte
   pxa260I2cIcr &= 0xFFF7;

   //set transmit empty
   pxa260I2cIsr |= 0x0040;

   //clear read write and unit busy bits if stop was sent
   if(pxa260I2cIcr & 0x0002)
      pxa260I2cIsr &= 0xFFFA;

   pxa260I2cUpdateInterrupt();
   //debugLog("I2C transmit empty triggered\n");
}

void pxa260I2cReceiveFull(void){
   //clear transfer byte
   pxa260I2cIcr &= 0xFFF7;

   //set receive full
   pxa260I2cIsr |= 0x0080;

   //clear read write and unit busy bits if stop was sent
   if(pxa260I2cIcr & 0x0002)
      pxa260I2cIsr &= 0xFFFA;

   pxa260I2cUpdateInterrupt();
   //debugLog("I2C receive full triggered\n");
}
