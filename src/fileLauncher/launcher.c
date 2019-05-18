#include <stdint.h>
#include <stdbool.h>

#include "../emulator.h"
#include "launcher.h"
#include "fatFs/ff.h"


bool launcherSaveSdCardImage;


static void launcherInstallPassMeM515(void){
   //works like PassMe on a DS, boots from a different slot
}

#if defined(EMU_SUPPORT_PALM_OS5)
static void launcherInstallPassMeTungstenC(void){
   //works like PassMe on a DS, boots from a different slot
}
#endif

static const char* launcherGetFileExtension(uint8_t fileType){
   switch(fileType){
      case LAUNCHER_FILE_TYPE_PRC:
         return "PRC";

      case LAUNCHER_FILE_TYPE_PDB:
         return "PDB";

      case LAUNCHER_FILE_TYPE_PQA:
         return "PQA";

      default:
         return "000";
   }
}

static uint32_t launcherMakeBootRecord(const char* appName){
   //writes BOOT.TXT to the PASSME of the SD card, the application in the PassMe image will run the application with the ID listed in this file after everything is copyed over
   //type is always 'appl', creator is the 4 letter app name
   FIL record;
   FRESULT result;
   uint32_t written;

   result = f_open(&record, "0/PASSME/BOOT.TXT", FA_WRITE | FA_CREATE_ALWAYS);
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   result = f_write(&record, appName, 4, &written);
   if(result != FR_OK || written != 4)
      return EMU_ERROR_UNKNOWN;

   result = f_close(&record);
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   return EMU_ERROR_NONE;
}

static uint32_t launcherCopyPrcPdbPqaToSdCard(file_t* file, uint32_t* fileId){
   FIL application;
   char name[FF_MAX_LFN];
   FRESULT result;
   uint32_t written;

   sprintf(name, "0/PASSME/%d.%s", *fileId, launcherGetFileExtension(file->type));

   result = f_open(&application, name, FA_WRITE | FA_CREATE_ALWAYS);
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   result = f_write(&application, file->fileData, file->fileSize, &written);
   if(result != FR_OK || written != file->fileSize)
      return EMU_ERROR_UNKNOWN;

   result = f_close(&application);
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   //set boot record if this is the boot application
   if(file->boot){
      uint32_t error;

      //cant read the name, not a real app
      if(file->fileSize < 0x40 + 4)
         return EMU_ERROR_INVALID_PARAMETER;

      //the app name is at 0x40, dont know if this applys to pqa or not
      error = launcherMakeBootRecord(file->fileData + 0x40);
      if(error != EMU_ERROR_NONE)
         return error;
   }

   (*fileId)++;
   return EMU_ERROR_NONE;
}

static uint32_t launcherCopyZipContentsToSdCard(file_t* file, uint32_t* fileId){
   //TODO
   return EMU_ERROR_NOT_IMPLEMENTED;
}

static uint32_t launcherSetupSdCardImageFromApps(file_t* files, uint32_t fileCount){
   uint32_t fileId = 0;//first file is named 0.p**, next is 1.p**, and the name continues going up with the file count
   FRESULT result;
   uint32_t error;
   uint32_t index;

   //setup filesystem
   result = f_mkfs("0", FM_FAT, 4096, NULL, 0);
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   //setup directory
   result = f_mkdir("0/PASSME");
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   for(index = 0; index < fileCount; index++){
      switch(files[index].type){
         case LAUNCHER_FILE_TYPE_PRC:
         case LAUNCHER_FILE_TYPE_PDB:
         case LAUNCHER_FILE_TYPE_PQA:
            error = launcherCopyPrcPdbPqaToSdCard(&files[index], &fileId);
            break;

         case LAUNCHER_FILE_TYPE_ZIP:
            error = launcherCopyZipContentsToSdCard(&files[index], &fileId);
            break;

         case LAUNCHER_FILE_TYPE_NONE:
         case LAUNCHER_FILE_TYPE_IMG:
         case LAUNCHER_FILE_TYPE_INFO_IMG:
         default:
            error = EMU_ERROR_INVALID_PARAMETER;
            break;
      }

      if(error != EMU_ERROR_NONE)
         return error;
   }

   return EMU_ERROR_NONE;
}

uint32_t launcherLaunch(file_t* files, uint32_t fileCount, uint8_t* sramData, uint32_t sramSize, uint8_t* sdCardData, uint32_t sdCardSize){
   bool applicationFileHasBeenLoaded = false;
   bool cardImageHasBeenLoaded = false;
   bool bootFileExists = false;
   uint32_t bootFileNum;
   uint32_t totalSize = 0;
   bool success;
   uint32_t index;
   
   for(index = 0; index < fileCount; index++){
      //cant load anything else if a card image has been loaded
      if(cardImageHasBeenLoaded)
         return EMU_ERROR_INVALID_PARAMETER;
      
      //cant load a card image if an app has been loaded already
      if(applicationFileHasBeenLoaded && (files[index].type == LAUNCHER_FILE_TYPE_IMG || files[index].type == LAUNCHER_FILE_TYPE_INFO_IMG))
         return EMU_ERROR_INVALID_PARAMETER;
      
      //there must be exactly 1 boot file
      if(files[index].boot){
         if(!bootFileExists)
            bootFileExists = true;
         else
            return EMU_ERROR_INVALID_PARAMETER;
      }

      totalSize += files[index].fileSize;
   }

   if(!bootFileExists)
      return EMU_ERROR_INVALID_PARAMETER;

   //in case the installed apps store anything on the SD card
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
      //in case the emulator is currently running with an SD card inserted
      emulatorEjectSdCard();

      if(sdCardData){
         //use existing SD card image
         emulatorInsertSdCard(sdCardData, sdCardSize, NULL);
      }
      else{
         //load apps to new SD card image
         uint32_t error;

         error = emulatorInsertSdCard(NULL, totalSize, NULL);
         if(error != EMU_ERROR_NONE)
            return error;

         error = launcherSetupSdCardImageFromApps(files, fileCount);
         if(error != EMU_ERROR_NONE)
            return error;
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
   for(index = 0; index < EMU_FPS * 10; index++){
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
