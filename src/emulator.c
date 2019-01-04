#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "audio/blip_buf.h"
#include "flx68000.h"
#include "armv5.h"
#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "sed1376.h"
#include "ads7846.h"
#include "pdiUsbD12.h"
#include "sdCard.h"
#include "silkscreen.h"
#include "portability.h"
#include "debug/sandbox.h"
#include "specs/emuFeatureRegisterSpec.h"


//Memory Map of Palm m515
//0x00000000-0x00FFFFFF RAM, CSC0 as RAS0, CSC1 as RAS1, CSD0 as CAS0 and CSD1 as CAS1
//0x10000000-0x103FFFFF ROM, CSA0, at address 0x00000000 during boot, palmos41-en-m515.rom, substitute "en" for your language code
//0x10400000-0x10C00000 USB, CSA1
//0x1FF80000-0x1FF800B3 SED1376(Display Controller) Registers, CSB0
//0x1FFA0000-0x1FFB3FFF SED1376(Display Controller) Framebuffer, CSB0, this is not the same as the Palm framebuffer which is always 16 bit color,
//this buffer must be processed depending on whats in the SED1376 registers, the result is the Palm framebuffer
//0xFFFFF000-0xFFFFFDFF Hardware Registers
//0xFFFFFE00-0xFFFFFFFF Bootloader, only reads from UART into RAM and jumps to it, never executed in consumer Palms
//VGhpcyBlbXVsYXRvciBpcyBkZWRpY2F0ZWQgdG8gdGhlIGJvdmluZSBtb28gY293cyB0aGF0IG1vby4=


static bool emulatorInitialized = false;

uint8_t*  palmRam;
uint8_t*  palmRom;
uint8_t*  palmReg;
input_t   palmInput;
sd_card_t palmSdCard;
misc_hw_t palmMisc;
emu_reg_t palmEmuFeatures;
uint16_t* palmFramebuffer;
uint16_t  palmFramebufferWidth;
uint16_t  palmFramebufferHeight;
int16_t*  palmAudio;
blip_t*   palmAudioResampler;
double    palmSysclksPerClk32;//how many SYSCLK cycles before toggling the 32.768 kHz crystal
double    palmCycleCounter;//can be greater then 0 if too many cycles where run
double    palmClockMultiplier;//used by the emulator to overclock the emulated Palm
uint32_t  palmFrameClk32s;//how many CLK32s have happened in the current frame
double    palmClk32Sysclks;//how many SYSCLKs have happened in the current CLK32


