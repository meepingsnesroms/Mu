#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../emulator.h"
#include "../portability.h"
#include "../m68k/m68k.h"
#include "../m68k/m68kcpu.h"
#include "../dbvz.h"
#include "launcher.h"


typedef struct{
   uint32_t sp;
   uint32_t pc;
   uint16_t sr;
   uint32_t a0;
   uint32_t d0;
}launcher_m68k_local_cpu_state_t;


#define LAUNCHER_TOUCH_DURATION 0.3//in seconds


bool launcherSaveSdCardImage;


static uint32_t launcherM515GetStackFrameSize(const char* prototype){
   const char* params = prototype + 2;
   uint32_t size = 0;

   while(*params != ')'){
      switch(*params){
         case 'v':
            //do nothing
            break;

         case 'b':
            //bytes are 16 bits long on the stack due to memory alignment restrictions
         case 'w':
            size += 2;
            break;

         case 'l':
         case 'p':
            size += 4;
            break;
      }

      params++;
   }

   return size;
}

static uint32_t launcherM515CallGuestFunction(uint32_t address, uint16_t trap, const char* prototype, uint32_t* argData){
   //prototype is a Java style function signature describing values passed and returned "v(wllp)"
   //is return void and pass a uint16_t(word), 2 uint32_t(long) and 1 pointer
   //valid types are b(yte), w(ord), l(ong), p(ointer) and v(oid)
   const char* params = prototype + 2;
   uint32_t argIndex = 0;
   uint32_t stackFrameStart = m68k_get_reg(NULL, M68K_REG_SP);
   uint32_t newStackFrameSize = launcherM515GetStackFrameSize(prototype);
   uint32_t stackWriteAddr = stackFrameStart - newStackFrameSize;
   uint32_t oldStopped = m68ki_cpu.stopped;
   uint32_t functionReturn = 0x00000000;
   launcher_m68k_local_cpu_state_t oldCpuState;
   uint32_t callWriteOut = 0xFFFFFFE0;
   uint32_t callStart;

   //backup old state
   oldCpuState.sp = m68k_get_reg(NULL, M68K_REG_SP);
   oldCpuState.pc = m68k_get_reg(NULL, M68K_REG_PC);
   oldCpuState.sr = m68k_get_reg(NULL, M68K_REG_SR);
   oldCpuState.a0 = m68k_get_reg(NULL, M68K_REG_A0);
   oldCpuState.d0 = m68k_get_reg(NULL, M68K_REG_D0);

   while(*params != ')'){
      switch(*params){
         case 'v':
            break;

         case 'b':
            //bytes are 16 bits long on the stack due to memory alignment restrictions
            //bytes are written to the top byte of there word
            m68k_write_memory_8(stackWriteAddr, argData[argIndex]);
            argIndex++;
            stackWriteAddr += 2;
            break;
         case 'w':
            m68k_write_memory_16(stackWriteAddr, argData[argIndex]);
            argIndex++;
            stackWriteAddr += 2;
            break;

         case 'l':
         case 'p':
            m68k_write_memory_32(stackWriteAddr, argData[argIndex]);
            argIndex++;
            stackWriteAddr += 4;
            break;
      }

      params++;
   }

   //write to the bootloader memory
   callStart = callWriteOut;

   if(address){
      //direct jump handler, used for private APIs
      m68k_write_memory_16(callWriteOut, 0x4EB9);//jump to subroutine opcode
      callWriteOut += 2;
      m68k_write_memory_32(callWriteOut, address);
      callWriteOut += 4;
   }
   else{
      //OS function handler
      m68k_write_memory_16(callWriteOut, 0x4E4F);//trap f opcode
      callWriteOut += 2;
      m68k_write_memory_16(callWriteOut, trap);
      callWriteOut += 2;
   }

   m68ki_cpu.stopped = 0;
   m68k_set_reg(M68K_REG_SP, stackFrameStart - newStackFrameSize);
   m68k_set_reg(M68K_REG_PC, callStart);

   //run until function returns
   while(m68k_get_reg(NULL, M68K_REG_PC) != callWriteOut)
      m68k_execute(1);//m68k_execute() always runs requested cycles + extra cycles of the final opcode, this executes 1 opcode
   if(prototype[0] == 'p')
      functionReturn = m68k_get_reg(NULL, M68K_REG_A0);
   else if(prototype[0] == 'b' || prototype[0] == 'w' || prototype[0] == 'l')
      functionReturn = m68k_get_reg(NULL, M68K_REG_D0);
   m68ki_cpu.stopped = oldStopped;

   //restore state
   m68k_set_reg(M68K_REG_SP, oldCpuState.sp);
   m68k_set_reg(M68K_REG_PC, oldCpuState.pc);
   m68k_set_reg(M68K_REG_SR, oldCpuState.sr & 0xF0FF | m68k_get_reg(NULL, M68K_REG_SR) & 0x0700);//dont restore intMask
   m68k_set_reg(M68K_REG_A0, oldCpuState.a0);
   m68k_set_reg(M68K_REG_D0, oldCpuState.d0);

   return functionReturn;
}

