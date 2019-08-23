#include <stdint.h>
#include <stdbool.h>

#include "../emulator.h"
#include "../tps65010.h"
#include "pxa260.h"
#include "pxa260_IC.h"
#include "pxa260Timing.h"
#include "pxa260I2c.h"


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
bool     pxa260I2cUnitBusy;


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
   pxa260I2cUnitBusy = false;
}

uint32_t pxa260I2cReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){
      case IBMR:
         return pxa260I2cBus & 0x03;

      case IDBR:
         return pxa260I2cBuffer;

      case ICR:
         return pxa260I2cIcr;

      case ISR:
         debugLog("I2C ISR is currently unimplemented\n");
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
         pxa260I2cBuffer = value & 0xFF;
         return;

      case ICR:
         //TODO: this is incomplete

         pxa260I2cIcr = value & 0xFFFF;

         if(value & 0x0001)
            tps65010I2cExchange(I2C_START);
         if(value & 0x0008){
            if(pxa260I2cIsr & 0x0001){
               //receive
               uint8_t index;

               debugLog("I2C transfer(receive) attempted\n");

               for(index = 0; index < 8; index++){
                  pxa260I2cBuffer <<= 1;
                  pxa260I2cBuffer |= !!(tps65010I2cExchange(I2C_FLOATING_BUS) & I2C_1);
               }

               pxa260I2cUnitBusy = true;
               pxa260TimingQueueEvent(PXA260_I2C_TRANSFER_DURATION, PXA260_TIMING_CALLBACK_I2C_RECEIVE_FULL);
            }
            else{
               //send
               uint8_t index;

               debugLog("I2C transfer(send) attempted\n");

               for(index = 0; index < 8; index++)
                  tps65010I2cExchange((pxa260I2cBuffer & 1 << 7 - index) ? I2C_1 : I2C_0);

               pxa260I2cUnitBusy = true;
               pxa260TimingQueueEvent(PXA260_I2C_TRANSFER_DURATION, PXA260_TIMING_CALLBACK_I2C_TRANSMIT_EMPTY);
            }
         }
         if(value & 0x0002){
            tps65010I2cExchange(I2C_STOP);

            //clear read/write bit
            pxa260I2cIsr &= 0xFFFE;
         }
         return;

      case ISR:{
            //clear IDBR RECEIVE FULL
            if(value & 0x0080)
               pxa260I2cIsr &= 0xFF7F;

            //clear IDBR TRANSMIT EMPTY
            if(value & 0x0040)
               pxa260I2cIsr &= 0xFFBF;

            //unit busy
            pxa260I2cIsr |= pxa260I2cUnitBusy << 2;

            //read write setting
            pxa260I2cIsr = pxa260I2cIsr & 0xFFFE | value & 0x0001;

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
   pxa260I2cIcr &= 0xFFF7;
   pxa260I2cIsr |= 0x0040;
   pxa260I2cUnitBusy = false;
   pxa260I2cUpdateInterrupt();
   debugLog("I2C transmit empty triggered\n");
}

void pxa260I2cReceiveFull(void){
   pxa260I2cIcr &= 0xFFF7;
   pxa260I2cIsr |= 0x0080;
   pxa260I2cUnitBusy = false;
   pxa260I2cUpdateInterrupt();
   debugLog("I2C receive full triggered\n");
}
