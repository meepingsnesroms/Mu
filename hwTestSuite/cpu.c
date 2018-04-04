#include <PalmOS.h>
#include <stdint.h>
#include "testSuite.h"
#include "emuFunctions.h"
#include "cpu.h"

#define rebootSafeData (*((volatile uint32_t*)0x00000008))/*can be used regardless of stack pointer or global variables state*/
static ErrJumpBuf rebootJmpData;/*stores D3,D4,D5,D6,D7,PC,A2,A3,A4,A5,A6,A7*/


uint8_t getPhysicalCpuType(){
   long     osVer;
   FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osVer);
   
   if(isEmulator())
      return CPU_NONE;
   else if(osVer >= PalmOS50)
      return CPU_ARM;
   return CPU_M68K;
}

uint8_t getSupportedInstructionSets(){
   if((getPhysicalCpuType() & CPU_ARM) || isEmulator()){
      /*emulated m68k on physical arm or running both cpus from emulator*/
      return CPU_BOTH;
   }
   return CPU_M68K;
}

char* getCpuString(){
   return "";
}

Boolean enterUnsafeMode(){
   if((getPhysicalCpuType() & CPU_M68K) || isEmulator()){
      unsafeMode = true;
      return true;/*now in unsafe mode*/
   }
   return false;
}

var runUnsafeCall(){
   /*this may crash the OS, but thats ok*/
   var callLocation = getSubprogramArgs();
   if(unsafeMode && getVarType(callLocation) == TYPE_PTR){
      activity_t unsafeFunction = (activity_t)getVarPointer(callLocation);
      int16_t setJumpDirection;
      
      setJumpDirection = ErrSetJump(rebootJmpData);
      if(setJumpDirection == 0){
         /*unsafe call not made yet*/
         rebootSafeData = (uint32_t)&rebootJmpData;
         writeArbitraryMemory32(0x00000000, (uint32_t)rebootJmpData[9]);/*reboot stack pointer*/
         writeArbitraryMemory32(0x00000004, (uint32_t)rebootJmpData[5]);/*reboot program counter*/
         
         /*setJumpDirection needs to be 1 after the crash, this function is not finished*/
         /*may need to disassemble ErrLongJump*/
         
         execSubprogram(unsafeFunction);
      }
      else if(setJumpDirection == 1){
         /*the OS crashed from the unsafe call, restore remaining registers*/
         /*the stack pointer was restored to before the crash on reboot*/
         ErrLongJump(*((ErrJumpBuf*)rebootSafeData), 2);
      }
      else if(setJumpDirection == 2){
         /*registers are back to normal, continue execution*/
         exitSubprogram();
      }
   }
}
