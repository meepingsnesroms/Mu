#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "portability.h"
#include "specs/emuFeatureRegisterSpec.h"
#include "m515Bus.h"
#include "dbvzRegisters.h"
#include "silkscreen.h"
#include "sed1376.h"
#include "flx68000.h"
#include "debug/sandbox.h"


void expansionHardwareReset(void){

}

uint32_t expansionHardwareStateSize(void){
   uint32_t size = 0;

   return size;
}

void expansionHardwareSaveState(uint8_t* data){
   uint32_t offset = 0;

}

void expansionHardwareLoadState(uint8_t* data){
   uint32_t offset = 0;

}

uint32_t expansionHardwareGetRegister(uint32_t address){
   address &= 0x00000FFF;
   switch(address){
      case EMU_INFO:
         return palmEmuFeatures.info;

      case EMU_VALUE:
         return palmEmuFeatures.value;

      default:
         debugLog("Invalid read from emu register 0x%08X.\n", address);
         return 0x00000000;
   }
}

void expansionHardwareSetRegister(uint32_t address, uint32_t value){
   address &= 0x00000FFF;
   switch(address){
      case EMU_SRC:
         palmEmuFeatures.src = value;
         return;

      case EMU_DST:
         palmEmuFeatures.dst = value;
         return;

      case EMU_SIZE:
         palmEmuFeatures.size = value;
         return;

      case EMU_VALUE:
         palmEmuFeatures.value = value;
         return;

      case EMU_CMD:
         switch(value){
            case CMD_SET_CPU_SPEED:
               if(palmEmuFeatures.info & FEATURE_FAST_CPU)
                  palmClockMultiplier = (double)palmEmuFeatures.value / 100.0 * (1.00 - EMU_CPU_PERCENT_WAITING);
               return;

            case CMD_DEBUG_PRINT:
               if(palmEmuFeatures.info & FEATURE_DEBUG){
                  char tempString[200];
                  uint16_t offset;

                  for(offset = 0; offset < 200; offset++){
                     uint8_t newChar = flx68000ReadArbitraryMemory(palmEmuFeatures.src + offset, 8);//cant use char, if its signed < 128 will allow non ascii chars through

                     newChar = newChar < 128 ? newChar : '\0';
                     newChar = (newChar != '\t' && newChar != '\n') ? newChar : ' ';
                     tempString[offset] = newChar;

                     if(newChar == '\0')
                        break;
                  }

                  debugLog("CMD_PRINT: %s\n", tempString);
               }
               return;

            case CMD_DEBUG_WATCH:
               if(palmEmuFeatures.info & FEATURE_DEBUG){
                  if(palmEmuFeatures.value == SANDBOX_WATCH_NONE)
                     sandboxClearWatchRegion(palmEmuFeatures.src);
                  else
                     palmEmuFeatures.value = sandboxSetWatchRegion(palmEmuFeatures.src, palmEmuFeatures.size, palmEmuFeatures.value);
               }
               return;

            case CMD_DEBUG_EXEC_END:
               if(palmEmuFeatures.info & FEATURE_DEBUG)
                  sandboxReturn();
               return;

            default:
               debugLog("Invalid emu command 0x%04X.\n", value);
               return;
         }

      default:
         debugLog("Invalid write 0x%08X to emu register 0x%08X.\n", value, address);
         return;
   }
}
