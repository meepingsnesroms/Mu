#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "debug.h"
#include "config.h"
#include "traps.h"
#include "globals.h"
#include "palmOsPriv.h"
#include "palmGlobalDefines.h"
#include "specs/emuFeatureRegisterSpec.h"


static void lockCodeXXXX(void){
   /*prevents the driver from being moved around in RAM while running*/
   uint16_t index = 0;
   
   while(true){
      MemHandle codeResourceHandle = DmGetResource('code', index);
      
      if(codeResourceHandle)
         MemHandleLock(codeResourceHandle);
      else
         break;
      
      index++;
   }
}

static void installDebugHandlers(void){
   void* sysUnimplementedAddress = SysGetTrapAddress(sysTrapSysUnimplemented);
   uint16_t index;
   
   /*patch all the debug handlers to go to the emu debug console*/
   for(index = sysTrapBase; index < sysTrapLastTrapNumber; index++){
      void* trapAddress = SysGetTrapAddress(index);
      
      /*eventually should remove the NULL check too, the trap list should have no NULL values*/
      
      if(trapAddress == sysUnimplementedAddress || trapAddress == NULL)
         TrapTablePtr[index - sysTrapBase] = (void*)emuSysUnimplemented;
   }
   TrapTablePtr[sysTrapErrDisplayFileLineMsg - sysTrapBase] = (void*)emuErrDisplayFileLineMsg;
}

void initBoot(uint32_t* configFile){
   if(configFile[DRIVER_ENABLED] && !configFile[SAFE_MODE]){
      LocalID thisApp;
      DmOpenRef thisAppRef;
      Err error;
      uint32_t heapFree;
      uint32_t heapBiggestBlock;
      uint32_t enabledFeatures = readArbitraryMemory32(EMU_REG_ADDR(EMU_INFO));
      
      /*skip the next boot and flush vars to the config file, that way if it crashes it will still boot again*/
      configFile[SAFE_MODE] = true;
      writeConfigFile(configFile);
      
      debugLog("OS booting!\n");
      
      debugLog("RAM size:%ld bytes\n", MemHeapSize(0));
      error = MemHeapFreeBytes(0, &heapFree, &heapBiggestBlock);
      if(!error)
         debugLog("RAM free:%ld, RAM biggest block:%ld bytes\n", heapFree, heapBiggestBlock);
      else
         debugLog("RAM free:check failed, RAM biggest block:check failed\n");
      
      debugLog("Storage size:%ld bytes\n", MemHeapSize(1));
      error = MemHeapFreeBytes(1, &heapFree, &heapBiggestBlock);
      if(!error)
         debugLog("Storage free:%ld bytes, Storage biggest block:%ld bytes\n", heapFree, heapBiggestBlock);
      else
         debugLog("Storage free:check failed, Storage biggest block:check failed\n");

      /*since this is running at boot time the apps own database isnt loaded and it needs to be opened*/
      thisApp = DmFindDatabase(0, APP_NAME);
      if(!thisApp)
         debugLog(APP_NAME " not installed!\n");
      
      thisAppRef = DmOpenDatabase(0, thisApp, dmModeReadOnly | dmModeLeaveOpen);
      if(!thisAppRef)
         debugLog("Cant open " APP_NAME "!\n");
      
      /*prevents the code being moved out from underneath its function pointers(in the API trap table), causing crashes*/
      lockCodeXXXX();
         
      if(enabledFeatures & FEATURE_DEBUG)
         installDebugHandlers();
      
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE), configFile[BOOT_CPU_SPEED]);
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_SET_CPU_SPEED);
      
      /*boot succeeded, clear flag*/
      configFile[SAFE_MODE] = false;
      writeConfigFile(configFile);
   }
}