static uint32_t launcherInstallAppM515(launcher_file_t* file){
   /*
   #define memNewChunkFlagNonMovable    0x0200
   #define memNewChunkFlagAllowLarge    0x1000  // this is not in the sdk *g*
   */
   uint32_t palmSideResourceData;
   bool storageRamReadOnly = dbvzChipSelects[DBVZ_CHIP_DX_RAM].readOnlyForProtectedMemory;
   uint16_t error;
   uint32_t count;
   uint32_t memChunkNewArgs[3];

   //try and get guest memory buffer
   memChunkNewArgs[0] = 1/*heapID, storage RAM*/;
   memChunkNewArgs[1] = file->fileSize;
   memChunkNewArgs[2] = 0x1200/*attr, seems to work without memOwnerID*/;
   palmSideResourceData = launcherM515CallGuestFunction(0x00000000, 0xA011/*MemChunkNew*/, "p(wlw)", memChunkNewArgs);

   //buffer not allocated
   if(!palmSideResourceData)
      return false;

   dbvzChipSelects[DBVZ_CHIP_DX_RAM].readOnlyForProtectedMemory = false;//need to unprotect storage RAM
   MULTITHREAD_LOOP(count) for(count = 0; count < file->fileSize; count++)
      m68k_write_memory_8(palmSideResourceData + count, file->fileData[count]);
   dbvzChipSelects[DBVZ_CHIP_DX_RAM].readOnlyForProtectedMemory = storageRamReadOnly;//restore old protection state
   error = launcherM515CallGuestFunction(0x00000000, 0xA07F/*DmCreateDatabaseFromImage*/, "w(p)", &palmSideResourceData);//Err DmCreateDatabaseFromImage(MemPtr bufferP);//this looks best
   launcherM515CallGuestFunction(0x00000000, 0xA012/*MemChunkFree*/, "w(p)", &palmSideResourceData);

   //didnt install
   if(error != 0)
      return EMU_ERROR_OUT_OF_MEMORY;

   return EMU_ERROR_NONE;
}

#if defined(EMU_SUPPORT_PALM_OS5)
static uint32_t launcherInstallAppTungstenT3(launcher_file_t* file){
   //TODO
   return EMU_ERROR_NOT_IMPLEMENTED;
}
#endif

static void launcherCalibrateTouchscreenM515(void){
   uint32_t index;

   //touch center, to skip prompt
   palmInput.touchscreenX = 160.0 / 2.0 / 160.0;
   palmInput.touchscreenY = (220.0 - 60.0) / 2.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release center, to skip prompt
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch upper left
   palmInput.touchscreenX = 8.0 / 160.0;
   palmInput.touchscreenY = 8.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release upper left
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch lower right
   palmInput.touchscreenX = (160.0 - 8.0) / 160.0;
   palmInput.touchscreenY = (220.0 - 60.0 - 8.0) / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release lower right
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch center
   palmInput.touchscreenX = 160.0 / 2.0 / 160.0;
   palmInput.touchscreenY = (220.0 - 60.0) / 2.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release center
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();
}

