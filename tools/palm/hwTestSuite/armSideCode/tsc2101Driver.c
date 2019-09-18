#include <stdint.h>

#include "armDefines.h"


uint16_t tsc2101Read(uint8_t address){
   uint32_t oldInts;
   uint16_t spiReturn;
   uint16_t value;

   oldInts = disableInts();

   //TODO: setup SPI params
   //TODO: set chip select low

   PXA260_REG(PXA260_SSP_BASE, SSDR) = 0x8000 | address << 5;
   PXA260_REG(PXA260_SSP_BASE, SSDR) = 0x0000;

   //wait for transfer to finish
   while(PXA260_REG(PXA260_SSP_BASE, SSSR) & 0x0F10);

   spiReturn = PXA260_REG(PXA260_SSP_BASE, SSDR);
   spiReturn = PXA260_REG(PXA260_SSP_BASE, SSDR);

   //TODO: set chip select high

   enableInts(oldInts);
   return spiReturn;
}

void tsc2101Write(uint8_t address, uint16_t value){

}
