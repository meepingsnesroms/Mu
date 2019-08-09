#include <stdint.h>

#include "../emulator.h"


#define SSCR0 0x0000
#define SSCR1 0x0004
#define SSSR 0x0008
#define SSDR 0x0010


uint32_t pxa260SspSscr0;
uint32_t pxa260SspSscr1;
uint32_t pxa260SspSssr;
uint32_t pxa260SspSsdr;


void pxa260SspReset(void){
   pxa260SspSscr0 = 0x0000;
   pxa260SspSscr1 = 0x0000;
   pxa260SspSssr = 0xF004;
   pxa260SspSsdr = 0x0000;
}

uint32_t pxa260SspReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){

      default:
         debugLog("Unimplimented 32 bit PXA260 SSP register read:0x%04X\n", address);
         return 0x00000000;
   }
}

void pxa260SspWriteWord(uint32_t address, uint32_t value){
   address &= 0xFFFF;

   switch(address){
      case SSCR0:
         pxa260SspSscr0 = value & 0xFFFF;
         return;

      case SSCR1:
         pxa260SspSscr1 = value & 0x3FFF;
         //update interrupts here
         return;

      default:
         debugLog("Unimplimented 32 bit PXA260 SSP register write:0x%04X, value:0x%08X\n", address, value);
         return;
   }
}
