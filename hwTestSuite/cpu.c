#include <PalmOS.h>
#include <stdint.h>
#include "testSuite.h"
#include "emuFunctions.h"
#include "cpu.h"


static ErrJumpBuf rebootJmpData;/*stores D3-D7,PC,A2-A7*/


static void crashHandler(){
   /*when this function is called the system has reset, all cpu registers are empty, last stack location is unknown*/
}

static void installCrashHandler(){
#if 0
   ErrSetJump(rebootJmpData);
   writeArbitraryMemory32(0x00000000, (long*)rebootJmpData);/*reboot stack pointer*/
   writeArbitraryMemory32(0x00000004, &crashHandler);/*reboot program counter*/
   writeArbitraryMemory32(0x00000008, &rebootJmpData);/*const "0xFEEDBEEF", replaced with pointer to rebootJmpData*/
#endif
}

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
      installCrashHandler();
      unsafeMode = true;
      return true;/*now in unsafe mode*/
   }
   return false;
}