uint32_t emulatorInit(buffer_t palmRomDump, buffer_t palmBootDump, uint32_t enabledEmuFeatures){
   if(emulatorInitialized)
      return EMU_ERROR_RESOURCE_LOCKED;

   if(!palmRomDump.data)
      return EMU_ERROR_INVALID_PARAMETER;

   //allocate buffers, add 4 to memory regions to prevent SIGSEGV from accessing off the end
   palmRam = malloc(((enabledEmuFeatures & FEATURE_RAM_HUGE) ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE) + 4);
   palmRom = malloc(ROM_SIZE + 4);
   palmReg = malloc(REG_SIZE + 4);
   palmFramebuffer = malloc(480 * 480 * sizeof(uint16_t));
   palmAudio = malloc(AUDIO_SAMPLES_PER_FRAME * 2 * sizeof(int16_t));
   palmAudioResampler = blip_new(AUDIO_SAMPLE_RATE);//have 1 second of samples
   if(!palmRam || !palmRom || !palmReg || !palmFramebuffer || !palmAudio || !palmAudioResampler){
      free(palmRam);
      free(palmRom);
      free(palmReg);
      free(palmFramebuffer);
      free(palmAudio);
      blip_delete(palmAudioResampler);
      return EMU_ERROR_OUT_OF_MEMORY;
   }

   //set default values
   memset(palmRam, 0x00, enabledEmuFeatures & FEATURE_RAM_HUGE ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE);
   memcpy(palmRom, palmRomDump.data, u64Min(palmRomDump.size, ROM_SIZE));
   if(palmRomDump.size < ROM_SIZE)
      memset(palmRom + palmRomDump.size, 0x00, ROM_SIZE - palmRomDump.size);
   swap16BufferIfLittle(palmRom, ROM_SIZE / sizeof(uint16_t));
   if(palmBootDump.data){
      memcpy(palmReg + REG_SIZE - 1 - BOOTLOADER_SIZE, palmBootDump.data, u64Min(palmBootDump.size, BOOTLOADER_SIZE));
      if(palmBootDump.size < BOOTLOADER_SIZE)
         memset(palmReg + REG_SIZE - 1 - BOOTLOADER_SIZE + palmBootDump.size, 0x00, BOOTLOADER_SIZE - palmBootDump.size);
      swap16BufferIfLittle(palmReg + REG_SIZE - 1 - BOOTLOADER_SIZE, BOOTLOADER_SIZE / sizeof(uint16_t));
   }
   else{
      memset(palmReg + REG_SIZE - 1 - BOOTLOADER_SIZE, 0x00, BOOTLOADER_SIZE);
   }
   memset(palmAudio, 0x00, AUDIO_SAMPLES_PER_FRAME * 2/*channels*/ * sizeof(int16_t));
   memset(&palmInput, 0x00, sizeof(palmInput));
   memset(&palmMisc, 0x00, sizeof(palmMisc));
   memset(&palmEmuFeatures, 0x00, sizeof(palmEmuFeatures));
   palmFramebufferWidth = 160;
   palmFramebufferHeight = 220;
   palmMisc.batteryLevel = 100;
   palmCycleCounter = 0.0;
   palmClockMultiplier = (enabledEmuFeatures & FEATURE_FAST_CPU) ? 2.00 : 1.00;//overclock
   palmClockMultiplier *= 0.70;//account for wait states when reading memory, tested with SysInfo.prc
   palmEmuFeatures.info = enabledEmuFeatures;

   //initialize components
   blip_set_rates(palmAudioResampler, AUDIO_CLOCK_RATE, AUDIO_SAMPLE_RATE);
   flx68000Init();
   sandboxInit(); 

   //reset everything
   sed1376Reset();
   ads7846Reset();
   pdiUsbD12Reset();
   sdCardReset();
   if(enabledEmuFeatures & FEATURE_HYBRID_CPU)
      armv5Reset();
   flx68000Reset();
   setRtc(0, 0, 0, 0);//RTCTIME and DAYR are not cleared by reset, clear them manually in case the frontend doesnt set the RTC

   emulatorInitialized = true;

   //hack, patch ROM image
   sandboxCommand(SANDBOX_PATCH_OS, NULL);

   return EMU_ERROR_NONE;
}

void emulatorExit(void){
   if(emulatorInitialized){
      free(palmRam);
      free(palmRom);
      free(palmReg);
      free(palmFramebuffer);
      free(palmAudio);
      blip_delete(palmAudioResampler);
      free(palmSdCard.flashChip.data);
      emulatorInitialized = false;
   }
}

void emulatorHardReset(void){
   //equivalent to taking the battery out and putting it back in
   memset(palmRam, 0x00, palmEmuFeatures.info & FEATURE_RAM_HUGE ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE);
   palmFramebufferWidth = 160;
   palmFramebufferHeight = 220;
   palmEmuFeatures.value = 0x00000000;
   sed1376Reset();
   ads7846Reset();
   pdiUsbD12Reset();
   sdCardReset();
   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
      armv5Reset();
   flx68000Reset();
   setRtc(0, 0, 0, 0);
}

void emulatorSoftReset(void){
   //equivalent to pushing the reset button on the back of the device
   palmFramebufferWidth = 160;
   palmFramebufferHeight = 220;
   palmEmuFeatures.value = 0x00000000;
   sed1376Reset();
   ads7846Reset();
   pdiUsbD12Reset();
   sdCardReset();
   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
      armv5Reset();
   flx68000Reset();
}

void emulatorSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
   setRtc(days, hours, minutes, seconds);
}

uint64_t emulatorGetStateSize(void){
   uint64_t size = 0;

   size += sizeof(uint32_t);//save state version
   size += sizeof(uint32_t);//palmEmuFeatures.info
   size += sizeof(uint64_t);//palmSdCard.flashChip.size, needs to be done first to verify the malloc worked
   size += sizeof(uint16_t) * 2;//palmFramebuffer(Width/Height)
   size += flx68000StateSize();
   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU)
      size += armv5StateSize();
   size += sed1376StateSize();
   size += ads7846StateSize();
   size += pdiUsbD12StateSize();
   if(palmEmuFeatures.info & FEATURE_RAM_HUGE)
      size += SUPERMASSIVE_RAM_SIZE;//system RAM buffer
   else
      size += RAM_SIZE;//system RAM buffer
   size += REG_SIZE;//hardware registers
   size += TOTAL_MEMORY_BANKS;//bank handlers
   size += sizeof(uint32_t) * 4 * CHIP_END;//chip select states
   size += sizeof(uint8_t) * 5 * CHIP_END;//chip select states
   size += sizeof(uint64_t) * 4;//32.32 fixed point double, timerXCycleCounter and CPU cycle timers
   size += sizeof(int8_t);//pllSleepWait
   size += sizeof(int8_t);//pllWakeWait
   size += sizeof(uint32_t);//clk32Counter
   size += sizeof(uint64_t);//pctlrCpuClockDivider
   size += sizeof(uint16_t) * 2;//timerStatusReadAcknowledge
   size += sizeof(uint8_t);//portDInterruptLastValue
   size += sizeof(uint16_t) * 9;//RX 8 * 16 SPI1 FIFO, 1 index is for FIFO full
   size += sizeof(uint16_t) * 9;//TX 8 * 16 SPI1 FIFO, 1 index is for FIFO full
   size += sizeof(uint8_t) * 5;//spi1(R/T)x(Read/Write)Position / spi1RxOverflowed
   size += sizeof(int32_t);//pwm1ClocksToNextSample
   size += sizeof(uint8_t) * 6;//pwm1Fifo[6]
   size += sizeof(uint8_t) * 2;//pwm1(Read/Write)
   size += sizeof(uint8_t) * 7;//palmMisc
   size += sizeof(uint32_t) * 4;//palmEmuFeatures.src / palmEmuFeatures.dst / palmEmuFeatures.size / palmEmuFeatures.value
   size += sizeof(uint64_t) * 2;//palmSdCard.command / palmSdCard.responseState
   size += sizeof(uint8_t) * 4;//palmSdCard.commandBitsRemaining / palmSdCard.response / palmSdCard.allowInvalidCrc / palmSdCard.chipSelect
   size += palmSdCard.flashChip.size;//palmSdCard.flashChip.data

   return size;
}

