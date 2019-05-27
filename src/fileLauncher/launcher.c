#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

//APIs called by this file
#define MemChunkNew                    0xA011
#define MemChunkFree                   0xA012
#define MemPtrNew                      0xA013
#define DmGetNextDatabaseByTypeCreator 0xA078
#define DmCreateDatabaseFromImage      0xA07F
#define SysUIAppSwitch                 0xA0A7


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

static uint32_t launcherInstallAppM515(uint8_t* data, uint32_t size){
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
   memChunkNewArgs[1] = size;
   memChunkNewArgs[2] = 0x1200/*attr, seems to work without memOwnerID*/;
   palmSideResourceData = launcherM515CallGuestFunction(0x00000000, MemChunkNew, "p(wlw)", memChunkNewArgs);

   //buffer not allocated
   if(!palmSideResourceData)
      return false;

   dbvzChipSelects[DBVZ_CHIP_DX_RAM].readOnlyForProtectedMemory = false;//need to unprotect storage RAM
   MULTITHREAD_LOOP(count) for(count = 0; count < size; count++)
      m68k_write_memory_8(palmSideResourceData + count, data[count]);
   dbvzChipSelects[DBVZ_CHIP_DX_RAM].readOnlyForProtectedMemory = storageRamReadOnly;//restore old protection state
   error = launcherM515CallGuestFunction(0x00000000, DmCreateDatabaseFromImage, "w(p)", &palmSideResourceData);//Err DmCreateDatabaseFromImage(MemPtr bufferP);//this looks best
   launcherM515CallGuestFunction(0x00000000, MemChunkFree, "w(p)", &palmSideResourceData);

   //didnt install
   if(error != 0)
      return EMU_ERROR_OUT_OF_MEMORY;

   return EMU_ERROR_NONE;
}

#if defined(EMU_SUPPORT_PALM_OS5)
static uint32_t launcherInstallAppTungstenT3(uint8_t* data, uint32_t size){
   //TODO
   return EMU_ERROR_NOT_IMPLEMENTED;
}
#endif

static uint32_t launcherLaunchAppM515(uint32_t appCode){
   //Err SysUIAppSwitch(UInt16 cardNo, LocalID dbID, UInt16 cmd, MemPtr cmdPBP);
   //Err DmGetNextDatabaseByTypeCreator(Boolean newSearch, DmSearchStatePtr stateInfoP, UInt32 type, UInt32 creator, Boolean onlyLatestVers, UInt16 *cardNoP, LocalID *dbIDP);
   uint32_t args[7];
   uint32_t returnBuffer;
   uint16_t error;

   args[0] = 8 * sizeof(uint32_t) + sizeof(uint16_t) + 4;
   returnBuffer = launcherM515CallGuestFunction(0x00000000, MemPtrNew, "p(l)", args);
   if(!returnBuffer)
      return EMU_ERROR_OUT_OF_MEMORY;

   args[0] = true;
   args[1] = returnBuffer;
   args[2] = 'appl';
   args[3] = appCode;
   args[4] = false;
   args[5] = returnBuffer + 8 * sizeof(uint32_t);
   args[6] = returnBuffer + 8 * sizeof(uint32_t) + sizeof(uint16_t);
   error = launcherM515CallGuestFunction(0x00000000, DmGetNextDatabaseByTypeCreator, "w(bpllbpp)", args);
   if(error != 0)
      return EMU_ERROR_UNKNOWN;

   args[0] = 0;//cardNo
   args[1] = m68k_read_memory_32(returnBuffer + 8 * sizeof(uint32_t) + sizeof(uint16_t));//the LocalID
   args[2] = 0;//sysAppLaunchCmdNormalLaunch
   args[3] = 0;//NULL
   error = launcherM515CallGuestFunction(0x00000000, SysUIAppSwitch, "w(wlwp)", args);

   //needs to be freed even if launch fails
   args[0] = returnBuffer;
   launcherM515CallGuestFunction(0x00000000, MemChunkFree, "w(p)", args);

   if(error != 0)
      return EMU_ERROR_UNKNOWN;

   return EMU_ERROR_NONE;
}

#if defined(EMU_SUPPORT_PALM_OS5)
static uint32_t launcherLaunchAppTungstenT3(uint32_t appCode){
   //TODO
   return EMU_ERROR_NOT_IMPLEMENTED;
}
#endif

