#include <PalmOS.h>
#include <stdint.h>

#include "testSuite.h"
#include "emuFunctions.h"
#include "specs/hardwareRegisterNames.h"
#include "viewer.h"
#include "cpu.h"


static char cpuStringBuffer[100];
static Boolean interruptsEnabled = true;
static uint16_t statusRegister;


void turnInterruptsOff(){
   if(interruptsEnabled){
      statusRegister = SysDisableInts();
      interruptsEnabled = false;
   }
}

void turnInterruptsOn(){
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

uint8_t getPhysicalCpuType(){
   uint32_t osVer;
   uint32_t dragonballType;
   
   if(isEmulator())
      return CPU_NONE;
   
   FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osVer);
   if(osVer >= PalmOS50)
      return CPU_ARM;
   
   /*get dragonball CPU subtype*/
   FtrGet(sysFtrCreator, sysFtrNumProcessorID, &dragonballType);
   dragonballType &= sysFtrNumProcessorMask;
   dragonballType >>= 12;/*sysFtrNumProcessor* -> CPU_M68K_**/
   
   return dragonballType | CPU_M68K;
}

uint8_t getSupportedInstructionSets(){
   /*emulated m68k on physical ARM or running both CPUs from emulator*/
   if((getPhysicalCpuType() & CPU_ARM) || isEmulator())
      return CPU_BOTH;
   return CPU_M68K;
}

const char* getCpuString(){
   const char* cpuTypeNames[3] = {"Emulator", "Dragonball ", "ARM(Type Unknown)"};
   const char* dragonballTypeNames[5] = {"", "328", "EZ", "VZ", "SZ"};
   uint8_t cpuModel = getPhysicalCpuType();
   uint8_t cpuType = cpuModel & CPU_TYPES;
   uint8_t dragonballType = (cpuModel & CPU_M68K_TYPES) >> 4;
   
   StrPrintF(cpuStringBuffer, "CPU:%s%s, InstructionSets:%s", cpuTypeNames[cpuType], dragonballTypeNames[dragonballType], (cpuType == CPU_ARM || isEmulatorFeatureEnabled(FEATURE_HYBRID_CPU)) ? "68k|arm" : "68k");
   return cpuStringBuffer;
}