bool emulatorSaveState(buffer_t buffer){
   uint64_t offset = 0;
   uint8_t index;

   if(buffer.size < emulatorGetStateSize())
      return false;//state cant fit

   //state validation, wont load states that are not from the same state version
   writeStateValue32(buffer.data + offset, SAVE_STATE_VERSION);
   offset += sizeof(uint32_t);

   //features, hotpluging emulated hardware is not supported
   writeStateValue32(buffer.data + offset, palmEmuFeatures.info);
   offset += sizeof(uint32_t);

   //SD card size
   writeStateValue64(buffer.data + offset, palmSdCard.flashChip.size);
   offset += sizeof(uint64_t);

   //screen state
   writeStateValue16(buffer.data + offset, palmFramebufferWidth);
   offset += sizeof(uint16_t);
   writeStateValue16(buffer.data + offset, palmFramebufferHeight);
   offset += sizeof(uint16_t);

   //chips
   flx68000SaveState(buffer.data + offset);
   offset += flx68000StateSize();
   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU){
      armv5SaveState(buffer.data + offset);
      offset += armv5StateSize();
   }
   sed1376SaveState(buffer.data + offset);
   offset += sed1376StateSize();
   ads7846SaveState(buffer.data + offset);
   offset += ads7846StateSize();
   pdiUsbD12SaveState(buffer.data + offset);
   offset += pdiUsbD12StateSize();

   //memory
   if(palmEmuFeatures.info & FEATURE_RAM_HUGE){
      memcpy(buffer.data + offset, palmRam, SUPERMASSIVE_RAM_SIZE);
      swap16BufferIfLittle(buffer.data + offset, SUPERMASSIVE_RAM_SIZE / sizeof(uint16_t));
      offset += SUPERMASSIVE_RAM_SIZE;
   }
   else{
      memcpy(buffer.data + offset, palmRam, RAM_SIZE);
      swap16BufferIfLittle(buffer.data + offset, RAM_SIZE / sizeof(uint16_t));
      offset += RAM_SIZE;
   }
   memcpy(buffer.data + offset, palmReg, REG_SIZE);
   swap16BufferIfLittle(buffer.data + offset, REG_SIZE / sizeof(uint16_t));
   offset += REG_SIZE;
   memcpy(buffer.data + offset, bankType, TOTAL_MEMORY_BANKS);
   offset += TOTAL_MEMORY_BANKS;
   for(index = CHIP_BEGIN; index < CHIP_END; index++){
      writeStateValue8(buffer.data + offset, chips[index].enable);
      offset += sizeof(uint8_t);
      writeStateValue32(buffer.data + offset, chips[index].start);
      offset += sizeof(uint32_t);
      writeStateValue32(buffer.data + offset, chips[index].lineSize);
      offset += sizeof(uint32_t);
      writeStateValue32(buffer.data + offset, chips[index].mask);
      offset += sizeof(uint32_t);
      writeStateValue8(buffer.data + offset, chips[index].inBootMode);
      offset += sizeof(uint8_t);
      writeStateValue8(buffer.data + offset, chips[index].readOnly);
      offset += sizeof(uint8_t);
      writeStateValue8(buffer.data + offset, chips[index].readOnlyForProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValue8(buffer.data + offset, chips[index].supervisorOnlyProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValue32(buffer.data + offset, chips[index].unprotectedSize);
      offset += sizeof(uint32_t);
   }

   //timing
   writeStateValueDouble(buffer.data + offset, palmSysclksPerClk32);
   offset += sizeof(uint64_t);
   writeStateValueDouble(buffer.data + offset, palmCycleCounter);
   offset += sizeof(uint64_t);
   writeStateValue8(buffer.data + offset, pllSleepWait);
   offset += sizeof(int8_t);
   writeStateValue8(buffer.data + offset, pllWakeWait);
   offset += sizeof(int8_t);
   writeStateValue32(buffer.data + offset, clk32Counter);
   offset += sizeof(uint32_t);
   writeStateValueDouble(buffer.data + offset, pctlrCpuClockDivider);
   offset += sizeof(uint64_t);
   writeStateValueDouble(buffer.data + offset, timerCycleCounter[0]);
   offset += sizeof(uint64_t);
   writeStateValueDouble(buffer.data + offset, timerCycleCounter[1]);
   offset += sizeof(uint64_t);
   writeStateValue16(buffer.data + offset, timerStatusReadAcknowledge[0]);
   offset += sizeof(uint16_t);
   writeStateValue16(buffer.data + offset, timerStatusReadAcknowledge[1]);
   offset += sizeof(uint16_t);
   writeStateValue8(buffer.data + offset, portDInterruptLastValue);
   offset += sizeof(uint8_t);

   //SPI1
   for(index = 0; index < 9; index++){
      writeStateValue16(buffer.data + offset, spi1RxFifo[index]);
      offset += sizeof(uint16_t);
   }
   for(index = 0; index < 9; index++){
      writeStateValue16(buffer.data + offset, spi1TxFifo[index]);
      offset += sizeof(uint16_t);
   }
   writeStateValue8(buffer.data + offset, spi1RxReadPosition);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, spi1RxWritePosition);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, spi1RxOverflowed);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, spi1TxReadPosition);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, spi1TxWritePosition);
   offset += sizeof(uint8_t);

   //PWM1, audio
   writeStateValue32(buffer.data + offset, pwm1ClocksToNextSample);
   offset += sizeof(int32_t);
   for(index = 0; index < 6; index++){
      writeStateValue8(buffer.data + offset, pwm1Fifo[index]);
      offset += sizeof(uint8_t);
   }
   writeStateValue8(buffer.data + offset, pwm1ReadPosition);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, pwm1WritePosition);
   offset += sizeof(uint8_t);

   //misc
   writeStateValue8(buffer.data + offset, palmMisc.powerButtonLed);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmMisc.lcdOn);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmMisc.backlightLevel);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmMisc.vibratorOn);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmMisc.batteryCharging);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmMisc.batteryLevel);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmMisc.dataPort);
   offset += sizeof(uint8_t);

   //emu features
   writeStateValue32(buffer.data + offset, palmEmuFeatures.src);
   offset += sizeof(uint32_t);
   writeStateValue32(buffer.data + offset, palmEmuFeatures.dst);
   offset += sizeof(uint32_t);
   writeStateValue32(buffer.data + offset, palmEmuFeatures.size);
   offset += sizeof(uint32_t);
   writeStateValue32(buffer.data + offset, palmEmuFeatures.value);
   offset += sizeof(uint32_t);

   //SD card
   writeStateValue64(buffer.data + offset, palmSdCard.command);
   offset += sizeof(uint64_t);
   writeStateValue8(buffer.data + offset, palmSdCard.commandBitsRemaining);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmSdCard.response);
   offset += sizeof(uint8_t);
   writeStateValue64(buffer.data + offset, palmSdCard.responseState);
   offset += sizeof(uint64_t);
   writeStateValue8(buffer.data + offset, palmSdCard.allowInvalidCrc);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmSdCard.chipSelect);
   offset += sizeof(uint8_t);
   memcpy(buffer.data + offset, palmSdCard.flashChip.data, palmSdCard.flashChip.size);
   offset += palmSdCard.flashChip.size;

   return true;
}

