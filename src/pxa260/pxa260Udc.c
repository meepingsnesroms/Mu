#include <stdint.h>

#include "pxa260.h"
#include "../emulator.h"


#define UDCCR 0x0000
//...
#define UDCCS0 0x0010
//...
#define UICR0 0x0050
#define UICR1 0x0054
//...
#define UFNHR 0x0060


uint8_t pxa260UdcUdccr;
uint8_t pxa260UdcUdccs0;
uint8_t pxa260UdcUicr0;
uint8_t pxa260UdcUicr1;
uint8_t pxa260UdcUfnhr;


void pxa260UdcReset(void){
   pxa260UdcUdccr = 0xA0;
   //...
   pxa260UdcUdccs0 = 0x00;
   //...
   pxa260UdcUicr0 = 0xFF;
   pxa260UdcUicr1 = 0xFF;
   //...
   pxa260UdcUfnhr = 0x40;
}

uint32_t pxa260UdcReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){
      case UDCCR:
         //TODO: is incomplete
         debugLog("PXA260 UDC UDCCR read, PC:0x%08X\n", pxa260GetPc());
         return pxa260UdcUdccr;

      case UDCCS0:
         //TODO: need to | with RECEIVE FIFO NOT EMPTY
         return pxa260UdcUdccs0;

      case UICR0:
         return pxa260UdcUicr0;

      case UICR1:
         return pxa260UdcUicr0;

      case UFNHR:
         return pxa260UdcUfnhr;

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

      case UDCCS0:
         pxa260UdcUdccs0 &= ~(value & 0x91);//clear clear on 1 bits
         pxa260UdcUdccs0 |= value & 0x26;//set set on 1 bits
         //TODO: need to update interrupts
         return;

      case UICR0:
         pxa260UdcUicr0 = value & 0xFF;
         //TODO: need to update interrupts
         return;

      case UICR1:
         pxa260UdcUicr1 = value & 0xFF;
         //TODO: need to update interrupts
         return;

      default:
         debugLog("Unimplimented 32 bit PXA260 UDC register write:0x%04X, value:0x%02X\n", address, value);
         return;
   }
}
