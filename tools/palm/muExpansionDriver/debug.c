#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>
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

uint16_t setWatchRegion(uint32_t address, uint32_t size, uint8_t type){
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE), type);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_SRC), address);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_SIZE), size);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_DEBUG_WATCH);
   
   return readArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE));
}

void clearWatchRegion(uint16_t index){
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE), 0);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_SRC), index);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_DEBUG_WATCH);
}

void watchAppCode(const char* appName){
   /*marks app code resources as debug watch areas*/
}
