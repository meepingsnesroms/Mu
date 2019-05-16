#include <stdint.h>
#include <stdbool.h>

#include "../emulator.h"
#include "launcher.h"
#include "fatFs/ff.h"


bool launcherSaveSdCardImage;


void launcherInstallPassMeM515(void){
   //works like PassMe on a DS, boots from a different slot
}

#if defined(EMU_SUPPORT_PALM_OS5)
void launcherInstallPassMeTungstenC(void){
   //works like PassMe on a DS, boots from a different slot
}
#endif

uint32_t launcherLaunch(file_t* files, uint32_t fileCount, uint8_t* sramData, uint32_t sramSize, uint8_t* sdCardData, uint32_t sdCardSize){
   bool applicationFileHasBeenLoaded = false;
   bool cardImageHasBeenLoaded = false;
   bool bootFileExists = false;
   uint32_t bootFileNum;
   uint64_t totalSize = 0;
   uint32_t count;
   bool success;
   
   for(count = 0; count < fileCount; count++){
      //cant load anything else if a card image has been loaded
      if(cardImageHasBeenLoaded)
         return EMU_ERROR_INVALID_PARAMETER;
      
      //cant load a card image if an app has been loaded already
      if(applicationFileHasBeenLoaded && (files[count].type == LAUNCHER_FILE_TYPE_IMG || files[count].type == LAUNCHER_FILE_TYPE_INFO_IMG))
         return EMU_ERROR_INVALID_PARAMETER;
      
      if(files[count].boot){
         bootFileExists = true;
         bootFileNum = count;
      }

      totalSize += files[count].fileSize;
   }

   if(!bootFileExists)
      return EMU_ERROR_INVALID_PARAMETER;

   //incase the installed apps store anything on the SD card
   launcherSaveSdCardImage = true;

   //make the card image or just copy it over
   if(cardImageHasBeenLoaded){
      //load card image
      emulatorInsertSdCard(files[bootFileNum].fileData, files[bootFileNum].fileSize, files[bootFileNum].info);

      //dont save read only card images
      if(files[bootFileNum].info && ((sd_card_info_t*)files[bootFileNum].info)->writeProtectSwitch)
         launcherSaveSdCardImage = false;
   }
   else{
      if(sdCardData){
         //use existing SD card image
         emulatorInsertSdCard(sdCardData, sdCardSize, NULL);
      }
      else{
         //load apps to new SD card image
         //TODO
      }
   }
   
   if(sramData){
      //use the existing SRAM file
      emulatorLoadRam(sramData, sramSize);
   }
   else{
      //install a pre specified RAM image for the current device(m515 or Tungsten C)needs to be decompressed into palmRam here
#if defined(EMU_SUPPORT_PALM_OS5)
      if(palmEmulatingTungstenC)
         launcherInstallPassMeTungstenC();
      else
#endif
         launcherInstallPassMeM515();
   }

   emulatorSoftReset();
   
   //execute frames until launch is completed(or failed with a time out)
   success = false;
   palmEmuFeatures.value = 'RUN0';
   for(count = 0; count < EMU_FPS * 10; count++){
      emulatorRunFrame();
      if(palmEmuFeatures.value == 'STOP'){
         success = true;
         break;
      }
   }

   //timed out
   if(!success)
      return EMU_ERROR_RESOURCE_LOCKED;

   //worked
   return EMU_ERROR_NONE;
}
