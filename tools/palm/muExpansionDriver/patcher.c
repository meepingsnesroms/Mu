#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "debug.h"
#include "config.h"
#include "traps.h"
#include "armv5.h"
#include "hires.h"
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

static void resizeTrapTable(void){
   uint16_t oldSr;
   void** oldTrapTable;
   void** newTrapTable;
   
   /*order and time sensitive code!!!*/
   oldSr = SysDisableInts();
   
   /*get new trap table memory*/
   newTrapTable = MemChunkNew(0, (sysTrapLastTrapNumber - sysTrapBase) * sizeof(uint32_t), memNewChunkFlagPreLock | memNewChunkFlagNonMovable | memNewChunkFlagAllowLarge);
   debugLog("New trap table created at:0x%08lX\n", newTrapTable);
   
   /*copy over old OS 4 size trap table and 0 out the OS 5 part*/
   MemMove(newTrapTable, TrapTablePtr, (sysTrapPceNativeCall - sysTrapBase) * sizeof(uint32_t));
   MemSet(newTrapTable + sysTrapPceNativeCall - sysTrapBase, (sysTrapLastTrapNumber - sysTrapPceNativeCall) * sizeof(uint32_t), 0x00);
   
   /*save old table pointer to free*/
   oldTrapTable = TrapTablePtr;
   
   /*swap the tables*/
   TrapTablePtr = newTrapTable;
   
   /*cant free the old table as it is in low mem globals out of the jurisdiction of the memory manager*/
   
   /*return to normal execution*/
   SysRestoreStatus(oldSr);
}

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

static void installPceNativeCallHandler(uint32_t armStackSize){
   uint8_t* oldArmStack = (uint8_t*)getGlobalVar(ARM_STACK_START);
   uint8_t* armStackStart;
   
   TrapTablePtr[sysTrapPceNativeCall - sysTrapBase] = (void*)emuPceNativeCall;
   
   if(oldArmStack)
      MemChunkFree(oldArmStack);
   
   armStackStart = MemChunkNew(0, armStackSize, memNewChunkFlagPreLock | memNewChunkFlagNonMovable | memNewChunkFlagAllowLarge);
   armv5SetStack(armStackStart, armStackSize);
   setGlobalVar(ARM_STACK_START, (uint32_t)armStackStart);
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
   
   /*set CPU ID*/
   if(hasArmCpu)
      FtrSet(sysFtrCreator, sysFtrNumProcessorID, sysFtrNumProcessorXscale);
   else
      FtrSet(sysFtrCreator, sysFtrNumProcessorID, sysFtrNumProcessorVZ);
   
   /*enable dpad API*/
   if(hasDpad)
      FtrSet(navFtrCreator, navFtrVersion, navVersion);
   else
      FtrUnregister(navFtrCreator, navFtrVersion);
   
   /*no fixed digitizer, enable digitizer APIs*/
   if(screenHeight != 220 && screenHeight != 440)
      FtrSet(pinCreator, pinFtrAPIVersion, pinAPIVersion1_1);
   else
      FtrUnregister(pinCreator, pinFtrAPIVersion);
   
   /*later Sony specific stuff may go here to allow running Clie apps*/
   /*later Tapwave specific stuff may go here to allow running Zodiac apps*/
   
   FtrSet(sysFileCSystem, sysFtrNumROMVersion, osVer);
   FtrSet(sysFileCSystem, sysFtrNumOEMCompanyID, companyId);
   FtrSet(sysFileCSystem, sysFtrNumOEMDeviceID, deviceId);
   FtrSet(sysFileCSystem, sysFtrNumOEMHALID, halId);
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
      
      /*make an OS 5 size trap table, needed if debugging or any OS 5 features are enabled*/
      if(enabledFeatures & (FEATURE_HYBRID_CPU | FEATURE_SND_STRMS | FEATURE_DEBUG))
         resizeTrapTable();
         
      if(enabledFeatures & FEATURE_DEBUG)
         installDebugHandlers();
      
      if(enabledFeatures & FEATURE_RAM_HUGE)
         install128mbRam(configFile[EXTRA_RAM_MB_DYNAMIC_HEAP]);
      
      if(enabledFeatures & FEATURE_HYBRID_CPU)
         installPceNativeCallHandler(configFile[ARM_STACK_SIZE]);
      
      if(enabledFeatures & FEATURE_CUSTOM_FB)
         if(installTungstenWLcdDrivers())
            setDeviceResolution(configFile[LCD_WIDTH], configFile[LCD_HEIGHT]);
      
      setProperDeviceId(configFile[LCD_WIDTH], configFile[LCD_HEIGHT], !!(enabledFeatures & FEATURE_HYBRID_CPU), !!(enabledFeatures & FEATURE_EXT_KEYS));
      
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE), configFile[BOOT_CPU_SPEED]);
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_SET_CPU_SPEED);
      
      /*boot succeeded, clear flag*/
      configFile[SAFE_MODE] = false;
      writeConfigFile(configFile);
   }
}
