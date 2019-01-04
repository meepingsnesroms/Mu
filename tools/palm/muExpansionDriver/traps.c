#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "specs/emuFeatureRegisterSpec.h"


/*cant use global variables in this file!!!*/
/*functions in this file are called directly by the Palm OS trap dispatch this means they can be called when the app isnt loaded and the globals are just a random data buffer*/


UInt32 emuPceNativeCall(NativeFuncType* nativeFuncP, void* userDataP){
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_SRC), (uint32_t)nativeFuncP);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_DST), (uint32_t)userDataP);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_ARM_RUN);
   
   return readArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE));
}
