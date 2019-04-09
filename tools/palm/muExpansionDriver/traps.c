#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "debug.h"
#include "armv5.h"
#include "traps.h"
#include "globals.h"
#include "palmOsPriv.h"
#include "palmGlobalDefines.h"


/*cant use normal global variables in this file!!!*/
/*functions in this file are called directly by the Palm OS trap dispatch, this means they can be called when the app isnt loaded and the globals are just a random data buffer*/


UInt32 emuPceNativeCall(NativeFuncType* nativeFuncP, void* userDataP){
   /*these arnt set as "static" because the globals dont work here*/
   const ALIGN(2) uint32_t armExitFunc[] = {0x0000A0E3, 0xBBBBBBF7};/*ARM asm blob*/
   const ALIGN(2) uint32_t armCall68kFunc[] = {0x0100A0E3, 0xBBBBBBF7, 0x0EF0A0E1};/*ARM asm blob*/
   const ALIGN(2) uint8_t m68kCallWithBlobFunc[] = {
      0x4e, 0x56, 0x00, 0x00, 0x48, 0xe7, 0x60, 0x70, 0x26, 0x6e, 0x00, 0x08,
      0x22, 0x6e, 0x00, 0x0c, 0x22, 0x2e, 0x00, 0x10, 0x34, 0x2e, 0x00, 0x14,
      0x24, 0x4f, 0x95, 0xc1, 0xbf, 0xca, 0x67, 0x00, 0x00, 0x0a, 0x34, 0x91,
      0x54, 0x89, 0x54, 0x8a, 0x60, 0xf2, 0x9f, 0xc1, 0x4e, 0x93, 0xdf, 0xc1,
      0x4a, 0x42, 0x67, 0x00, 0x00, 0x04, 0x20, 0x08, 0x4c, 0xdf, 0x0e, 0x06,
      0x4e, 0x5e, 0x4e, 0x75
      
   };/*m68k asm blob*/
   uint32_t oldArmRegisters[5];
   uint32_t returnValue;
   
   /*AAPCS calling convention*/
   /*on a function call R0<->R3 store the function arguments if they fit, extras will go on the stack*/
   /*first arg goes in R0, second in R1, ..., up to 4 args can go in registers*/
   /*on function return R0 and R1 store the return value of the last function, R1 is only used if the return value is 64 bit*/
   /*R4<->R7 are always local variables*/
   
   /*My dual CPU calling convention, called when ARM opcode 0xF7BBBBBB is run*/
   /*R0 = 0 is execution finished, R0 = 1 is run function*/
   /*when R0 = 1, R1 = function to execute, R2 = stack blob, R3 = stack blob size and want A0*/
   
   /*Fake ARM 32 bit alignment convention*/
   /*0x00000000<->0x7FFFFFFF Normal memory access*/
   /*0x80000000<->0xFFFFFFFF Mirrored range, realAddress = address - 0x80000000 + 0x00000002*/
   
   /*this will still fail if ARM allocates a 16 bit aligned buffer and passes it to the m68k,*/
   /*but it allows 16 bit aligned data to be executed from the m68k stack and works as a*/
   /*temporary measure until I get MemChunkNew patched correctly*/
   
   debugLog("Called ARM function:0x%08lX\n", (uint32_t)nativeFuncP);
   
   /*save old ARM state, needed if this function is called recursively*/
   /*(68k(normal mode)->PceNativeCall->...->Call68kFunc->...->PceNativeCall(this could corrupt old args if they arnt backed up!!!))*/
   oldArmRegisters[0] = armv5GetRegister(0);
   oldArmRegisters[1] = armv5GetRegister(1);
   oldArmRegisters[2] = armv5GetRegister(2);
   oldArmRegisters[3] = armv5GetRegister(14);
   oldArmRegisters[4] = armv5GetRegister(15);
   
   armv5SetRegister(0, 0xBADC0DE);/*emulStateP is unused*/
   armv5SetRegister(1, (uint32_t)userDataP);
   armv5SetRegister(2, (uint32_t)armCall68kFunc);
   
   armv5SetRegister(14, (uint32_t)armExitFunc);/*set link register to return location*/
   armv5SetRegister(15, (uint32_t)nativeFuncP);/*set program counter to function*/
   
   while(true){
      armv5Execute(100);/*run 100 opcodes, returns instantly with cycles remaining when service is needed*/
      if(armv5NeedsService()){
         /*ARM tried to call a 68k function or has finished executing*/
         if(armv5GetRegister(0)){
            /*call function*/
            /*uint32_t emulStateP = armv5GetRegister(0);*/
            uint32_t function = armv5GetRegister(1);
            uint32_t stackBlob = armv5GetRegister(2);
            uint32_t stackBlobSizeAndWantA0 = armv5GetRegister(3);
            uint32_t (*m68kCallWithBlobFuncPtr)(uint32_t functionAddress, uint32_t stackBlob, uint32_t stackBlobSize, uint16_t returnA0) = (uint32_t (*)(uint32_t, uint32_t, uint32_t, uint16_t))m68kCallWithBlobFunc;
            
            /*debug checks, I think these where preventing the callback to 68k code MemPtrNew???*/
            /*
            if(true){
               uint32_t index;
               uint32_t end = stackBlobSizeAndWantA0 & ~kPceNativeWantA0;
               
               debugLog("ARM calling 68k function:0x%08lX, stackBlob:0x%08lX, stackBlobSize:0x%08lX, wantA0:%d\n", function, stackBlob, stackBlobSizeAndWantA0 & ~kPceNativeWantA0, !!(stackBlobSizeAndWantA0 & kPceNativeWantA0));
               for(index = 0; index < end; index++)
                  debugLog("Stack byte:0x%02X\n", ((uint8_t*)stackBlob)[index]);
            }
            */
            /*debugLog("Called 68k function:0x%08lX\n", function);*/
            /*
            debugLog("ARM calling 68k: emulStateP:0x%08lX, function:0x%08lX, stackBlob:0x%08lX, stackBlobSize:0x%08lX, wantA0:%d\n", emulStateP, function, stackBlob, stackBlobSizeAndWantA0 & ~kPceNativeWantA0, !!(stackBlobSizeAndWantA0 & kPceNativeWantA0));
            */
             debugLog("ARM calling 68k function:0x%08lX, stackBlob:0x%08lX, stackBlobSize:0x%08lX, wantA0:%d\n", function, stackBlob, stackBlobSizeAndWantA0 & ~kPceNativeWantA0, !!(stackBlobSizeAndWantA0 & kPceNativeWantA0));
            
            /*API call, convert to address first*/
            if(function <= kPceNativeTrapNoMask)
               function = (uint32_t)TrapTablePtr[function];/*SysGetTrapAddress cant be used because this needs to handle OS 5 API calls too*/
            
            /*return whatever the 68k function did*/
            armv5SetRegister(0, m68kCallWithBlobFuncPtr(function, stackBlob, stackBlobSizeAndWantA0 & ~kPceNativeWantA0, !!(stackBlobSizeAndWantA0 & kPceNativeWantA0)));
         }
         else{
            /*execution is over*/
            break;
         }
      }
   }
   
   returnValue = armv5GetRegister(0);
   
   /*load old register state*/
   armv5SetRegister(0, oldArmRegisters[0]);
   armv5SetRegister(1, oldArmRegisters[1]);
   armv5SetRegister(2, oldArmRegisters[2]);
   armv5SetRegister(14, oldArmRegisters[3]);
   armv5SetRegister(15, oldArmRegisters[4]);
   
   debugLog("Finished ARM function:0x%08lX\n", (uint32_t)nativeFuncP);
   
   return returnValue;
}

