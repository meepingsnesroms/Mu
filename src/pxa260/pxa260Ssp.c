#include <stdint.h>

#include "pxa260.h"
#include "pxa260_IC.h"
#include "pxa260Timing.h"
#include "../emulator.h"


#define SSCR0 0x0000
#define SSCR1 0x0004
#define SSSR 0x0008
#define SSDR 0x0010


uint32_t pxa260SspSscr0;
uint32_t pxa260SspSscr1;
uint32_t pxa260SspSssr;
uint32_t pxa260SspSsdr;


static void pxa260SspUpdateInterrupt(void){
   debugLog("Unimplimented PXA260 SSP interrupt check\n");
   if(/*TODO*/ false)
      pxa260icInt(&pxa260Ic, PXA260_I_SSP, true);
   else
      pxa260icInt(&pxa260Ic, PXA260_I_SSP, false);
}

void pxa260SspReset(void){
   pxa260SspSscr0 = 0x0000;
   pxa260SspSscr1 = 0x0000;
   pxa260SspSssr = 0xF004;
   pxa260SspSsdr = 0x0000;
}

uint32_t pxa260SspReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){
      case SSCR0:
         return pxa260SspSscr0;

      case SSCR1:
         return pxa260SspSscr1;

      case SSSR:
         return pxa260SspSssr;//TODO: need to return FIFO state too

      case SSDR:
          //TODO:SSP SPI seems to exchange on write(of constantly?) instead of having a trigger exchange bit in the status register
         debugLog("Unimplimented PXA260 SSP transfer(read)\n");
         return pxa260SspSsdr;

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
         pxa260SspUpdateInterrupt();
         return;

      case SSSR:
         pxa260SspSssr = pxa260SspSssr & 0x00FE | value & 0xFF00;

         //clear RECEIVE FIFO OVERRUN
         if(value & 0x0080)
            pxa260SspSssr &= 0xFF7F;

         pxa260SspUpdateInterrupt();
         return;

      case SSDR:
         //TODO:SSP SPI seems to exchange on write(of constantly?) instead of having a trigger exchange bit in the status register
         debugLog("Unimplimented PXA260 SSP transfer(write):0x%04X\n", value);
         pxa260SspSsdr = value & 0xFFFF;
         return;

      default:
         debugLog("Unimplimented 32 bit PXA260 SSP register write:0x%04X, value:0x%08X\n", address, value);
         return;
   }
}
