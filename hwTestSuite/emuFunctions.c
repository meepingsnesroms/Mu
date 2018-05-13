#include <PalmOS.h>
#include <stdint.h>
#include "testSuite.h"
#include "specs/emuFeatureRegistersSpec.h"

Boolean isEmulator(){
   /*return (readArbitraryMemory32(EMU_REG_ADDR(EMU_INFO)) & FEATURE_EMU_HONEST) != 0;*/
   return false;
}

Boolean isEmulatorFeatureEnabled(uint32_t feature){
   if(isEmulator())
      return (readArbitraryMemory32(EMU_REG_ADDR(EMU_INFO)) & feature) != 0;
   return false;
}