bool emulatorLoadState(buffer_t buffer){
   uint64_t offset = 0;
   uint8_t index;
   uint64_t stateSdCardSize;
   uint8_t* stateSdCardBuffer;

   //state validation, wont load states that are not from the same state version
   if(readStateValue32(buffer.data + offset) != SAVE_STATE_VERSION)
      return false;
   offset += sizeof(uint32_t);

   //features, hotpluging emulated hardware is not supported
   if(readStateValue32(buffer.data + offset) != palmEmuFeatures.info)
      return false;
   offset += sizeof(uint32_t);

   //SD card size, the malloc when loading can make it fail, make sure if it fails the emulator state doesnt change
   stateSdCardSize = readStateValue64(buffer.data + offset);
   stateSdCardBuffer = stateSdCardSize > 0 ? malloc(stateSdCardSize) : NULL;
   if(stateSdCardSize > 0 && !stateSdCardBuffer)
      return false;
   offset += sizeof(uint64_t);

   //screen state
   palmFramebufferWidth = readStateValue16(buffer.data + offset);
   offset += sizeof(uint16_t);
   palmFramebufferHeight = readStateValue16(buffer.data + offset);
   offset += sizeof(uint16_t);

   //chips
   flx68000LoadState(buffer.data + offset);
   offset += flx68000StateSize();
   if(palmEmuFeatures.info & FEATURE_HYBRID_CPU){
      armv5LoadState(buffer.data + offset);
      offset += armv5StateSize();
   }
   sed1376LoadState(buffer.data + offset);
   offset += sed1376StateSize();
   ads7846LoadState(buffer.data + offset);
   offset += ads7846StateSize();
   pdiUsbD12LoadState(buffer.data + offset);
   offset += pdiUsbD12StateSize();

   //memory
   if(palmEmuFeatures.info & FEATURE_RAM_HUGE){
      memcpy(palmRam, buffer.data + offset, SUPERMASSIVE_RAM_SIZE);
      swap16BufferIfLittle(palmRam, SUPERMASSIVE_RAM_SIZE / sizeof(uint16_t));
      offset += SUPERMASSIVE_RAM_SIZE;
   }
   else{
      memcpy(palmRam, buffer.data + offset, RAM_SIZE);
      swap16BufferIfLittle(palmRam, RAM_SIZE / sizeof(uint16_t));
      offset += RAM_SIZE;
   }
   memcpy(palmReg, buffer.data + offset, REG_SIZE);
   swap16BufferIfLittle(palmReg, REG_SIZE / sizeof(uint16_t));
   offset += REG_SIZE;
   memcpy(bankType, buffer.data + offset, TOTAL_MEMORY_BANKS);
   offset += TOTAL_MEMORY_BANKS;
   for(index = CHIP_BEGIN; index < CHIP_END; index++){
      chips[index].enable = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[index].start = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
      chips[index].lineSize = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
      chips[index].mask = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
      chips[index].inBootMode = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[index].readOnly = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[index].readOnlyForProtectedMemory = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[index].supervisorOnlyProtectedMemory = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[index].unprotectedSize = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
   }

   //timing
   palmSysclksPerClk32 = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   palmCycleCounter = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   pllSleepWait = readStateValue8(buffer.data + offset);
   offset += sizeof(int8_t);
   pllWakeWait = readStateValue8(buffer.data + offset);
   offset += sizeof(int8_t);
   clk32Counter = readStateValue32(buffer.data + offset);
   offset += sizeof(uint32_t);
   pctlrCpuClockDivider = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   timerCycleCounter[0] = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   timerCycleCounter[1] = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   timerStatusReadAcknowledge[0] = readStateValue16(buffer.data + offset);
   offset += sizeof(uint16_t);
   timerStatusReadAcknowledge[1] = readStateValue16(buffer.data + offset);
   offset += sizeof(uint16_t);
   portDInterruptLastValue = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);

   //SPI1
   for(index = 0; index < 9; index++){
      spi1RxFifo[index] = readStateValue16(buffer.data + offset);
      offset += sizeof(uint16_t);
   }
   for(index = 0; index < 9; index++){
      spi1TxFifo[index] = readStateValue16(buffer.data + offset);
      offset += sizeof(uint16_t);
   }
   spi1RxReadPosition = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   spi1RxWritePosition = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   spi1RxOverflowed = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   spi1TxReadPosition = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   spi1TxWritePosition = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);

   //PWM1, audio
   pwm1ClocksToNextSample = readStateValue32(buffer.data + offset);
   offset += sizeof(int32_t);
   for(index = 0; index < 6; index++){
      pwm1Fifo[index] = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
   }
   pwm1ReadPosition = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   pwm1WritePosition = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);

   //misc
   palmMisc.powerButtonLed = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.lcdOn = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.backlightLevel = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.vibratorOn = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryCharging = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryLevel = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.dataPort = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);

   //emu features
   palmEmuFeatures.src = readStateValue32(buffer.data + offset);
   offset += sizeof(uint32_t);
   palmEmuFeatures.dst = readStateValue32(buffer.data + offset);
   offset += sizeof(uint32_t);
   palmEmuFeatures.size = readStateValue32(buffer.data + offset);
   offset += sizeof(uint32_t);
   palmEmuFeatures.value = readStateValue32(buffer.data + offset);
   offset += sizeof(uint32_t);

   //SD card
   palmSdCard.command = readStateValue64(buffer.data + offset);
   offset += sizeof(uint64_t);
   palmSdCard.commandBitsRemaining = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.response = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.responseState = readStateValue64(buffer.data + offset);
   offset += sizeof(uint64_t);
   palmSdCard.allowInvalidCrc = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.chipSelect = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   if(palmSdCard.flashChip.data)
      free(palmSdCard.flashChip.data);
   palmSdCard.flashChip.data = stateSdCardBuffer;
   palmSdCard.flashChip.size = stateSdCardSize;
   memcpy(palmSdCard.flashChip.data, buffer.data + offset, stateSdCardSize);
   offset += stateSdCardSize;

   //some modules depend on all the state memory being loaded before certian required actions can occur(refreshing cached data, freeing memory blocks)
   flx68000LoadStateFinished();

   return true;
}

