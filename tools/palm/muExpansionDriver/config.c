#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "config.h"
#include "debug.h"


static void getConfigDefaults(uint32_t* configFile){
   debugLog("No config file, setting default config.\n");
   
   configFile[USER_WARNING_GIVEN] = false;
   configFile[DRIVER_ENABLED] = false;
   configFile[SAFE_MODE] = false;
   configFile[ARM_STACK_SIZE] = 0x1000;
   configFile[LCD_WIDTH] = 160;
   configFile[LCD_HEIGHT] = 220;
   configFile[EXTRA_RAM_MB_DYNAMIC_HEAP] = 10;
   configFile[BOOT_CPU_SPEED] = 800;
}

void readConfigFile(uint32_t* configFile){
   DmOpenRef configDb;
   
   configDb = DmOpenDatabaseByTypeCreator('EMUC', APP_CREATOR, dmModeReadWrite);
   
   if(!configDb){
      /*config doesnt exist, get defaults*/
      getConfigDefaults(configFile);
   }
   else{
      /*get config data*/
      MemHandle configHandle;
      uint32_t* dmConfigFile;
      
      configHandle = DmGetResource('CONF', 0);
      if(!configHandle)
         debugLog("Cant open DB resource!\n");
      
      dmConfigFile = MemHandleLock(configHandle);
      if(!dmConfigFile)
         debugLog("Cant lock DB handle!\n");
      
      MemMove(configFile, dmConfigFile, CONFIG_FILE_ENTRIES * sizeof(uint32_t));
      
      MemHandleUnlock(configHandle);
      DmReleaseResource(configHandle);
      DmCloseDatabase(configDb);
   }
}

void writeConfigFile(uint32_t* configFile){
   DmOpenRef configDb;
   MemHandle configHandle;
   uint32_t* dmConfigFile;
   Err error;
   
   configDb = DmOpenDatabaseByTypeCreator('EMUC', APP_CREATOR, dmModeReadWrite);
   
   /*create db and set defaults if config doesnt exist*/
   if(!configDb){
      error = DmCreateDatabase(0/*cardNo*/, "Emu Config", APP_CREATOR, 'EMUC', true);
      if(error != errNone)
         debugLog("Tried to create DB, err:%d\n", error);
      
      configDb = DmOpenDatabaseByTypeCreator('EMUC', APP_CREATOR, dmModeReadWrite);
      if(!configDb)
         debugLog("Cant find created DB!\n");
      
      configHandle = DmNewResource(configDb, 'CONF', 0, CONFIG_FILE_ENTRIES * sizeof(uint32_t));
      if(!configHandle)
         debugLog("Cant open DB resource!\n");
   }
   else{
      configHandle = DmGetResource('CONF', 0);
      if(!configHandle)
         debugLog("Cant open DB resource!\n");
   }
   
   dmConfigFile = MemHandleLock(configHandle);
   if(!dmConfigFile)
      debugLog("Cant lock DB handle!\n");
   
   /*must use DmWrite to write to databases or a write protect violation may trigger*/
   error = DmWrite(dmConfigFile, 0, configFile, CONFIG_FILE_ENTRIES * sizeof(uint32_t));
   if(error != errNone)
      debugLog("Coludnt write config file, err:%d\n", error);
   
   MemHandleUnlock(configHandle);
   DmReleaseResource(configHandle);
   DmCloseDatabase(configDb);
}
