#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

/*dont include this anywhere else*/
#include "MuExpDriverRsc.h"

#include "debug.h"
#include "traps.h"
#include "armv5.h"
#include "globals.h"
#include "palmGlobalDefines.h"
#include "specs/emuFeatureRegisterSpec.h"


/*config vars*/
enum{
   USER_WARNING_GIVEN = 0,
   ARM_STACK_SIZE,
   LCD_WIDTH,
   LCD_HEIGHT,
   EXTRA_RAM_MB_DYNAMIC_HEAP,
   BOOT_CPU_SPEED,
   /*add new entries above*/
   CONFIG_FILE_ENTRIES
};

static void install128mbRam(uint8_t mbDynamicHeap){
   /*these functions are called by HwrInit, which can be called directly by apps*/
   /*PrvGetRAMSize_10002C5E, small ROM*/
   /*PrvGetRAMSize_1008442E, big ROM*/
   /*HwrPreDebugInit_10002B06, small ROM*/
   /*HwrPreDebugInit_100842DC, big ROM, sets up the default RAM card*/
   /*HwrCalcDynamicHeapSize_10005D10, small ROM*/
   /*HwrCalcDynamicHeapSize_10083B54, big ROM, sets up the heap size*/
   
   
   /*installs a new virtual card with the extra RAM(128mb - 16mb)*/
   
   /*
   Err         MemCardFormat(UInt16 cardNo, const Char *cardNameP, 
                             const Char *manufNameP, const Char *ramStoreNameP)
   SYS_TRAP(sysTrapMemCardFormat);
   */
   
   /*
   Err         MemStoreSetInfo(UInt16 cardNo, UInt16 storeNumber,
                               UInt16 *versionP, UInt16 *flagsP,  Char *nameP, 
                               UInt32 *crDateP, UInt32 *bckUpDateP, 
                               UInt32 *heapListOffsetP, UInt32 *initCodeOffset1P,
                               UInt32 *initCodeOffset2P, LocalID*   databaseDirIDP)
   SYS_TRAP(sysTrapMemStoreSetInfo);
   */
}

static void setProperDeviceId(uint16_t screenWidth, uint16_t screenHeight, Boolean hasArmCpu, Boolean hasDpad){
   uint32_t osVer;
   uint32_t companyId;
   uint32_t deviceId;
   uint32_t halId;
   
   if(hasArmCpu){
      /*get matching OS 5 device*/
      switch((uint32_t)screenWidth << 16 | screenHeight){
         case (uint32_t)480 << 16 | 320:
            /*Tapwave Zodiac 2*/
            osVer = sysMakeROMVersion(5, 4, 0, sysROMStageRelease, 0);/*needs to be verifyed!!!*/
            companyId = 'Tpwv';
            deviceId = 'Rdog';
            halId = 'MX1a';
            break;
            
         case (uint32_t)320 << 16 | 440:
            /*Tungsten E2*/
            osVer = sysMakeROMVersion(5, 4, 0, sysROMStageRelease, 0);/*needs to be verifyed!!!*/
            companyId = 'Palm';
            deviceId = 'Zir4';
            halId = 'hspr';
            break;
         
         default:
            /*use Palm Z22, has the same resolution as the Palm m515*/
            osVer = sysMakeROMVersion(5, 4, 0, sysROMStageRelease, 0);/*needs to be verifyed!!!*/
            companyId = 'Palm';
            deviceId = 'D051';
            halId = 'S051';
            break;
      }
   }
   else{
      /*get matching OS 4 device*/
      switch((uint32_t)screenWidth << 16 | screenHeight){
         case (uint32_t)320 << 16 | 440:
            /*Tungsten E running OS 4*/
            /*there are no 320x320 + digitizer, 16 bit color, OS 4 devices*/
            /*this is not a real device, using it may cause issues*/
            FtrGet(sysFileCSystem, sysFtrNumROMVersion, &osVer);
            companyId = 'Palm';
            deviceId = 'Cct1';
            halId = 'Ect1';
            break;
            
         case (uint32_t)320 << 16 | 320:
            /*Tungsten W*/
            osVer = sysMakeROMVersion(4, 1, 0, sysROMStageRelease, 0);/*needs to be verifyed!!!*/
            companyId = 'palm';
            deviceId = 'atc1';
            halId = 'atlc';
            break;
            
         default:
            /*use device defaults*/
            FtrGet(sysFileCSystem, sysFtrNumROMVersion, &osVer);
            FtrGet(sysFileCSystem, sysFtrNumOEMCompanyID, &companyId);
            FtrGet(sysFileCSystem, sysFtrNumOEMDeviceID, &deviceId);
            FtrGet(sysFileCSystem, sysFtrNumOEMHALID, &halId);
            break;
      }
   }
   
   /*enable dpad API*/
   if(hasDpad)
      FtrSet(navFtrCreator, navFtrVersion, navVersion);
   
   /*no fixed digitizer, enable digitizer APIs*/
   if(screenHeight != 220 && screenHeight != 440)
      FtrSet(pinCreator, pinFtrAPIVersion, pinAPIVersion1_1);
   
   /*later Sony specific stuff may go here to allow running Clie apps*/
   /*later Tapwave specific stuff may go here to allow running Zodiac apps*/
   
   FtrSet(sysFileCSystem, sysFtrNumROMVersion, osVer);
   FtrSet(sysFileCSystem, sysFtrNumOEMCompanyID, companyId);
   FtrSet(sysFileCSystem, sysFtrNumOEMDeviceID, deviceId);
   FtrSet(sysFileCSystem, sysFtrNumOEMHALID, halId);
}

