#include <stdint.h>
#include <stdbool.h>

#include "armDefines.h"


bool getGpio(uint32_t pin){
   if(pin >= 85)
      return false;
   
   return !!(PXA260_REG(PXA260_GPIO_BASE, GPLR0 + pin / 32 * 4) &  1 << pin % 32);
}

void setGpio(uint32_t pin, bool value){
   if(pin >= 85)
      return;
   
   if(value)
      PXA260_REG(PXA260_GPIO_BASE, GPSR0 + pin / 32 * 4) |= 1 << pin % 32;
   else
      PXA260_REG(PXA260_GPIO_BASE, GPCR0 + pin / 32 * 4) |= 1 << pin % 32;
}
