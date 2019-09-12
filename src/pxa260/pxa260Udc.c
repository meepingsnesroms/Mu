#include <stdint.h>

#include "pxa260.h"
#include "../emulator.h"


#define UDCCR 0x0000
//...
#define UDCCS0 0x0010
//...
#define UICR0 0x0050
#define UICR1 0x0054
#define USIR0 0x0058
#define USIR1 0x005C
//...
#define UFNHR 0x0060


uint8_t pxa260UdcUdccr;
uint8_t pxa260UdcUdccs0;
uint8_t pxa260UdcUicr0;
uint8_t pxa260UdcUicr1;
uint8_t pxa260UdcUsir0;
uint8_t pxa260UdcUsir1;
uint8_t pxa260UdcUfnhr;


void pxa260UdcReset(void){
   pxa260UdcUdccr = 0xA0;
   //...
   pxa260UdcUdccs0 = 0x00;
   //...
   pxa260UdcUicr0 = 0xFF;
   pxa260UdcUicr1 = 0xFF;
   pxa260UdcUsir0 = 0x00;
   pxa260UdcUsir1 = 0x00;
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
         debugLog("PXA260 UDC UDCCS0 read, PC:0x%08X\n", pxa260GetPc());
         return pxa260UdcUdccs0;

      case UICR0:
         debugLog("PXA260 UDC UICR0 read, PC:0x%08X\n", pxa260GetPc());
         return pxa260UdcUicr0;

      case UICR1:
         debugLog("PXA260 UDC UICR1 read, PC:0x%08X\n", pxa260GetPc());
         return pxa260UdcUicr1;

      case USIR0:
         debugLog("PXA260 UDC USIR0 read, PC:0x%08X\n", pxa260GetPc());
         return pxa260UdcUsir0;

      case USIR1:
         debugLog("PXA260 UDC USIR1 read, PC:0x%08X\n", pxa260GetPc());
         return pxa260UdcUsir1;

      case UFNHR:
         debugLog("PXA260 UDC UFNHR read, PC:0x%08X\n", pxa260GetPc());
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
         debugLog("PXA260 UDC UDCCR write:0x%02X\n", value & 0xFF);
         if(value & 0x01){
            if(value & 0x04)
               debugLog("PXA260 UDC DEVICE RESUME SET\n");

            pxa260UdcUdccr = pxa260UdcUdccr & 0x5E | value & 0xA1;//write normal bits
            pxa260UdcUdccr &= ~(value & 0x58);//clear clear on 1 bits
            pxa260UdcUdccr |= value & 0x04;//set set on 1 bits
         }
         else{
            debugLog("PXA260 UDC reset\n");
            pxa260UdcReset();
         }
         //TODO: need to update interrupts
         return;

      case UDCCS0:
         debugLog("PXA260 UDC UDCCS0 write:0x%02X\n", value & 0xFF);
         pxa260UdcUdccs0 &= ~(value & 0x91);//clear clear on 1 bits
         pxa260UdcUdccs0 |= value & 0x26;//set set on 1 bits
         //TODO: need to update interrupts
         return;

      case UICR0:
         debugLog("PXA260 UDC UICR0 write:0x%02X\n", value & 0xFF);
         pxa260UdcUicr0 = value & 0xFF;
         //TODO: need to update interrupts
         return;

      case UICR1:
         debugLog("PXA260 UDC UICR1 write:0x%02X\n", value & 0xFF);
         pxa260UdcUicr1 = value & 0xFF;
         //TODO: need to update interrupts
         return;

      case USIR0:
         debugLog("PXA260 UDC USIR0 write:0x%02X\n", value & 0xFF);
         pxa260UdcUsir0 &= ~(value & 0xFF);
         //TODO: need to update interrupts
         return;

      case USIR1:
         debugLog("PXA260 UDC USIR1 write:0x%02X\n", value & 0xFF);
         pxa260UdcUicr1 &= ~(value & 0xFF);
         //TODO: need to update interrupts
         return;

      case UFNHR:
         debugLog("PXA260 UDC UFNHR write:0x%02X\n", value & 0xFF);
         pxa260UdcUfnhr = pxa260UdcUfnhr & 0xBF | value & 0x40;//write normal bits
         pxa260UdcUfnhr &= ~(value & 0xB8);//clear clear on 1 bits
         //TODO: need to update interrupts
         return;

      default:
         debugLog("Unimplimented 32 bit PXA260 UDC register write:0x%04X, value:0x%02X\n", address, value);
         return;
   }
}
