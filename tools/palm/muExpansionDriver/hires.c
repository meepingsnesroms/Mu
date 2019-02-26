#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "globals.h"
#include "palmGlobalDefines.h"
#include "specs/emuFeatureRegisterSpec.h"


static Boolean loadTungstenWDrivers(void){
   /*check that all driver files are installed and launch drivers boot entrypoints*/
   if(!getGlobalVar(TUNGSTEN_W_DRIVERS_INSTALLED)){
      
      /*TODO*/
      
      /*success*/
      setGlobalVar(TUNGSTEN_W_DRIVERS_INSTALLED, true);
      return true;
   }
   
   /*already installed*/
   return true;
}

static void unloadTungstenWDrivers(void){
   /*put back old window trap list*/
   if(getGlobalVar(TUNGSTEN_W_DRIVERS_INSTALLED)){
      
      /*TODO*/
      
      setGlobalVar(TUNGSTEN_W_DRIVERS_INSTALLED, false);
   }
}

static Boolean setTungstenWDriverFramebuffer(uint16_t width, uint16_t height){
   if(getGlobalVar(TUNGSTEN_W_DRIVERS_INSTALLED)){
      BitmapType* newBitmap;
      WindowType* driverWindow;
      Err error;
      
      driverWindow = WinGetWindowPointer(WinGetDisplayWindow());
      
      newBitmap = BmpCreate(width, height, 16, NULL, &error);
      if(error != errNone)
         return false;
      
      /*clean up old framebuffer and set new one*/
      MemPtrFree(driverWindow->bitmapP);
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

Boolean setDeviceResolution(uint16_t width, uint16_t height){
   uint32_t oldResolution = getGlobalVar(CURRENT_RESOLUTION);
   uint32_t newResolution = (uint32_t)width << 16 | height;
   
   if(newResolution != oldResolution){
      if(width == 160 && height == 220){
         unloadTungstenWDrivers();
         /*need to free old window and set original framebuffer back*/
         /*TODO, theres no going back for now*/
         SysFatalAlert("Cant reverse driver setting!");
         while(true);
      }
      else{
         loadTungstenWDrivers();
         /*all traps have been swaped out, data from the old ones may be invalid now*/
         setTungstenWDriverFramebuffer(width, height);
      }
      
      setGlobalVar(CURRENT_RESOLUTION, newResolution);
   }
   
   return true;
}

void screenSignalApplicationStart(void){
   /*backup original variables*/
   setGlobalVar(ORIGINAL_FRAMEBUFFER, (uint32_t)WinGetBitmap(WinGetDisplayWindow()));
}

void screenSignalApplicationExit(void){
   /*put back all defaults, apps must signal they support hires before its enabled and its disabled again when the app is closed*/
   setDeviceResolution(160, 220);
}
