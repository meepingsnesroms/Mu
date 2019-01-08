#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

/*dont include this anywhere else*/
#include "MuExpDriverRsc.h"

#include "debug.h"
#include "traps.h"
#include "armv5.h"
#include "palmGlobalDefines.h"
#include "specs/emuFeatureRegisterSpec.h"


/*config vars*/
#define CONFIG_FILE_SIZE (20 * sizeof(uint32_t))

enum{
   ARM_STACK_SIZE = 0,
   LCD_WIDTH,
   LCD_HEIGHT
};


static void setConfigDefaults(uint32_t* configFile){
   debugLog("First load, setting default config.\n");
   configFile[ARM_STACK_SIZE] = 0x4000;
   configFile[LCD_WIDTH] = 160;
   configFile[LCD_HEIGHT] = 220;
}

static void showGui(uint32_t* configFile){
   debugLog("Attemped to load GUI.\n");
   
   while(true){
      EventType event;
   
      do{
         EvtGetEvent(&event, 1);
         SysHandleEvent(&event);
         if(event.eType == appStopEvent)
            return;
      }
      while(event.eType != nilEvent);
   }
}

static void initBoot(void){
   uint32_t enabledFeatures = readArbitraryMemory32(EMU_REG_ADDR(EMU_INFO));
   
   debugLog("OS booting!\n");
#if 0
   if(enabledFeatures & FEATURE_HYBRID_CPU){
      SysSetTrapAddress(sysTrapPceNativeCall, (void*)emuPceNativeCall);
      
      if(configFile[ARM_STACK_SIZE] > 0)
         armv5SetStack(MemPtrNew(configFile[ARM_STACK_SIZE]), configFile[ARM_STACK_SIZE]);
   }
#endif
}

UInt32 PilotMain(UInt16 cmd, MemPtr cmdBPB, UInt16 launchFlags){
#if 0
   /*use config file*/
   DmOpenRef configDb;
   MemHandle configHandle;
   uint32_t* configFile;
   
   configDb = DmOpenDatabaseByTypeCreator('EMUC', 'GuiC', dmModeReadOnly);
   
   /*create db and set defaults if config doesnt exist*/
   if(!configDb){
      DmCreateDatabase(0/*cardNo*/, "Emu Config", 'GuiC', 'EMUC', true);
      configDb = DmOpenDatabaseByTypeCreator('EMUC', 'GuiC', dmModeReadOnly);
      configHandle = DmNewResource(configDb, 'CONF', 0, CONFIG_FILE_SIZE);
      configFile = MemHandleLock(configHandle);
      setConfigDefaults(configFile);
   }
   else{
      configHandle = DmGet1Resource('CONF', 0);
      configFile = MemHandleLock(configHandle);
   }
   
   if(cmd == sysAppLaunchCmdNormalLaunch)
      showGui(configFile);
   else if(cmd == sysAppLaunchCmdSystemReset)
      initBoot(configFile);
   
   MemHandleUnlock(configHandle);
   DmReleaseResource(configHandle);
   DmCloseDatabase(configDb);
#else
   /*dont use config file*/
   uint32_t* configFile = MemPtrNew(CONFIG_FILE_SIZE);
   
   setConfigDefaults(configFile);
   
   if(cmd == sysAppLaunchCmdNormalLaunch)
      showGui(configFile);
   else if(cmd == sysAppLaunchCmdSystemReset)
      initBoot(configFile);
   
   MemPtrFree(configFile);
#endif
   
   return 0;
}
