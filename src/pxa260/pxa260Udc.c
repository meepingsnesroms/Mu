#include <stdint.h>

#include "pxa260.h"
#include "pxa260_IC.h"
#include "pxa260Timing.h"
#include "../emulator.h"


/*
USB devices have pull up resistors, USB hosts have pull down resistors, therefore being unplugged does not issue USB reset commands
*/


#define PXA260_UDC_DEVICE_RESUME_DURATION 10

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


static void pxa260UdcUpdateInterrupt(void){
   //USB reset
   if(pxa260UdcUdccr & 0x40 && !(pxa260UdcUdccr & 0x80))
      goto trigger;

   //TODO: need to check other interrupts

   pxa260icInt(&pxa260Ic, PXA260_I_USB, false);
   return;

   trigger:
   pxa260icInt(&pxa260Ic, PXA260_I_USB, true);
   return;
}

void pxa260UdcReset(void){
   pxa260UdcUdccr = 0xA0;// | 0x02;//TODO: never receiving a USB reset, nothing connected
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
         if(value & 0x04 && !(pxa260UdcUdccr & 0x04)){
            debugLog("PXA260 UDC DEVICE RESUME SET\n");
            pxa260UdcUdccr |= 0x04;
            pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_UDC_DEVICE_RESUME_COMPLETE, PXA260_UDC_DEVICE_RESUME_DURATION);
         }

         pxa260UdcUdccr = pxa260UdcUdccr & 0x5E | value & 0xA1;//write normal bits
         pxa260UdcUdccr &= ~(value & 0x58);//clear clear on 1 bits

         pxa260UdcUpdateInterrupt();
         return;

      case UDCCS0:
         debugLog("PXA260 UDC UDCCS0 write:0x%02X\n", value & 0xFF);
         pxa260UdcUdccs0 &= ~(value & 0x91);//clear clear on 1 bits
         pxa260UdcUdccs0 |= value & 0x26;//set set on 1 bits
         pxa260UdcUpdateInterrupt();
         return;

      case UICR0:
         //debugLog("PXA260 UDC UICR0 write:0x%02X\n", value & 0xFF);
         pxa260UdcUicr0 = value & 0xFF;
         //dont need to check interrupts
         //from datasheet: It only blocks future zero to one transitions of the interrupt bit.
         return;

      case UICR1:
         //debugLog("PXA260 UDC UICR1 write:0x%02X\n", value & 0xFF);
         pxa260UdcUicr1 = value & 0xFF;
         //dont need to check interrupts
         //from datasheet: It only blocks future zero to one transitions of the interrupt bit.
         return;

      case USIR0:
         debugLog("PXA260 UDC USIR0 write:0x%02X\n", value & 0xFF);
         pxa260UdcUsir0 &= ~(value & 0xFF);
         pxa260UdcUpdateInterrupt();
         return;

      case USIR1:
         debugLog("PXA260 UDC USIR1 write:0x%02X\n", value & 0xFF);
         pxa260UdcUicr1 &= ~(value & 0xFF);
         pxa260UdcUpdateInterrupt();
         return;

      case UFNHR:
         debugLog("PXA260 UDC UFNHR write:0x%02X\n", value & 0xFF);
         pxa260UdcUfnhr = pxa260UdcUfnhr & 0xBF | value & 0x40;//write normal bits
         pxa260UdcUfnhr &= ~(value & 0xB8);//clear clear on 1 bits
         pxa260UdcUpdateInterrupt();
         return;

      default:
         debugLog("Unimplimented 32 bit PXA260 UDC register write:0x%04X, value:0x%02X\n", address, value);
         return;
   }
}

void pxa260UdcDeviceResumeComplete(void){
   pxa260UdcUdccr &= 0xFB;
}
