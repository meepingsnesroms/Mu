#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "armv5.h"
#include "palmGlobalDefines.h"
#include "specs/emuFeatureRegisterSpec.h"


#define SWAP_32(x) ((uint32_t)((((uint32_t)(x) & 0x000000FF) << 24) | (((uint32_t)(x) & 0x0000FF00) <<  8) | (((uint32_t)(x) & 0x00FF0000) >>  8) | (((uint32_t)(x) & 0xFF000000) >> 24)))


void armv5SetStack(uint8_t* location, uint32_t size){
   uint32_t firstStackEntry = (uint32_t)location + size - 1;

   /*must be 32 bit aligned, m68k only enforces 16 bit aligned even for 32 bit access so MemPtrNew may return a word aligned address*/
   while((firstStackEntry & 0x3) != 0)
      firstStackEntry--;
   
   armv5SetRegister(13, firstStackEntry);
}

void armv5StackPush(uint32_t value){
   uint32_t stackPtr = armv5GetRegister(13);
   
   stackPtr -= sizeof(uint32_t);
   writeArbitraryMemory32(stackPtr, SWAP_32(value));
   armv5SetRegister(13, stackPtr);
}

uint32_t armv5StackPop(void){
   uint32_t stackPtr = armv5GetRegister(13);
   uint32_t value = SWAP_32(readArbitraryMemory32(stackPtr));
   
   stackPtr += sizeof(uint32_t);
   armv5SetRegister(13, stackPtr);
   return value;
}

void armv5SetRegister(uint8_t reg, uint32_t value){
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_DST), reg);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE), value);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_ARM_SET_REG);
}

uint32_t armv5GetRegister(uint8_t reg){
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_SRC), reg);
   writeArbitraryMemory32(EMU_REG_ADDR(EMU_CMD), CMD_ARM_GET_REG);
   return readArbitraryMemory32(EMU_REG_ADDR(EMU_VALUE));
}
