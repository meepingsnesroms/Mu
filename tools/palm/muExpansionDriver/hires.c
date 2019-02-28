#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "debug.h"
#include "globals.h"
#include "palmGlobalDefines.h"
#include "specs/emuFeatureRegisterSpec.h"


static Boolean setTungstenWDriverFramebuffer(uint16_t width, uint16_t height){
   if(getGlobalVar(TUNGSTEN_W_DRIVERS_INSTALLED)){
      BitmapTypeV3* newBitmap;
      WindowType* driverWindow;
      Err error;
      
      driverWindow = WinGetWindowPointer(WinGetDisplayWindow());
      
      newBitmap = BmpCreateBitmapV3(BmpCreate(width, height, 16, NULL, &error), kDensityDouble, NULL/*bitsP*/, NULL/*colorTableP*/);
      if(error != errNone)
         return false;
      
      /*clean up old framebuffer if its not the original one and set new one*/
      driverWindow->bitmapP->flags.forScreen = false;
      BmpDelete(driverWindow->bitmapP);
      
      newBitmap->flags.forScreen = true;
      driverWindow->bitmapP = newBitmap;
      
      /*tell the emu where the framebuffer is*/
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_SRC), (uint32_t)BmpGetBits(newBitmap));
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE), (uint32_t)width << 16 | height);
      writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_LCD_SET_FB);
      
      return true;
   }
   
   /*driver is not loaded, cant set window*/
   return false;
}

Boolean installTungstenWLcdDrivers(void){
   /*check that all driver files are installed and launch drivers boot entrypoints*/
   if(!getGlobalVar(TUNGSTEN_W_DRIVERS_INSTALLED)){
#if 1
      Err error;
      uint16_t unused1;
      LocalID unused2;
      DmSearchStateType unused3;
      DmOpenRef highDensityDisplay;
      MemHandle highDensityDisplayInitHandle;
      void (*installHighDensityDisplay)(void);
      
      /*Hi Res Blitter.prc, this is the Tungsten W GPU driver, this will crash the device so dont install it*/
      /*PRC ID:bhal hire*/
      /*Resource ID:boot0000*/
      
      /*HighDensityFonts.prc, font pack, only need to check if present*/
      /*PRC ID:data hidd*/
      /*Resource ID:nfnt????*/
      error = DmGetNextDatabaseByTypeCreator(true, &unused3, 'data', 'hidd', false, &unused1, &unused2);
      if(error != errNone)
         return false;/*file does not exist*/
      
      /*HighDensityDisplay.prc, HAL addon, need to jump to exte0000/0x00000010*/
      /*PRC ID:extn hidd*/
      /*Resource ID:????????*/
      error = DmGetNextDatabaseByTypeCreator(true,  &unused3, 'extn', 'hidd', false, &unused1, &unused2);
      if(error != errNone)
         return false;/*file does not exist*/
      
      highDensityDisplayInitHandle = DmGetResource('exte', 0x0000);
      if(!highDensityDisplayInitHandle)
         return false;
      installHighDensityDisplay = (void(*)(void))((uint32_t)MemHandleLock(highDensityDisplayInitHandle) + 0x00000010);
      
      /*backup original LCD window for SED1376*/
      setGlobalVar(ORIGINAL_FRAMEBUFFER, (uint32_t)WinGetBitmap(WinGetDisplayWindow()));
      
      /*may need to patch HwrDisplayAttributes before running so driver will install*/
      
      /*run HighDensityDisplay.prc exte0000, 0x00000010*/
      installHighDensityDisplay();
      
      debugLog("Tungsten W drivers installed!\n");
      
      /*close everything*/
      MemHandleUnlock(highDensityDisplayInitHandle);
      DmCloseDatabase(highDensityDisplay);
#endif
      
      /*success*/
      setGlobalVar(TUNGSTEN_W_DRIVERS_INSTALLED, true);
      return true;
   }
   
   /*already installed*/
   return true;
}

Boolean setDeviceResolution(uint16_t width, uint16_t height){
   uint32_t oldResolution = getGlobalVar(CURRENT_RESOLUTION);
   uint32_t newResolution = (uint32_t)width << 16 | height;
   
   if(newResolution != oldResolution){
      if(!setTungstenWDriverFramebuffer(width, height))
         return false;
      
      setGlobalVar(CURRENT_RESOLUTION, newResolution);
   }
   
   return true;
}

void screenSignalApplicationStart(void){
   /*nothing for now*/
}

void screenSignalApplicationExit(void){
   /*put back all defaults, apps must signal they support hires before its enabled and its disabled again when the app is closed*/
   setDeviceResolution(160, 220);
}