static void setConfigDefaults(uint32_t* configFile){
   debugLog("First load, setting default config.\n");
   
   configFile[USER_WARNING_GIVEN] = false;
   configFile[ARM_STACK_SIZE] = 0x4000;
   configFile[LCD_WIDTH] = 160;
   configFile[LCD_HEIGHT] = 220;
   configFile[BOOT_CPU_SPEED] = 800;/*temp, hack for Chuzzle demo*/
   configFile[EXTRA_RAM_MB_DYNAMIC_HEAP] = 10;
}

static Boolean appHandleEvent(EventPtr eventP){
   Boolean handled = false;
   
   if(eventP->eType == frmOpenEvent){
      FormType* form = FrmGetActiveForm();
      
      FrmDrawForm(form);
      handled = true;
   }
   else if(eventP->eType == frmCloseEvent){
      /*nothing for now*/
   }
   
   /*wrong event handler, pass the event down*/
   return handled;
}

static void showGui(uint32_t* configFile){
   EventType event;
   Err unused;
   FormType* currentWindow = NULL;
   
   debugLog("Attempting to load GUI.\n");
   
   /*popup warning dialog*/
   if(!configFile[USER_WARNING_GIVEN]){
      /*continue if user pressed "No", not a real device*/
      if(!FrmAlert(GUI_ALERT_USER_WARNING))
         configFile[USER_WARNING_GIVEN] = true;
   }
   
   /*set starting window*/
   currentWindow = FrmInitForm(GUI_FORM_MAIN_WINDOW);
   FrmSetActiveForm(currentWindow);
   
   do{
      EvtGetEvent(&event, evtWaitForever);
      
      if(!appHandleEvent(&event))
         if(!SysHandleEvent(&event))
            if(!MenuHandleEvent(0, &event, &unused))
               FrmDispatchEvent(&event);
   }
   while(event.eType != appStopEvent);
   
   /*free form memory*/
   FrmDeleteForm(currentWindow);
}

static void initBoot(uint32_t* configFile){
   Err error;
   uint32_t heapFree;
   uint32_t heapBiggestBlock;
   
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
   
   if(configFile[USER_WARNING_GIVEN]){
      Boolean wantCfw = false;
      uint32_t enabledFeatures = readArbitraryMemory32(EMU_REG_ADDR(EMU_INFO));
      
      if(enabledFeatures & FEATURE_RAM_HUGE)
         install128mbRam(configFile[EXTRA_RAM_MB_DYNAMIC_HEAP]);
      
      if(enabledFeatures & FEATURE_HYBRID_CPU){
         SysSetTrapAddress(sysTrapPceNativeCall, (void*)emuPceNativeCall);
         
         if(configFile[ARM_STACK_SIZE] > 0){
            uint8_t* armStackStart = MemPtrNew(configFile[ARM_STACK_SIZE]);/*currently isnt ever freed*/
            
            armv5SetStack(armStackStart, configFile[ARM_STACK_SIZE]);
            setGlobalVar(ARM_STACK_START, (uint32_t)armStackStart);
         }
      }
      
      setProperDeviceId(configFile[LCD_WIDTH], configFile[LCD_HEIGHT], !!(enabledFeatures & FEATURE_HYBRID_CPU), !!(enabledFeatures & FEATURE_EXT_KEYS));
      
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE), configFile[BOOT_CPU_SPEED]);
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_SET_CPU_SPEED);
   }
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