UInt32 emuKeyCurrentState(void){
   /*need to call old KeyCurrentState then | wihth new keys*/
   return 0x00000000;
}

void emuErrDisplayFileLineMsg(const Char* const filename, UInt16 lineNo, const Char* const msg){
   debugLog("Error at:%s, Line:%d, Msg:%s\n", filename, lineNo, msg);
}

void emuSysUnimplemented(void){
   __return_stack_frame;
   uint32_t callLocation = getCallLocation();
   uint16_t apiNum = getCalledApiNum();
   
   debugLog("Unimplemented API:0x%04X, called from:0x%08lX\n", apiNum, callLocation);
}

Err emuHwrDisplayAttributes(Boolean set, UInt8 attribute, void* dataPtr){
   /*this function is exempt from formatting standards, it is meant as an exact C reconstrution of the Tungsten W HwrDisplayAttributes patched to work on an m515*/
   static const char lcdControllerName[] = "MQ11xx  LCD Controller";
   Err error = errNone;
   
   /*dispErrorClass | 0x01 = Cant Set???, also returned for null pointers*/
   /*dispErrorClass | 0x04 = Attribute Doesnt Exist??*/
   
   switch(attribute){
      case 0:
         /*display controler ID, MediaQ GPU*/
         (UInt32*)dataPtr = '12MQ';
         break;
         
      case 1:
         /*???*/
         (UInt16*)dataPtr = 1;
         break;
         
      case 2:
         /*???*/
         (UInt16*)dataPtr = 1;
         break;
         
      case 3:
         /*???*/
         (UInt16*)dataPtr = 0x808B;
         break;
         
      case 4:
         /*read some random RAM value and fail if 0*/
         if(ScrStatePtr)
            (UInt16*)dataPtr = 0x10;
         else
            error = dispErrorClass | 0x04;
         break;
         
      case 5:
         /*???*/
         (UInt16*)dataPtr = 8;
         break;
         
      case 6:
         /*???*/
         (UInt16*)dataPtr = 0x10;
         break;
         
      case 7:
         /*display size something, width???, PrvDisplaySize(Boolean set, void* dataPtr)*/
         /*UNTESTED*/
         (UInt16*)dataPtr = 320;
         break;
         
      case 8:
         /*display size something, height???, PrvDisplaySize(Boolean set, void* dataPtr)*/
         /*UNTESTED*/
         (UInt16*)dataPtr = 320;
         break;
         
      case 9:
         /*display chip framebuffer start address*/
         (UInt32*)dataPtr = 0x1F000000;
         break;
         
      case 10:
         /*display chip framebuffer size*/
         (UInt32*)dataPtr = 0x3C000;
         break;
         
      case 11:
         /*???*/
         (UInt16*)dataPtr = 1;
         break;
         
      case 12:
         /*full name of display controller*/
         StrCopy((char*)dataPtr, lcdControllerName);
         break;
         
      case 13:
         /*display base address*/
         /*UNTESTED*/
         (UInt32*)dataPtr = 0x1F000000;
         break;
         
      case 14:
         /*display depth*/
         /*UNTESTED*/
         (UInt16*)dataPtr = 16;
         break;
         
      case 15:
         /*read some random RAM value and write to dataPtr*/
         if(readArbitraryMemory8(0x36C))
            (UInt16*)dataPtr = 0x140;
         else
            (UInt16*)dataPtr = 0xA0;
         break;
         
      case 16:
         /*read some random RAM value and write to dataPtr*/
         if(readArbitraryMemory8(0x36C))
            (UInt16*)dataPtr = 0x140;
         else
            (UInt16*)dataPtr = 0xA0;
         break;
         
      case 17:
         /*display row bytes*/
         /*UNTESTED*/
         (UInt16*)dataPtr = 320 * sizeof(uint16_t);
         break;
         
      case 18:
         /*display backlight*/
         /*UNTESTED*/
         (UInt8*)dataPtr = 0x01;
         break;
         
      case 19:
         /*display contrast*/
         /*UNTESTED*/
         (UInt8*)dataPtr = 0x01;
         break;
         
      /*case 20: has no handler*/
         
      case 21:
         /*display debug indicator*/
         /*TODO, shouldnt be needed*/
         break;
         
      case 22:
         /*display controller chip framebuffer end address???*/
         if(set)
            error = dispErrorClass | 0x01;
         else
            (UInt32*)dataPtr = 0x1F03C000;
         break;
         
      /*case 23: has no handler*/
      /*case 24: has no handler*/
      /*case 25: has no handler*/
      /*case 26: has no handler*/
         
      case 27:
         /*???*/
         (UInt16*)dataPtr = 0x90;
         break;
         
      case 28:
         /*???*/
         (UInt16*)dataPtr = 0x40;
         break;
         
      case 29:
         /*???*/
         (UInt16*)dataPtr = 0x20;
         break;
         
      case 30:
         /*???*/
         (UInt8*)dataPtr = 1;
         break;
         
      default:
         debugLog("Invalid display attribute requested:%d\n", attribute);
         error = dispErrorClass | 0x04;
         break;
   }
   
   return error;
}

void emuScrDrawChars(WinPtr pWindow, Int16 xLoc, Int16 yLoc, Int16 xExtent, Int16 yExtent, Int16 clipTop, Int16 clipLeft, Int16 clipBottom, Int16 clipRight, Char* chars, UInt16 len, FontPtr fontPtr){
   /*do nothing*/
}

/*
may need to set this to change the border color like on OS 5
Boolean UIPickColor(IndexedColorType *indexP, RGBColorType *rgbP,
                    UIPickColorStartType start, const Char *titleP,
                    const Char *tipP)
SYS_TRAP(sysTrapUIPickColor);
*/

/*
 may need to remove these from trap table:
 ScrGetGrayPat
 ScrGetColortable
 ScrDrawChars
 ScrCopyRectangle
 */
