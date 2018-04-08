#include <PalmOS.h>
#include <stdint.h>
#include "emuFeatureRegistersSpec.h"

Boolean isEmulator(){
   /*return (readArbitraryMemory32(EMU_REGISTER_BASE | EMU_INFO) & FEATURE_EMU_HONEST) != 0;*/
   return false;
}

Boolean isEmulatorFeatureEnabled(uint32_t feature){
   if(isEmulator())
      return (readArbitraryMemory32(EMU_REGISTER_BASE | EMU_INFO) & feature) != 0;
   return false;
}