uint64_t emulatorGetRamSize(void){
   return palmEmuFeatures.info & FEATURE_RAM_HUGE ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE;
}

bool emulatorSaveRam(buffer_t buffer){
   uint64_t size = palmEmuFeatures.info & FEATURE_RAM_HUGE ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE;

   if(buffer.size < size)
      return false;

   memcpy(buffer.data, palmRam, size);
   swap16BufferIfLittle(buffer.data, size / sizeof(uint16_t));

   return true;
}

bool emulatorLoadRam(buffer_t buffer){
   uint64_t size = palmEmuFeatures.info & FEATURE_RAM_HUGE ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE;

   if(buffer.size < size)
      return false;

   memcpy(palmRam, buffer.data, size);
   swap16BufferIfLittle(palmRam, size / sizeof(uint16_t));

   return true;
}

buffer_t emulatorGetSdCardBuffer(void){
   return palmSdCard.flashChip;
}

uint32_t emulatorInsertSdCard(buffer_t image){
   //SD card is currently inserted
   if(palmSdCard.flashChip.data)
      return EMU_ERROR_RESOURCE_LOCKED;

   palmSdCard.flashChip.data = malloc(image.size);
   if(!palmSdCard.flashChip.data)
      return EMU_ERROR_OUT_OF_MEMORY;

   if(image.data)
      memcpy(palmSdCard.flashChip.data, image.data, image.size);
   else
      memset(palmSdCard.flashChip.data, 0x00, image.size);

   palmSdCard.flashChip.size = image.size;
   sdCardReset();

   return EMU_ERROR_NONE;
}

