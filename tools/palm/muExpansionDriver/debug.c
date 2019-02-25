#include "sdkPatch/PalmOSPatched.h"
#include <stdarg.h>

#include "palmGlobalDefines.h"
#include "specs/emuFeatureRegisterSpec.h"


#ifdef DEBUG
void debugLog(const char* format, ...){
   const char prefix[] = "muExpDriver:";
   char temp[200];
   va_list va;

   va_start(va, format);
   StrCopy(temp, prefix);
   StrVPrintF(temp + StrLen(prefix), format, va);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_SRC), (uint32_t)temp);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_DEBUG_PRINT);
   va_end(va);
}
#endif
