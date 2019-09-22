#include <stdint.h>

#include "armDefines.h"
#include "armControl.h"


uint16_t tsc2101Read(uint8_t address){
   volatile uint16_t spiReturn;
   uint16_t value;

   //TODO: may need to setup SPI params
   setGpio(24, false);//TSC2101 chip select

   PXA260_REG(PXA260_SSP_BASE, SSDR) = 0x8000 | address << 5;
   PXA260_REG(PXA260_SSP_BASE, SSDR) = 0x0000;

   //wait for transfer to finish
   while(PXA260_REG(PXA260_SSP_BASE, SSSR) & 0x0F10);

   spiReturn = PXA260_REG(PXA260_SSP_BASE, SSDR);
   spiReturn = PXA260_REG(PXA260_SSP_BASE, SSDR);

   setGpio(24, true);//TSC2101 chip select

   return spiReturn;
}

void tsc2101Write(uint8_t address, uint16_t value){
   volatile uint16_t spiReturn;

   //TODO: may need to setup SPI params
   setGpio(24, false);//TSC2101 chip select

   PXA260_REG(PXA260_SSP_BASE, SSDR) = address << 5;
   PXA260_REG(PXA260_SSP_BASE, SSDR) = value;

   //wait for transfer to finish
   while(PXA260_REG(PXA260_SSP_BASE, SSSR) & 0x0F10);

   spiReturn = PXA260_REG(PXA260_SSP_BASE, SSDR);
   spiReturn = PXA260_REG(PXA260_SSP_BASE, SSDR);

   setGpio(24, true);//TSC2101 chip select
}