void emulatorEjectSdCard(void){
   //clear SD flash chip and controller
   if(palmSdCard.flashChip.data)
      free(palmSdCard.flashChip.data);
   memset(&palmSdCard, 0x00, sizeof(palmSdCard));
}

uint32_t emulatorInstallPrcPdb(buffer_t file){
   return sandboxCommand(SANDBOX_INSTALL_APP, &file);
   //return EMU_ERROR_NONE;
}

void emulatorRunFrame(void){
   uint32_t samples;

   //I/O
   refreshInputState();

   //CPU
   palmFrameClk32s = 0;
   for(; palmCycleCounter < (double)CRYSTAL_FREQUENCY / EMU_FPS; palmCycleCounter += 1.0){
      flx68000Execute();
      palmFrameClk32s++;
   }
   palmCycleCounter -= (double)CRYSTAL_FREQUENCY / EMU_FPS;

   //audio
   blip_end_frame(palmAudioResampler, blip_clocks_needed(palmAudioResampler, AUDIO_SAMPLES_PER_FRAME));
   blip_read_samples(palmAudioResampler, palmAudio, AUDIO_SAMPLES_PER_FRAME, true);
   MULTITHREAD_LOOP(samples) for(samples = 0; samples < AUDIO_SAMPLES_PER_FRAME * 2; samples += 2)
      palmAudio[samples + 1] = palmAudio[samples];

   //video
   sed1376Render();
   if(palmFramebufferWidth == 160 && palmFramebufferHeight == 220){
      //simple render
      memcpy(palmFramebuffer, sed1376Framebuffer, 160 * 160 * sizeof(uint16_t));
      memcpy(palmFramebuffer + 160 * 160, silkscreen160x60, 160 * 60 * sizeof(uint16_t));
   }
   else{
      //advanced render
      //DRIVER NEEDS TO BE WRITTEN STILL
   }
}
