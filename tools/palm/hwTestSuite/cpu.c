#include <PalmOS.h>
#include <stdint.h>

#include "testSuite.h"
#include "specs/dragonballVzRegisterSpec.h"
#include "viewer.h"
#include "cpu.h"


static char cpuStringBuffer[100];
static Boolean interruptsEnabled = true;
static uint16_t statusRegister;


void turnInterruptsOff(void){
   if(interruptsEnabled){
      statusRegister = SysDisableInts();
      interruptsEnabled = false;
   }
}

void turnInterruptsOn(void){
   if(!interruptsEnabled){
      SysRestoreStatus(statusRegister);
      interruptsEnabled = true;
   }
}

void wasteXOpcodes(uint32_t opcodes){
   volatile uint32_t blockOptimize = opcodes;
   while(blockOptimize)
      blockOptimize--;
}

uint8_t getPhysicalCpuType(void){
   uint32_t osVer;
   uint32_t dragonballType;
   
   FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osVer);
   if(osVer >= PalmOS50)
      return CPU_ARM;
   
   /*get dragonball CPU subtype*/
   FtrGet(sysFtrCreator, sysFtrNumProcessorID, &dragonballType);
   dragonballType &= sysFtrNumProcessorMask;
   dragonballType >>= 12;/*sysFtrNumProcessor* -> CPU_M68K_**/
   
   return dragonballType | CPU_M68K;
}

uint8_t getSupportedInstructionSets(void){
   if((getPhysicalCpuType() & CPU_ARM))
      return CPU_BOTH;
   return CPU_M68K;
}

const char* getCpuString(void){
   const char* cpuTypeNames[3] = {"None", "Dragonball ", "ARM(Type Unknown)"};
   const char* dragonballTypeNames[5] = {"", "328", "EZ", "VZ", "SZ"};
   uint8_t cpuModel = getPhysicalCpuType();
   uint8_t cpuType = cpuModel & CPU_TYPES;
   uint8_t dragonballType = (cpuModel & CPU_M68K_TYPES) >> 4;
   
   StrPrintF(cpuStringBuffer, "CPU:%s%s, InstructionSets:%s", cpuTypeNames[cpuType], dragonballTypeNames[dragonballType], cpuType == CPU_ARM ? "68K|ARM" : "68K");
   return cpuStringBuffer;
}
