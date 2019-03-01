#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "debug.h"
#include "armv5.h"
#include "globals.h"
#include "palmGlobalDefines.h"


/*cant use global variables in this file!!!*/
/*functions in this file are called directly by the Palm OS trap dispatch this means they can be called when the app isnt loaded and the globals are just a random data buffer*/


UInt32 emuPceNativeCall(NativeFuncType* nativeFuncP, void* userDataP){
   /*AAPCS calling convention*/
   /*on a function call R0<->R3 store the function arguments if they fit, extras will go on the stack*/
   /*first arg goes in R0, second in R1, ..., up to 4 args can go in registers*/
   /*on function return R0 and R1 store the return value of the last function, R1 is only used if the return value is 64 bit*/
   /*R4<->R7 are always local variables*/
   
   /*My dual CPU calling convention, called when ARM opcode 0xF7BBBBBB is run*/
   /*R0 = 0 is execution finished, R0 = 1 is run function*/
   /*when R0 = 1, R1 = function to execute, R2 = stack blob, R3 = stack blob size and want A0*/
   
   /*these may need to be moved*/
   const ALIGN(4) uint32_t armExitFunc[] = {0x0000A0E3, 0xBBBBBBF7};/*ARM asm blob*/
   const ALIGN(4) uint32_t armCall68kFunc[] = {0x0100A0E3, 0xBBBBBBF7, 0x0EF0A0E1};/*ARM asm blob*/
   const ALIGN(2) uint8_t m68kCallWithBlobFunc[] = {0x4E, 0x56, 0x00, 0x00, 0x48, 0xE7, 0x60, 0x70, 0x26, 0x6E, 0x00, 0x08, 0x22, 0x6E, 0x00, 0x0C, 0x22, 0x2E, 0x00, 0x10, 0x34, 0x2E, 0x00, 0x14, 0x24, 0x4F, 0x95, 0xC1, 0xBF, 0xCA, 0x67, 0x00, 0x00, 0x0C, 0x34, 0x91, 0x54, 0x89, 0x54, 0x8A, 0x4E, 0xF8, 0x10, 0x4E, 0x9F, 0xC1, 0x4E, 0x93, 0xDF, 0xC1, 0x4A, 0x42, 0x67, 0x00, 0x00, 0x04, 0x20, 0x08, 0x4C, 0xDF, 0x0E, 0x06, 0x4E, 0x5E, 0x4E, 0x75};/*m68k asm blob*/
   uint32_t oldArmRegisters[5];
   uint32_t returnValue;
   
   debugLog("Called ARM function:0x%08lX\n", (uint32_t)nativeFuncP);
   
   /*save old ARM state, needed if this function is called recursively*/
   /*(68k(normal mode)->PceNativeCall->...->Call68kFunc->...->PceNativeCall(this could corrupt old args if they arnt backed up!!!))*/
   oldArmRegisters[0] = armv5GetRegister(0);
   oldArmRegisters[1] = armv5GetRegister(1);
   oldArmRegisters[2] = armv5GetRegister(2);
   oldArmRegisters[3] = armv5GetRegister(14);
   oldArmRegisters[4] = armv5GetRegister(15);
   
   armv5SetRegister(0, 0x0000000);/*emulStateP is unusesd*/
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
            uint32_t function = armv5GetRegister(1);
            uint32_t stackBlob = armv5GetRegister(2);
            uint32_t stackBlobSizeAndWantA0 = armv5GetRegister(3);
            uint32_t (*m68kCallWithBlobFuncPtr)(uint32_t functionAddress, uint32_t stackBlob, uint32_t stackBlobSize, uint16_t returnA0) = m68kCallWithBlobFunc;
            
            /*API call, convert to address first*/
            if(function < 0x1000)
               function = (uint32_t)SysGetTrapAddress(0xA000 | function);
            
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
   
   return returnValue;
}

UInt32 emuKeyCurrentState(void){
   /*need to call old KeyCurrentState then | wihth new keys*/
   return 0x00000000;
}

void emuErrDisplayFileLineMsg(const Char* const filename, UInt16 lineNo, const Char* const msg){
   debugLog("Error at:%s, Line:%d, Msg:%s\n", filename, lineNo, msg);
}

UInt32 emuHwrDisplayAttributes(UInt16 unknown1, UInt16 unknown2, UInt16 unknown3){
   
}