static void launcherCalibrateTouchscreenM515(void){
   uint32_t index;

   //touch center first time, to skip prompt
   palmInput.touchscreenX = 160.0 / 2.0 / 160.0;
   palmInput.touchscreenY = 160.0 / 2.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release center first time, to skip prompt
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch center second time, to skip prompt
   palmInput.touchscreenX = 160.0 / 2.0 / 160.0;
   palmInput.touchscreenY = 160.0 / 2.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release center second time, to skip prompt
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch upper left
   palmInput.touchscreenX = 10.0 / 160.0;
   palmInput.touchscreenY = 10.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release upper left
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch lower right
   palmInput.touchscreenX = (160.0 - 10.0) / 160.0;
   palmInput.touchscreenY = (160.0 - 10.0) / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release lower right
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch center(offset slightly to match icon)
   palmInput.touchscreenX = 160.0 / 2.0 / 160.0;
   palmInput.touchscreenY = (160.0 / 2.0 - 20.0) / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release center
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch "Next"
   palmInput.touchscreenX = 68.0 / 160.0;
   palmInput.touchscreenY = 153.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release "Next"
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //touch "Done"
   palmInput.touchscreenX = 130.0 / 160.0;
   palmInput.touchscreenY = 151.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release "Done"
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();
}

#if defined(EMU_SUPPORT_PALM_OS5)
static void launcherCalibrateTouchscreenTungstenT3(void){
   //TODO
}
#endif

static void launcherPushHomeButtonTouchscreenM515(void){
   uint32_t index;

   //press
   palmInput.touchscreenX = 10.0 / 160.0;
   palmInput.touchscreenY = 160.0 + 10.0 / 220.0;
   palmInput.touchscreenTouched = true;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();

   //release
   palmInput.touchscreenTouched = false;
   for(index = 0; index < EMU_FPS * LAUNCHER_TOUCH_DURATION; index++)
      emulatorSkipFrame();
}

#if defined(EMU_SUPPORT_PALM_OS5)
static void launcherPushHomeButtonTouchscreenTungstenT3(void){
   //TODO
}
#endif

void launcherBootInstantly(bool hasSram){
   uint32_t index;

   if(hasSram){
      //just boot from SRAM
      for(index = 0; index < EMU_FPS * 7.0; index++)
         emulatorSkipFrame();

#if defined(EMU_SUPPORT_PALM_OS5)
      if(palmEmulatingTungstenT3)
         launcherPushHomeButtonTouchscreenTungstenT3();
      else
#endif
         launcherPushHomeButtonTouchscreenM515();

      //give it time to go home
      for(index = 0; index < EMU_FPS * 1.0; index++)
         emulatorSkipFrame();
   }
   else{
      //start boot, calibrate touchscreen, skip setup
      for(index = 0; index < EMU_FPS * 10.0; index++)
         emulatorSkipFrame();

#if defined(EMU_SUPPORT_PALM_OS5)
      if(palmEmulatingTungstenT3)
         launcherCalibrateTouchscreenTungstenT3();
      else
#endif
         launcherCalibrateTouchscreenM515();

      //goes to home screen on its own after setup

      //give it time to go home
      for(index = 0; index < EMU_FPS * 1.0; index++)
         emulatorSkipFrame();
   }
}

uint32_t launcherInstallFile(uint8_t* data, uint32_t size){
#if defined(EMU_SUPPORT_PALM_OS5)
      if(palmEmulatingTungstenT3)
         return launcherInstallAppTungstenT3(data, size);
      else
#endif
         return launcherInstallAppM515(data, size);
}

bool launcherIsExecutable(uint8_t* data, uint32_t size){
   if(size > 0x4D){
      //has full prc header
      if(data[0x21] & 0x01){
         //is a prc
         uint32_t type = readStateValue32(data + 0x3C);

         if(type == 'appl')
            return true;
      }
   }

   return false;
}

uint32_t launcherGetAppId(uint8_t* data, uint32_t size){
   if(size > 0x4D){
      //has full prc header
      if(data[0x21] & 0x01){
         //is a prc
         uint32_t type = readStateValue32(data + 0x3C);
         uint32_t creator = readStateValue32(data + 0x40);

         if(type == 'appl')
            return creator;
      }
   }

   return 0x00000000;
}

uint32_t launcherExecute(uint32_t appId){
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3)
      return launcherLaunchAppTungstenT3(appId);
   else
#endif
      return launcherLaunchAppM515(appId);
}

void launcherGetSdCardInfoFromInfoFile(uint8_t* data, uint32_t size, sd_card_info_t* returnValue){
   //parse a small binary file with the extra SD card data
   uint32_t offset = 0;

   //clear everything
   memset(returnValue, 0x00, sizeof(sd_card_info_t));

   //csd
   if(size < offset + 16)
      return;
   memcpy(returnValue->csd, data + offset, 16);
   offset += 16;

   //cid
   if(size < offset + 16)
      return;
   memcpy(returnValue->cid, data + offset, 16);
   offset += 16;

   //scr
   if(size < offset + 8)
      return;
   memcpy(returnValue->scr, data + offset, 8);
   offset += 8;

   //ocr
   if(size < offset + sizeof(uint32_t))
      return;
   returnValue->ocr = readStateValue32(data + offset);
   offset += sizeof(uint32_t);

   //writeProtectSwitch
   if(size < offset + sizeof(uint8_t))
      return;
   returnValue->writeProtectSwitch = readStateValue8(data + offset);
   offset += sizeof(uint8_t);

   //this can keep growing with new values but all the current values are fixed!!
}
