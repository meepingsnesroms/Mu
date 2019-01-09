#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "armv5.h"
#include "globals.h"


/*cant use global variables in this file!!!*/
/*functions in this file are called directly by the Palm OS trap dispatch this means they can be called when the app isnt loaded and the globals are just a random data buffer*/


static uint32_t m68kCallWithBlobFunc(uint32_t functionAddress, uint32_t stackBlob, uint32_t stackBlobSize, uint16_t returnA0){
   register uint32_t d0 asm("d0");
   register uint32_t d1 asm("d1");
   register uint32_t d2 asm("d2");
   register uint32_t a0 asm("a0");
   register uint32_t a1 asm("a1");
   register uint32_t a2 asm("a2");
   register uint32_t a3 asm("a3");
   register uint32_t sp asm("sp");
   
   d1 = stackBlobSize;
   d2 = returnA0;
   
   a1 = stackBlob;
   a2 = sp - stackBlobSize;
   a3 = functionAddress;
   
   /*copy blob*/
   while(a2 != sp){
      *((uint16_t*)a2) = *((uint16_t*)a1);
      a1 += 2;
      a2 += 2;
   }
   
   /*call function*/
   ((void (*)(void))a3)();
   
   /*remove old args*/
   sp += d1;
   
   if(d2)
      return a0;
   
   return d0;
};

UInt32 emuPceNativeCall(NativeFuncType* nativeFuncP, void* userDataP){
   /*AAPCS calling convention*/
   /*on a function call R0<->R3 store the function arguments if they fit, extras will go on the stack*/
   /*first arg goes in R0, second in R1, ..., up to 4 args can go in registers*/
   /*on function return R0 and R1 store the return value of the last function, R1 is only used if the return value is 64 bit*/
   /*R4<->R7 are always local variables*/
   
   /*My dual CPU calling convention, called when ARM opcode 0xF7BBBBBB is run*/
   /*R4 = 0 is execution finished, R4 = 1 is run function*/
   /*when R4 = 1, R5 = function to execute, R6 = stack blob, R7 = stack blob size and want A0*/
   
   uint32_t exitFunc = getGlobalVar(ARM_EXIT_FUNC);/*points to an ARM asm snippet that stops ARM execution*/
   uint32_t call68kFunc = getGlobalVar(ARM_CALL_68K_FUNC);/*points to an ARM asm snippet that calls a 68k function*/
   uint32_t emulState = getGlobalVar(ARM_EMUL_STATE);/*required by OS 5 functions, but should be unused*/
   
   armv5SetRegister(0, emulState);
   armv5SetRegister(1, (uint32_t)userDataP);
   armv5SetRegister(2, call68kFunc);
   
   armv5SetRegister(14, exitFunc);/*set link register to return location*/
   armv5SetRegister(15, (uint32_t)nativeFuncP);/*set program counter to function*/
   
   while(true){
      armv5Execute(100);/*run 100 opcodes, returns instantly if service is needed*/
      if(armv5NeedsService()){
         /*ARM tried to call a 68k function or has finished executing*/
         if(armv5GetRegister(4)){
            /*call function*/
            uint32_t function = armv5GetRegister(5);
            uint32_t stackBlob = armv5GetRegister(6);
            uint32_t stackBlobSizeAndWantA0 = armv5GetRegister(7);
            
            /*API call, convert to address first*/
            if(function < 0x1000)
               function = (uint32_t)SysGetTrapAddress(0xA000 | function);
            
            /*return whatever the 68k function did*/
            armv5SetRegister(0, m68kCallWithBlobFunc(function, stackBlob, stackBlobSizeAndWantA0 & ~kPceNativeWantA0, !!(stackBlobSizeAndWantA0 & kPceNativeWantA0)));
         }
         else{
            /*execution is over*/
            break;
         }
      }
   }
   
   return armv5GetRegister(0);
}
