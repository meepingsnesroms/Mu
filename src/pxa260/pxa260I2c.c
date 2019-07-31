#include <stdint.h>

#include "../emulator.h"
#include "../tps65010.h"
#include "pxa260.h"
#include "pxa260_IC.h"
#include "pxa260I2c.h"


#define IBMR 0x1680
#define IDBR 0x1688
#define ICR 0x1690
#define ISR 0x1698
#define ISAR 0x16A0


uint8_t  pxa260I2cBus;
uint8_t  pxa260I2cBuffer;
uint16_t pxa260I2cIcr;
uint16_t pxa260I2cIsr;
uint16_t pxa260I2cIsar;


void pxa260I2cReset(void){
   pxa260I2cBus = I2C_FLOATING_BUS;
   pxa260I2cBuffer = 0x00;
   pxa260I2cIcr = 0x0000;
   pxa260I2cIsr = 0x0000;
   pxa260I2cIsar = 0x0000;
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

         pxa260I2cIcr = value & 0xFFFF;//this is wrong

         if(value & 0x0001)
            tps65010I2cExchange(I2C_START);
         if(value & 0x0008){
            uint8_t index;

            for(index = 0; index < 8; index++){
               uint8_t receiveValue = tps65010I2cExchange((pxa260I2cBuffer & 1 << 7 - index) ? I2C_1 : I2C_0);

               if(receiveValue != I2C_FLOATING_BUS){
                  pxa260I2cBuffer <<= 1;
                  pxa260I2cBuffer |= receiveValue == I2C_1;
                  pxa260I2cBus = receiveValue;
               }
            }
         }
         if(value & 0x0002)
            tps65010I2cExchange(I2C_STOP);

         //TODO: run the CPU 200 opcodes here
         //just storing the trigger code here untill delays work

         if(value & 0x0100){
            pxa260I2cIsr |= 0x0040;
            pxa260icInt(&pxa260Ic, PXA260_I_I2C, true);
         }
         return;

      case ISR:
         //TODO: clear bits when written with 1s
         if(value & 0x0080){
            //IDBR RECEIVE FULL
            //TODO: interrupt stuff
         }

         if(value & 0x0040){
            //IDBR TRANSMIT EMPTY
            //TODO: interrupt stuff
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
