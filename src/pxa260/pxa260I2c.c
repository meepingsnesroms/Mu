#include <stdint.h>

#include "../emulator.h"
#include "../tps65010.h"
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
         //TODO: transfer stuff here

         //some of the bits have special behavior
         //pxa260I2cIcr = value & 0xFFFF;
         return;

      case ISR:
         //TODO: clear bits when written with 1s
         return;

      case ISAR:
         pxa260I2cIsar = value & 0x7F;
         return;

      default:
         debugLog("Unimplemented I2C register write, reg:0x%04X, value:0x%08X\n", address, value);
         return;
   }
}
