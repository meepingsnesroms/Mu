#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../emulator.h"
#include "../portability.h"
#include "launcher.h"
#include "fatFs/ff.h"


#define LAUNCHER_SD_CARD_SIZE (16 * 0x100000) //SD card size for the default boot configuration
#define LAUNCHER_BOOT_TIMEOUT 10 //in seconds

bool launcherSaveSdCardImage;


static void launcherInstallPassMeM515(void){
   //works like PassMe on a DS, boots from a different slot
}

#if defined(EMU_SUPPORT_PALM_OS5)
static void launcherInstallPassMeTungstenT3(void){
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

   result = f_open(&record, "0:/PASSME/BOOT.TXT", FA_WRITE | FA_CREATE_ALWAYS);
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

static uint32_t launcherCopyPrcPdbPqaToSdCard(launcher_file_t* file, uint32_t* fileId){
   FIL application;
   char name[FF_MAX_LFN];
   FRESULT result;
   uint32_t written;

   sprintf(name, "0:/PASSME/%d.%s", *fileId, launcherGetFileExtension(file->type));

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

static uint32_t launcherSetupSdCardImageFromApps(launcher_file_t* files, uint32_t fileCount){
   uint32_t fileId = 0;//first file is named 0.p**, next is 1.p**, and the name continues going up with the file count
   FATFS* fsData;
   FRESULT result;
   uint32_t error;
   uint32_t index;

   //setup filesystem
   result = f_mkfs("0:/", FM_FAT, SD_CARD_BLOCK_SIZE, NULL, 0);
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   fsData = malloc(sizeof(FATFS));
   if(!fsData)
      return EMU_ERROR_OUT_OF_MEMORY;

   result = f_mount(fsData, "0:/", 1);
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   //setup directory
   result = f_mkdir("0:/PASSME");
   if(result != FR_OK)
      return EMU_ERROR_UNKNOWN;

   for(index = 0; index < fileCount; index++){
      switch(files[index].type){
         case LAUNCHER_FILE_TYPE_PRC:
         case LAUNCHER_FILE_TYPE_PDB:
         case LAUNCHER_FILE_TYPE_PQA:
            error = launcherCopyPrcPdbPqaToSdCard(&files[index], &fileId);
            break;

         case LAUNCHER_FILE_TYPE_NONE:
         case LAUNCHER_FILE_TYPE_IMG:
         default:
            error = EMU_ERROR_INVALID_PARAMETER;
            break;
      }

      if(error != EMU_ERROR_NONE)
         return error;
   }

   f_unmount("0:/");
   free(fsData);

   return EMU_ERROR_NONE;
}

void launcherGetSdCardInfoFromInfoFile(launcher_file_t* file, sd_card_info_t* returnValue){
   //parse a small binary file with the extra SD card data
   uint32_t offset = 0;

   //clear everything
   memset(returnValue, 0x00, sizeof(sd_card_info_t));

   //csd
   if(file->infoSize < offset + 16)
      return;
   memcpy(returnValue->csd, file->infoData + offset, 16);
   offset += 16;

   //cid
   if(file->infoSize < offset + 16)
      return;
   memcpy(returnValue->cid, file->infoData + offset, 16);
   offset += 16;

   //scr
   if(file->infoSize < offset + 8)
      return;
   memcpy(returnValue->scr, file->infoData + offset, 8);
   offset += 8;

   //ocr
   if(file->infoSize < offset + sizeof(uint32_t))
      return;
   returnValue->ocr = readStateValue32(file->infoData + offset);
   offset += sizeof(uint32_t);

   //writeProtectSwitch
   if(file->infoSize < offset + sizeof(uint8_t))
      return;
   returnValue->writeProtectSwitch = readStateValue8(file->infoData + offset);
   offset += sizeof(uint8_t);

   //this can keep growing with new values but all the current values are fixed!!
}

uint32_t launcherLaunch(launcher_file_t* files, uint32_t fileCount, uint8_t* sramData, uint32_t sramSize, uint8_t* sdCardData, uint32_t sdCardSize){
   bool applicationFileHasBeenLoaded = false;
   bool cardImageHasBeenLoaded = false;
   bool bootFileExists = false;
   uint32_t bootFileNum;
   uint32_t totalSize = 0;
   bool success;
   uint32_t error;
   uint32_t index;
   
   for(index = 0; index < fileCount; index++){
      //cant load anything else if a card image has been loaded
      if(cardImageHasBeenLoaded)
         return EMU_ERROR_INVALID_PARAMETER;
      
      //cant load a card image if an app has been loaded already
      if(applicationFileHasBeenLoaded && files[index].type == LAUNCHER_FILE_TYPE_IMG)
         return EMU_ERROR_INVALID_PARAMETER;

      if(files[index].type == LAUNCHER_FILE_TYPE_IMG)
         cardImageHasBeenLoaded = true;
      else
         applicationFileHasBeenLoaded = true;
      
      //there must be exactly 1 boot file
      if(files[index].boot){
         if(!bootFileExists){
            bootFileExists = true;
            bootFileNum = index;
         }
         else{
            return EMU_ERROR_INVALID_PARAMETER;
         }
      }

      totalSize += files[index].fileSize;
   }

   if(!bootFileExists)
      return EMU_ERROR_INVALID_PARAMETER;

   //in case the installed apps store anything on the SD card
   launcherSaveSdCardImage = true;

   //in case the emulator is currently running with an SD card inserted
   emulatorEjectSdCard();

   //make the card image or just copy it over
   if(cardImageHasBeenLoaded){
      if(files[bootFileNum].infoData){
         //with info file
         sd_card_info_t sdInfo;

         //get SD info from file
         launcherGetSdCardInfoFromInfoFile(&files[bootFileNum], &sdInfo);

         //load card image
         error = emulatorInsertSdCard(files[bootFileNum].fileData, files[bootFileNum].fileSize, &sdInfo);
         if(error != EMU_ERROR_NONE)
            return error;

         //dont save read only card images
         if(sdInfo.writeProtectSwitch)
            launcherSaveSdCardImage = false;
      }
      else{
         //without info file
         error = emulatorInsertSdCard(files[bootFileNum].fileData, files[bootFileNum].fileSize, NULL);
         if(error != EMU_ERROR_NONE)
            return error;
      }
   }
   else{
      if(sdCardData){
         //use existing SD card image
         error = emulatorInsertSdCard(sdCardData, sdCardSize, NULL);
         if(error != EMU_ERROR_NONE)
            return error;
      }
      else{
         //load apps to new SD card image
         if(totalSize > LAUNCHER_SD_CARD_SIZE)
            return EMU_ERROR_OUT_OF_MEMORY;

         error = emulatorInsertSdCard(NULL, LAUNCHER_SD_CARD_SIZE, NULL);
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
      //install a pre specified RAM image for the current device(m515 or Tungsten T3)needs to be decompressed into palmRam here
#if defined(EMU_SUPPORT_PALM_OS5)
      if(palmEmulatingTungstenT3)
         launcherInstallPassMeTungstenT3();
      else
#endif
         launcherInstallPassMeM515();
   }

   emulatorSoftReset();
   
   //leave this off until I make the PassMe program
   /*
   //execute frames until launch is completed(or failed with a time out)
   success = false;
   palmEmuFeatures.value = cardImageHasBeenLoaded ? 'RUNC' : 'RUNA';
   for(index = 0; index < EMU_FPS * LAUNCHER_BOOT_TIMEOUT; index++){
      emulatorRunFrame();
      if(palmEmuFeatures.value == 'STOP'){
         success = true;
         break;
      }
   }

   //timed out
   if(!success)
      return EMU_ERROR_RESOURCE_LOCKED;
   */

   //worked
   return EMU_ERROR_NONE;
}
