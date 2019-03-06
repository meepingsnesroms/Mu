#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "debug.h"
#include "config.h"
#include "traps.h"
#include "armv5.h"
#include "hires.h"
#include "globals.h"
#include "memalign.h"
#include "palmGlobalDefines.h"
#include "specs/emuFeatureRegisterSpec.h"


static void installResourceGlobals(void){
   if(!getGlobalVar(RESOURCE_GLOBALS_INITIALIZED)){
      LocalID thisApp;
      DmOpenRef thisAppRef;
      MemHandle funcBlob;
      MemPtr funcBlobPtr;
      
      /*set globals, since this is running at boot time the apps own database isnt loaded it needs to be opened*/
      thisApp = DmFindDatabase(0, APP_NAME);
      if(!thisApp)
         debugLog(APP_NAME " not installed!\n");
      
      thisAppRef = DmOpenDatabase(0, thisApp, dmModeReadOnly | dmModeLeaveOpen);
      if(!thisAppRef)
         debugLog("Cant open " APP_NAME "!\n");
      
      /*ARM_EXIT_FUNC*/
      funcBlob = DmGetResource(FUNCTION_BLOB_TYPE, ARM_EXIT_FUNC_ID);
      if(!funcBlob)
         debugLog("Cant open ARM_EXIT_FUNC!\n");
      
      funcBlobPtr = MemHandleLock(funcBlob);
      if(!funcBlobPtr)
         debugLog("Cant lock ARM_EXIT_FUNC!\n");
      
      if((uint32_t)funcBlobPtr & 0x00000003)
         debugLog("ARM_EXIT_FUNC is unaligned!\n");
      
      setGlobalVar(ARM_EXIT_FUNC, (uint32_t)funcBlobPtr);

      /*ARM_CALL_68K_FUNC*/
      funcBlob = DmGetResource(FUNCTION_BLOB_TYPE, ARM_CALL_68K_FUNC_ID);
      if(!funcBlob)
         debugLog("Cant open ARM_CALL_68K_FUNC!\n");
      
      funcBlobPtr = MemHandleLock(funcBlob);
      if(!funcBlobPtr)
         debugLog("Cant lock ARM_CALL_68K_FUNC!\n");
      
      if((uint32_t)funcBlobPtr & 0x00000003)
         debugLog("ARM_CALL_68K_FUNC is unaligned!\n");
      
      setGlobalVar(ARM_CALL_68K_FUNC, (uint32_t)funcBlobPtr);
      
      /*M68K_CALL_WITH_BLOB_FUNC*/
      funcBlob = DmGetResource(FUNCTION_BLOB_TYPE, M68K_CALL_WITH_BLOB_FUNC_ID);
      if(!funcBlob)
         debugLog("Cant open M68K_CALL_WITH_BLOB_FUNC!\n");
      
      funcBlobPtr = MemHandleLock(funcBlob);
      if(!funcBlobPtr)
         debugLog("Cant lock M68K_CALL_WITH_BLOB_FUNC!\n");
      
      if((uint32_t)funcBlobPtr & 0x00000001)
         debugLog("M68K_CALL_WITH_BLOB_FUNC is unaligned!\n");
      
      setGlobalVar(M68K_CALL_WITH_BLOB_FUNC, (uint32_t)funcBlobPtr);
      
      /*dont run this function again*/
      setGlobalVar(RESOURCE_GLOBALS_INITIALIZED, true);
   }
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
   
   SysSetTrapAddress(sysTrapPceNativeCall, (void*)emuPceNativeCall);
   
   if(oldArmStack)
      memalign_free(oldArmStack);
   
   /*must have 32 bit aligned size and start*/
   armStackSize &= 0xFFFFFFFC;
   armStackStart = memalign_alloc(sizeof(uint32_t), armStackSize);
   armv5SetStack(armStackStart, armStackSize);
   setGlobalVar(ARM_STACK_START, (uint32_t)armStackStart);
}

static void installDebugHandlers(void){
   /*patch all the debug handlers to go to the emu debug console*/
   SysSetTrapAddress(sysTrapErrDisplayFileLineMsg, (void*)emuErrDisplayFileLineMsg);
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
   
   installResourceGlobals();
   
   if(configFile[USER_WARNING_GIVEN]){
      uint32_t enabledFeatures = readArbitraryMemory32(EMU_REG_ADDR(EMU_INFO));
      
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
   }
}

void reinit(uint32_t* configFile){
   /*TODO*/
}