#if defined(EMU_SUPPORT_PALM_OS5)
static void launcherCalibrateTouchscreenTungstenT3(void){
   //TODO
}
#endif

static void launcherGetSdCardInfoFromInfoFile(launcher_file_t* file, sd_card_info_t* returnValue){
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
   bool cardImageHasBeenLoaded = false;
   uint32_t cardImage;
   uint32_t totalSize = 0;
   uint32_t error;
   uint32_t index;
   
   for(index = 0; index < fileCount; index++){
      //cant load a 2 card images
      if(cardImageHasBeenLoaded && files[index].type == LAUNCHER_FILE_TYPE_IMG)
         return EMU_ERROR_INVALID_PARAMETER;

      if(files[index].type == LAUNCHER_FILE_TYPE_IMG){
         cardImageHasBeenLoaded = true;
         cardImage = index;
      }

      totalSize += files[index].fileSize;
   }

   //in case the installed apps store anything on the SD card
   launcherSaveSdCardImage = true;

   //in case the emulator is currently running with an SD card inserted
   emulatorEjectSdCard();

   //dont want a data leak, could cause glitches
   emulatorHardReset();

   //insert card if present
   if(cardImageHasBeenLoaded){
      if(files[cardImage].infoData){
         //with info file
         sd_card_info_t sdInfo;

         //get SD info from file
         launcherGetSdCardInfoFromInfoFile(&files[cardImage], &sdInfo);

         //load card image
         error = emulatorInsertSdCard(files[cardImage].fileData, files[cardImage].fileSize, &sdInfo);
         if(error != EMU_ERROR_NONE)
            return error;

         //dont save read only card images
         if(sdInfo.writeProtectSwitch)
            launcherSaveSdCardImage = false;
      }
      else{
         //without info file
         error = emulatorInsertSdCard(files[0].fileData, files[0].fileSize, NULL);
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
   }

   //use the existing SRAM file if available
   if(sramData)
      emulatorLoadRam(sramData, sramSize);
   
   //execute frames until launch is completed(or failed with a time out)
   if(sramData){
      //just boot from SRAM
      for(index = 0; index < EMU_FPS * 10.0; index++)
         emulatorSkipFrame();
   }
   else{
      //start boot, calibrate touchscreen, install apps, finish boot
      for(index = 0; index < EMU_FPS * 10.0; index++)
         emulatorSkipFrame();

#if defined(EMU_SUPPORT_PALM_OS5)
      if(palmEmulatingTungstenT3)
         launcherCalibrateTouchscreenTungstenT3();
      else
#endif
         launcherCalibrateTouchscreenM515();

      //install all non img files
      for(index = 0; index < fileCount; index++)
         if(files[index].type == LAUNCHER_FILE_TYPE_RESOURCE_FILE)
            launcherInstallFiles(&files[index], 1);

      //let it refresh everything
      for(index = 0; index < EMU_FPS * 2.0; index++)
         emulatorSkipFrame();
   }

   //worked
   return EMU_ERROR_NONE;
}

uint32_t launcherInstallFiles(launcher_file_t* files, uint32_t fileCount){
   uint32_t index;

   for(index = 0; index < fileCount; index++){
      uint32_t error;

      //cant install img files
      if(files[index].type != LAUNCHER_FILE_TYPE_RESOURCE_FILE)
         return EMU_ERROR_INVALID_PARAMETER;

#if defined(EMU_SUPPORT_PALM_OS5)
      if(palmEmulatingTungstenT3)
         error = launcherInstallAppTungstenT3(&files[index]);
      else
#endif
         error = launcherInstallAppM515(&files[index]);

      if(error != EMU_ERROR_NONE)
         return error;
   }

   return EMU_ERROR_NONE;
}
