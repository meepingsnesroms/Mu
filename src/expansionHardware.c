#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "portability.h"
#include "specs/emuFeatureRegisterSpec.h"
#include "memoryAccess.h"
#include "hardwareRegisters.h"
#include "silkscreen.h"
#include "sed1376.h"
#include "flx68000.h"
#include "armv5.h"
#include "debug/sandbox.h"


static uint32_t expansionHardwareLcdPointer;


static bool expansionHardwareBufferInRam(uint32_t address, uint32_t size){
   if(address >= chips[CHIP_DX_RAM].start && address < chips[CHIP_DX_RAM].start + chips[CHIP_DX_RAM].lineSize)
      if(address + size >= chips[CHIP_DX_RAM].start && address + size < chips[CHIP_DX_RAM].start + chips[CHIP_DX_RAM].lineSize)
         return true;

   return false;
}

void expansionHardwareReset(void){
   expansionHardwareLcdPointer = 0x00000000;

   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
      armv5Reset();
}

uint32_t expansionHardwareStateSize(void){
   uint32_t size = 0;

   size += sizeof(uint32_t);
   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
      size += armv5StateSize();

   return size;
}

void expansionHardwareSaveState(uint8_t* data){
   uint32_t offset = 0;

   writeStateValue32(data + offset, expansionHardwareLcdPointer);
   offset += sizeof(uint32_t);
   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU){
      armv5SaveState(data + offset);
      offset += armv5StateSize();
   }
}

void expansionHardwareLoadState(uint8_t* data){
   uint32_t offset = 0;

   expansionHardwareLcdPointer = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU){
      armv5LoadState(data + offset);
      offset += armv5StateSize();
   }
}

void expansionHardwareRenderAudio(void){
   /*audio streams will allow the max amount of Palm OS 5 audio streams + 1 for PWM1, each stream will act like an OS 5 audio stream and PWM1 will be the last stream*/

   uint32_t samples;

   blip_end_frame(palmAudioResampler, blip_clocks_needed(palmAudioResampler, AUDIO_SAMPLES_PER_FRAME));
   blip_read_samples(palmAudioResampler, palmAudio, AUDIO_SAMPLES_PER_FRAME, true);
   MULTITHREAD_LOOP(samples) for(samples = 0; samples < AUDIO_SAMPLES_PER_FRAME * 2; samples += 2)
      palmAudio[samples + 1] = palmAudio[samples];

   //DRIVER NEEDS TO BE WRITTEN STILL
}

void expansionHardwareRenderDisplay(void){
   if(palmFramebufferWidth == 160 && palmFramebufferHeight == 220){
      //simple render
      sed1376Render();
      memcpy(palmFramebuffer, sed1376Framebuffer, 160 * 160 * sizeof(uint16_t));
      memcpy(palmFramebuffer + 160 * 160, silkscreen160x60, 160 * 60 * sizeof(uint16_t));
   }
   else{
      //advanced render
      uint16_t* lcdData = palmRam + expansionHardwareLcdPointer;

      //copy over framebuffer
      memcpy(palmFramebuffer, lcdData, palmFramebufferWidth * palmFramebufferHeight * sizeof(uint16_t));

      //add silkscreen if needed
      if(palmFramebufferWidth == 320 && palmFramebufferHeight == 440)
         memcpy(palmFramebuffer + 320 * 320, silkscreen320x120, 320 * 120 * sizeof(uint16_t));
   }
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

            case CMD_ARM_SERVICE:
               if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
                  palmEmuFeatures.value = armv5ServiceRequest;
               return;

            case CMD_ARM_SET_REG:
               if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
                  armv5SetRegister(palmEmuFeatures.dst, palmEmuFeatures.value);
               return;

            case CMD_ARM_GET_REG:
               if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
                  palmEmuFeatures.value = armv5GetRegister(palmEmuFeatures.src);
               return;

            case CMD_ARM_RUN:
               if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
                  palmEmuFeatures.value = armv5Execute(palmEmuFeatures.value);
               return;

            case CMD_LCD_SET_FB:
               if(palmEmuFeatures.info & FEATURE_CUSTOM_FB){
                  uint16_t width = palmEmuFeatures.value >> 16;
                  uint16_t height = palmEmuFeatures.value & 0xFFFF;
                  uint32_t size = width * height * sizeof(uint16_t);

                  if(expansionHardwareBufferInRam(palmEmuFeatures.src, size) && width > 0 && height > 0 && width <= 480 && height <= 480){
                     expansionHardwareLcdPointer = palmEmuFeatures.src;
                     palmFramebufferWidth = width;
                     palmFramebufferHeight = height;
                     debugLog("Set LCD to:%dx%d, buffer:0x%08X\n", palmFramebufferWidth, palmFramebufferHeight, expansionHardwareLcdPointer);
                  }
                  else{
                     debugLog("Invalid LCD set command: dimensions:%dx%d, address:0x%08X, size:0x%08X\n", width, height, palmEmuFeatures.src, size);
                  }
               }
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

            case CMD_GET_KEYS:
               if(palmEmuFeatures.info & FEATURE_EXT_KEYS)
                  palmEmuFeatures.value = (palmInput.buttonLeft ? EXT_BUTTON_LEFT : 0) | (palmInput.buttonRight ? EXT_BUTTON_RIGHT : 0) | (palmInput.buttonCenter ? EXT_BUTTON_CENTER : 0);
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
