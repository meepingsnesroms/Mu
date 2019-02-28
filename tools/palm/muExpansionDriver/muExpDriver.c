#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "debug.h"
#include "config.h"
#include "gui.h"
#include "patcher.h"


static void setConfigDefaults(uint32_t* configFile){
   debugLog("First load, setting default config.\n");
   
   configFile[USER_WARNING_GIVEN] = false;
   configFile[ARM_STACK_SIZE] = 0x4000;
   configFile[LCD_WIDTH] = 320;/*160*/
   configFile[LCD_HEIGHT] = 440;/*220*/
   configFile[BOOT_CPU_SPEED] = 100;
   configFile[EXTRA_RAM_MB_DYNAMIC_HEAP] = 10;
}

UInt32 PilotMain(UInt16 cmd, MemPtr cmdBPB, UInt16 launchFlags){
   DmOpenRef configDb;
   MemHandle configHandle;
   uint32_t* configFile;
   Err error;
   
   configDb = DmOpenDatabaseByTypeCreator('EMUC', 'GuiC', dmModeReadWrite);
   
   /*create db and set defaults if config doesnt exist*/
   if(!configDb){
      error = DmCreateDatabase(0/*cardNo*/, "Emu Config", 'GuiC', 'EMUC', true);
      
      debugLog("Tried to create db, err:%d\n", error);
      
      configDb = DmOpenDatabaseByTypeCreator('EMUC', 'GuiC', dmModeReadWrite);
      
      if(!configDb)
         debugLog("Cant find created db!\n");
      
      configHandle = DmNewResource(configDb, 'CONF', 0, CONFIG_FILE_ENTRIES * sizeof(uint32_t));
      
      if(!configHandle)
         debugLog("Cant open db resource!\n");
      
      configFile = MemHandleLock(configHandle);
      setConfigDefaults(configFile);
   }
   else{
      configHandle = DmGetResource('CONF', 0);
      
      if(!configHandle)
         debugLog("Cant open db resource!\n");
      
      configFile = MemHandleLock(configHandle);
   }
   
   if(cmd == sysAppLaunchCmdNormalLaunch)
      showGui(configFile);
   else if(cmd == sysAppLaunchCmdSystemReset)
      initBoot(configFile);
   
   MemHandleUnlock(configHandle);
   DmReleaseResource(configHandle);
   DmCloseDatabase(configDb);
   
   return 0;
}
