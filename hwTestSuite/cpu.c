#include <PalmOS.h>
#include <stdint.h>
#include "testSuite.h"
#include "emuFunctions.h"
#include "hardwareRegisterNames.h"
#include "viewer.h"
#include "cpu.h"


static char cpuStringBuffer[100];


uint8_t getPhysicalCpuType(){
   long     osVer;
   FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osVer);
   
   if(isEmulator())
      return CPU_NONE;
   else if(osVer >= PalmOS50)
      return CPU_ARM;
   
   /*get dragonball CPU subtype*/
   
   return CPU_M68K;
}

uint8_t getSupportedInstructionSets(){
   if((getPhysicalCpuType() & CPU_ARM) || isEmulator()){
      /*emulated m68k on physical ARM or running both CPUs from emulator*/
      return CPU_BOTH;
   }
   return CPU_M68K;
}

char* getCpuString(){
   uint8_t cpuType = getPhysicalCpuType() & CPU_TYPES;
   
   if(cpuType == CPU_NONE){
      StrPrintF(cpuStringBuffer, "CPU:Emulator, InstructionSets:%s", isEmulatorFeatureEnabled(FEATURE_HYBRID_CPU) ? "68k|arm" : "68k");
   }
   else if(cpuType == CPU_ARM){
      StrPrintF(cpuStringBuffer, "CPU:ARM(Type Unknown), InstructionSets:68k|arm");
   }
   else if(cpuType == CPU_M68K){
      uint8_t dragonballSubtype = getPhysicalCpuType() & CPU_M68K_TYPES;
      char* dragonballTypeName;
      switch(dragonballSubtype){
            
         case CPU_M68K_328:
            dragonballTypeName = "328";
            break;
            
         case CPU_M68K_EZ:
            dragonballTypeName = "EZ";
            break;
            
         case CPU_M68K_VZ:
            dragonballTypeName = "VZ";
            break;
            
         case CPU_M68K_SZ:
            dragonballTypeName = "SZ";
            break;
      }
      
      /*add die revision here*/
      
      StrPrintF(cpuStringBuffer, "CPU:Dragonball %s, InstructionSets:68k", dragonballTypeName);
   }
   
   return cpuStringBuffer;
}

var enterUnsafeMode(){
   exitSubprogram();/*only run once/for one frame*/
   
   if((getPhysicalCpuType() & CPU_M68K) || isEmulator()){
      /*all memory banks set to read/write and unprotected*/
      /*
      writeArbitraryMemory16(HW_REG_ADDR(CSA), readArbitraryMemory16(HW_REG_ADDR(CSA)) & 0x7FFF);
      writeArbitraryMemory16(HW_REG_ADDR(CSB), readArbitraryMemory16(HW_REG_ADDR(CSB)) & 0x7FFF);
      writeArbitraryMemory16(HW_REG_ADDR(CSC), readArbitraryMemory16(HW_REG_ADDR(CSC)) & 0x7FFF);
      writeArbitraryMemory16(HW_REG_ADDR(CSD), readArbitraryMemory16(HW_REG_ADDR(CSD)) & 0x7FFF);
      */
      
      /*disable interrupt on invalid memory access*/
      writeArbitraryMemory8(HW_REG_ADDR(SCR), readArbitraryMemory8(HW_REG_ADDR(SCR)) & 0xEF);
      
      unsafeMode = true;
      resetFunctionViewer();
      return makeVar(LENGTH_1, TYPE_BOOL, true);/*now in unsafe mode*/
   }

   return makeVar(LENGTH_1, TYPE_BOOL, false);
}
