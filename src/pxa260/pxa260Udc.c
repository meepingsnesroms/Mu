#include <stdint.h>

#include "pxa260.h"
#include "../emulator.h"


#define UDCCR 0x0000
//...
#define UICR0 0x0050


uint8_t pxa260UdcUdccr;
uint8_t pxa260UdcUicr0;


void pxa260UdcReset(void){
   pxa260UdcUdccr = 0xA0;
   //...
   pxa260UdcUicr0 = 0xFF;
}

uint32_t pxa260UdcReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){
      case UDCCR:
         //TODO: is incomplete
         debugLog("Snoot boop, PC:0x%08X\n", pxa260GetPc());
         return pxa260UdcUdccr;

      case UICR0:
         //simple read, no actions needed
         return pxa260UdcUicr0;

      default:
         debugLog("Unimplimented 32 bit PXA260 UDC register read:0x%04X\n", address);
         return 0x00000000;
   }
}

void pxa260UdcWriteWord(uint32_t address, uint32_t value){
   address &= 0xFFFF;

   switch(address){
      case UDCCR:
         //TODO: is incomplete
         debugLog("PXA260 UDC UDCCR write:0x%08X\n", value);
         pxa260UdcUdccr = value & 0xFF;
         return;

      case UICR0:
         pxa260UdcUicr0 = value & 0xFF;
         //TODO: need to update interrupts
         return;

      default:
         debugLog("Unimplimented 32 bit PXA260 UDC register write:0x%04X, value:0x%02X\n", address, value);
         return;
   }
}
