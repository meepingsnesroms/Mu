#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "audio/blip_buf.h"
#include "flx68000.h"
#include "emulator.h"
#include "dbvzRegisters.h"
#include "expansionHardware.h"
#include "m515Bus.h"
#include "sed1376.h"
#include "ads7846.h"
#include "pdiUsbD12.h"
#include "sdCard.h"
#include "silkscreen.h"
#include "portability.h"
#include "debug/sandbox.h"
#include "specs/emuFeatureRegisterSpec.h"

#if defined(EMU_SUPPORT_PALM_OS5)
#include "tungstenCBus.h"
#include "pxa255/pxa255.h"
#endif


//Memory map of Palm m515
//0x00000000<->0x00FFFFFF RAM, CSC0 as RAS0, CSC1 as RAS1, CSD0 as CAS0 and CSD1 as CAS1
//0x10000000<->0x103FFFFF ROM, CSA0, at address 0x00000000 during boot, palmos41-en-m515.rom, substitute "en" for your language code
//0x10400000<->0x10400003 USB, CSA1
//0x1FF80000<->0x1FF800B3 SED1376(Display Controller) Registers, CSB0
//0x1FFA0000<->0x1FFB3FFF SED1376(Display Controller) Framebuffer, CSB0, this is not the same as the Palm framebuffer which is always 16 bit color,
//this buffer must be processed depending on whats in the SED1376 registers, the result is the Palm framebuffer
//0xFFFFF000<->0xFFFFFDFF Hardware Registers
//0xFFFFFE00<->0xFFFFFFFF Bootloader, only reads from UART into RAM and jumps to it, never executed in consumer Palms

//Memory map of Tungsten C
//This is a map of the boot address ranges, it can be(and is) changed with the MMU
//0x00000000<->0x003FFFFF ROM
//0xA0000000<->0xA3FFFFFF RAM
//TODO: get the default address ranges Palm OS sets up after boot

//VGhpcyBlbXVsYXRvciBpcyBkZWRpY2F0ZWQgdG8gdGhlIGJvdmluZSBtb28gY293cyB0aGF0IG1vby4=


static bool emulatorInitialized = false;
#if defined(EMU_SUPPORT_PALM_OS5)
static bool emulatorEmulatingTungstenC;
#endif

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
double    palmCycleCounter;//can be greater then 0 if too many cycles where run
double    palmClockMultiplier;//used by the emulator to overclock the emulated Palm


#if defined(EMU_SUPPORT_PALM_OS5)
static void emulatorTungstenCFrame(void){
   pxa255Execute();
}
#endif

static void emulatorM515Frame(void){
   uint32_t samples;

   //I/O
   m515RefreshInputState();

   //CPU
   dbvzFrameClk32s = 0;
   for(; palmCycleCounter < (double)M515_CRYSTAL_FREQUENCY / EMU_FPS; palmCycleCounter += 1.0){
      flx68000Execute();
      dbvzFrameClk32s++;
   }
   palmCycleCounter -= (double)M515_CRYSTAL_FREQUENCY / EMU_FPS;

   //audio
   blip_end_frame(palmAudioResampler, blip_clocks_needed(palmAudioResampler, AUDIO_SAMPLES_PER_FRAME));
   blip_read_samples(palmAudioResampler, palmAudio, AUDIO_SAMPLES_PER_FRAME, true);
   MULTITHREAD_LOOP(samples) for(samples = 0; samples < AUDIO_SAMPLES_PER_FRAME * 2; samples += 2)
      palmAudio[samples + 1] = palmAudio[samples];

   //video
   sed1376Render();
}

uint32_t emulatorInit(buffer_t palmRomDump, buffer_t palmBootDump, uint32_t enabledEmuFeatures){
   //only accept valid non debug features from the user
   enabledEmuFeatures &= FEATURE_FAST_CPU | FEATURE_SYNCED_RTC | FEATURE_HLE_APIS | FEATURE_DURABLE;

#if defined(EMU_DEBUG)
   //enable debug features if compiled in debug mode
   enabledEmuFeatures |= FEATURE_DEBUG;
#endif

   if(emulatorInitialized)
      return EMU_ERROR_RESOURCE_LOCKED;

   if(!palmRomDump.data || palmRomDump.size < 0x8)
      return EMU_ERROR_INVALID_PARAMETER;

#if defined(EMU_SUPPORT_PALM_OS5)
   //0x00000004 is boot program counter on 68k, its just 0x00000000 on ARM
   emulatorEmulatingTungstenC = !(palmRomDump.data[0x4] || palmRomDump.data[0x5] || palmRomDump.data[0x6] || palmRomDump.data[0x7]);

   if(emulatorEmulatingTungstenC){
      //emulating Tungsten C
      bool dynarecInited = false;

      dynarecInited = pxa255Init(&palmRom, &palmRam);
      palmFramebuffer = malloc(320 * 320 * sizeof(uint16_t));
      palmAudio = malloc(AUDIO_SAMPLES_PER_FRAME * 2 * sizeof(int16_t));
      palmAudioResampler = blip_new(AUDIO_SAMPLE_RATE);//have 1 second of samples
      if(!palmFramebuffer || !palmAudio || !palmAudioResampler || !dynarecInited){
         free(palmFramebuffer);
         free(palmAudio);
         blip_delete(palmAudioResampler);
         pxa255Deinit();
         return EMU_ERROR_OUT_OF_MEMORY;
      }
      memcpy(palmRom, palmRomDump.data, u32Min(palmRomDump.size, TUNGSTEN_C_ROM_SIZE));
      if(palmRomDump.size < TUNGSTEN_C_ROM_SIZE)
         memset(palmRom + palmRomDump.size, 0x00, TUNGSTEN_C_ROM_SIZE - palmRomDump.size);
      memset(palmRam, 0x00, TUNGSTEN_C_RAM_SIZE);
      memset(palmAudio, 0x00, AUDIO_SAMPLES_PER_FRAME * 2/*channels*/ * sizeof(int16_t));
      memset(&palmInput, 0x00, sizeof(palmInput));
      memset(&palmMisc, 0x00, sizeof(palmMisc));
      memset(&palmSdCard, 0x00, sizeof(palmSdCard));
      memset(&palmEmuFeatures, 0x00, sizeof(palmEmuFeatures));
      palmFramebufferWidth = 320;
      palmFramebufferHeight = 320;
      palmMisc.batteryLevel = 100;
      palmCycleCounter = 0.0;
      palmEmuFeatures.info = enabledEmuFeatures;

      //initialize components, I dont think theres much in a Tungsten C
      blip_set_rates(palmAudioResampler, AUDIO_CLOCK_RATE, AUDIO_SAMPLE_RATE);
      sandboxInit();

      //reset everything
      emulatorSoftReset();
   }
   else{
#endif
      //emulating Palm m515
      //allocate buffers, add 4 to memory regions to prevent SIGSEGV from accessing off the end
      palmRom = malloc(M515_ROM_SIZE + 4);
      palmRam = malloc(M515_RAM_SIZE + 4);
      palmReg = malloc(DBVZ_REG_SIZE + 4);
      palmFramebuffer = malloc(160 * 220 * sizeof(uint16_t));
      palmAudio = malloc(AUDIO_SAMPLES_PER_FRAME * 2 * sizeof(int16_t));
      palmAudioResampler = blip_new(AUDIO_SAMPLE_RATE);//have 1 second of samples
      if(!palmRom || !palmRam || !palmReg || !palmFramebuffer || !palmAudio || !palmAudioResampler){
         free(palmRom);
         free(palmRam);
         free(palmReg);
         free(palmFramebuffer);
         free(palmAudio);
         blip_delete(palmAudioResampler);
         return EMU_ERROR_OUT_OF_MEMORY;
      }

      //set default values
      memcpy(palmRom, palmRomDump.data, u32Min(palmRomDump.size, M515_ROM_SIZE));
      if(palmRomDump.size < M515_ROM_SIZE)
         memset(palmRom + palmRomDump.size, 0x00, M515_ROM_SIZE - palmRomDump.size);
      swap16BufferIfLittle(palmRom, M515_ROM_SIZE / sizeof(uint16_t));
      memset(palmRam, 0x00, M515_RAM_SIZE);
      if(palmBootDump.data){
         memcpy(palmReg + DBVZ_REG_SIZE - 1 - DBVZ_BOOTLOADER_SIZE, palmBootDump.data, u32Min(palmBootDump.size, DBVZ_BOOTLOADER_SIZE));
         if(palmBootDump.size < DBVZ_BOOTLOADER_SIZE)
            memset(palmReg + DBVZ_REG_SIZE - 1 - DBVZ_BOOTLOADER_SIZE + palmBootDump.size, 0x00, DBVZ_BOOTLOADER_SIZE - palmBootDump.size);
         swap16BufferIfLittle(palmReg + DBVZ_REG_SIZE - 1 - DBVZ_BOOTLOADER_SIZE, DBVZ_BOOTLOADER_SIZE / sizeof(uint16_t));
      }
      else{
         memset(palmReg + DBVZ_REG_SIZE - 1 - DBVZ_BOOTLOADER_SIZE, 0x00, DBVZ_BOOTLOADER_SIZE);
      }
      memcpy(palmFramebuffer + 160 * 160, silkscreen160x60, 160 * 60 * sizeof(uint16_t));
      memset(palmAudio, 0x00, AUDIO_SAMPLES_PER_FRAME * 2/*channels*/ * sizeof(int16_t));
      memset(&palmInput, 0x00, sizeof(palmInput));
      memset(&palmMisc, 0x00, sizeof(palmMisc));
      memset(&palmSdCard, 0x00, sizeof(palmSdCard));
      memset(&palmEmuFeatures, 0x00, sizeof(palmEmuFeatures));
      palmFramebufferWidth = 160;
      palmFramebufferHeight = 220;
      palmMisc.batteryLevel = 100;
      palmCycleCounter = 0.0;
      palmEmuFeatures.info = enabledEmuFeatures;
      sed1376Framebuffer = palmFramebuffer;

      //initialize components
      blip_set_rates(palmAudioResampler, AUDIO_CLOCK_RATE, AUDIO_SAMPLE_RATE);
      flx68000Init();
      sandboxInit();

      //reset everything
      emulatorSoftReset();
      dbvzSetRtc(0, 0, 0, 0);//RTCTIME and DAYR are not cleared by reset, clear them manually in case the frontend doesnt set the RTC
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif

   emulatorInitialized = true;

   return EMU_ERROR_NONE;
}

void emulatorExit(void){
   if(emulatorInitialized){
#if defined(EMU_SUPPORT_PALM_OS5)
      if(!emulatorEmulatingTungstenC){
#endif
         free(palmRom);
         free(palmRam);
         free(palmReg);
#if defined(EMU_SUPPORT_PALM_OS5)
      }
#endif
      free(palmFramebuffer);
      free(palmAudio);
      blip_delete(palmAudioResampler);
#if defined(EMU_SUPPORT_PALM_OS5)
      if(emulatorEmulatingTungstenC)
         pxa255Deinit();
#endif
      free(palmSdCard.flashChip.data);
      emulatorInitialized = false;
   }
}

void emulatorHardReset(void){
   //equivalent to taking the battery out and putting it back in
#if defined(EMU_SUPPORT_PALM_OS5)
   if(emulatorEmulatingTungstenC){
      memset(palmRam, 0x00, TUNGSTEN_C_RAM_SIZE);
      emulatorSoftReset();
      sdCardReset();
   }
   else{
#endif
      memset(palmRam, 0x00, M515_RAM_SIZE);
      emulatorSoftReset();
      sdCardReset();
      dbvzSetRtc(0, 0, 0, 0);
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif
}

void emulatorSoftReset(void){
   //equivalent to pushing the reset button on the back of the device
#if defined(EMU_SUPPORT_PALM_OS5)
   if(emulatorEmulatingTungstenC){
      palmEmuFeatures.value = 0x00000000;
      palmClockMultiplier = 1.00;
      pxa255Reset();
      sandboxReset();
   }
   else{
#endif
      palmEmuFeatures.value = 0x00000000;
      palmClockMultiplier = 1.00 - DBVZ_CPU_PERCENT_WAITING;
      sed1376Reset();
      ads7846Reset();
      pdiUsbD12Reset();
      expansionHardwareReset();
      flx68000Reset();
      sandboxReset();
      //sdCardReset() should not be called here, the SD card does not have a reset line and should only be reset by a power cycle
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif
}

void emulatorSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
#if defined(EMU_SUPPORT_PALM_OS5)
   if(emulatorEmulatingTungstenC)
      pxa255SetRtc(days, hours, minutes, seconds);
   else
#endif
      dbvzSetRtc(days, hours, minutes, seconds);
}

uint32_t emulatorGetStateSize(void){
   uint32_t size = 0;

   size += sizeof(uint32_t);//save state version
   size += sizeof(uint32_t);//palmEmuFeatures.info
   size += sizeof(uint64_t);//palmSdCard.flashChip.size, needs to be done first to verify the malloc worked
   size += sizeof(uint16_t) * 2;//palmFramebuffer(Width/Height)
   size += flx68000StateSize();
   size += sed1376StateSize();
   size += ads7846StateSize();
   size += pdiUsbD12StateSize();
   size += expansionHardwareStateSize();
   size += sandboxStateSize();
   size += M515_RAM_SIZE;//system RAM buffer
   size += DBVZ_REG_SIZE;//hardware registers
   size += DBVZ_TOTAL_MEMORY_BANKS;//bank handlers
   size += sizeof(uint32_t) * 4 * DBVZ_CHIP_END;//chip select states
   size += sizeof(uint8_t) * 5 * DBVZ_CHIP_END;//chip select states
   size += sizeof(uint64_t) * 5;//32.32 fixed point double, timerXCycleCounter and CPU cycle timers
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
   size += sizeof(uint32_t) * 4;//palmEmuFeatures.(src/dst/size/value)
   size += sizeof(uint64_t);//palmSdCard.command
   size += sizeof(uint8_t) * 8;//palmSdCard.(commandBitsRemaining/runningCommand/commandIsAcmd/allowInvalidCrc/chipSelect/receivingCommand/inIdleState/writeProtectSwitch)
   size += sizeof(uint16_t) * 2;//palmSdCard.response(Read/Write)Position
   size += sizeof(int8_t);//palmSdCard.responseReadPositionBit
   size += sizeof(uint32_t) * 3;//palmSdCard.runningCommandVars
   size += SD_CARD_BLOCK_DATA_PACKET_SIZE;//palmSdCard.runningCommandPacket
   size += SD_CARD_RESPONSE_FIFO_SIZE;//palmSdCard.responseFifo
   size += palmSdCard.flashChip.size;//palmSdCard.flashChip.data

   return size;
}

bool emulatorSaveState(buffer_t buffer){
   uint32_t offset = 0;
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
   sed1376SaveState(buffer.data + offset);
   offset += sed1376StateSize();
   ads7846SaveState(buffer.data + offset);
   offset += ads7846StateSize();
   pdiUsbD12SaveState(buffer.data + offset);
   offset += pdiUsbD12StateSize();
   expansionHardwareSaveState(buffer.data + offset);
   offset += expansionHardwareStateSize();

   //sandbox, does nothing when disabled
   sandboxSaveState(buffer.data + offset);
   offset += sandboxStateSize();

   //memory
   memcpy(buffer.data + offset, palmRam, M515_RAM_SIZE);
   swap16BufferIfLittle(buffer.data + offset, M515_RAM_SIZE / sizeof(uint16_t));
   offset += M515_RAM_SIZE;
   memcpy(buffer.data + offset, palmReg, DBVZ_REG_SIZE);
   swap16BufferIfLittle(buffer.data + offset, DBVZ_REG_SIZE / sizeof(uint16_t));
   offset += DBVZ_REG_SIZE;
   memcpy(buffer.data + offset, dbvzBankType, DBVZ_TOTAL_MEMORY_BANKS);
   offset += DBVZ_TOTAL_MEMORY_BANKS;
   for(index = DBVZ_CHIP_BEGIN; index < DBVZ_CHIP_END; index++){
      writeStateValue8(buffer.data + offset, dbvzChipSelects[index].enable);
      offset += sizeof(uint8_t);
      writeStateValue32(buffer.data + offset, dbvzChipSelects[index].start);
      offset += sizeof(uint32_t);
      writeStateValue32(buffer.data + offset, dbvzChipSelects[index].lineSize);
      offset += sizeof(uint32_t);
      writeStateValue32(buffer.data + offset, dbvzChipSelects[index].mask);
      offset += sizeof(uint32_t);
      writeStateValue8(buffer.data + offset, dbvzChipSelects[index].inBootMode);
      offset += sizeof(uint8_t);
      writeStateValue8(buffer.data + offset, dbvzChipSelects[index].readOnly);
      offset += sizeof(uint8_t);
      writeStateValue8(buffer.data + offset, dbvzChipSelects[index].readOnlyForProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValue8(buffer.data + offset, dbvzChipSelects[index].supervisorOnlyProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValue32(buffer.data + offset, dbvzChipSelects[index].unprotectedSize);
      offset += sizeof(uint32_t);
   }

   //timing
   writeStateValueDouble(buffer.data + offset, dbvzSysclksPerClk32);
   offset += sizeof(uint64_t);
   writeStateValueDouble(buffer.data + offset, palmCycleCounter);
   offset += sizeof(uint64_t);
   writeStateValueDouble(buffer.data + offset, palmClockMultiplier);
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
   writeStateValue8(buffer.data + offset, palmSdCard.runningCommand);
   offset += sizeof(uint8_t);
   for(index = 0; index < 3; index++){
      writeStateValue32(buffer.data + offset, palmSdCard.runningCommandVars[index]);
      offset += sizeof(uint32_t);
   }
   memcpy(buffer.data + offset, palmSdCard.runningCommandPacket, SD_CARD_BLOCK_DATA_PACKET_SIZE);
   offset += SD_CARD_BLOCK_DATA_PACKET_SIZE;
   memcpy(buffer.data + offset, palmSdCard.responseFifo, SD_CARD_RESPONSE_FIFO_SIZE);
   offset += SD_CARD_RESPONSE_FIFO_SIZE;
   writeStateValue16(buffer.data  + offset, palmSdCard.responseReadPosition);
   offset += sizeof(uint16_t);
   writeStateValue8(buffer.data + offset, palmSdCard.responseReadPositionBit);
   offset += sizeof(int8_t);
   writeStateValue16(buffer.data  + offset, palmSdCard.responseWritePosition);
   offset += sizeof(uint16_t);
   writeStateValue8(buffer.data + offset, palmSdCard.commandIsAcmd);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmSdCard.allowInvalidCrc);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmSdCard.chipSelect);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmSdCard.receivingCommand);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmSdCard.inIdleState);
   offset += sizeof(uint8_t);
   writeStateValue8(buffer.data + offset, palmSdCard.writeProtectSwitch);
   offset += sizeof(uint8_t);
   memcpy(buffer.data + offset, palmSdCard.flashChip.data, palmSdCard.flashChip.size);
   offset += palmSdCard.flashChip.size;

   return true;
}

bool emulatorLoadState(buffer_t buffer){
   uint32_t offset = 0;
   uint8_t index;
   uint32_t stateSdCardSize;
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
   sed1376LoadState(buffer.data + offset);
   offset += sed1376StateSize();
   ads7846LoadState(buffer.data + offset);
   offset += ads7846StateSize();
   pdiUsbD12LoadState(buffer.data + offset);
   offset += pdiUsbD12StateSize();
   expansionHardwareLoadState(buffer.data + offset);
   offset += expansionHardwareStateSize();

   //sandbox, does nothing when disabled
   sandboxLoadState(buffer.data + offset);
   offset += sandboxStateSize();

   //memory
   memcpy(palmRam, buffer.data + offset, M515_RAM_SIZE);
   swap16BufferIfLittle(palmRam, M515_RAM_SIZE / sizeof(uint16_t));
   offset += M515_RAM_SIZE;
   memcpy(palmReg, buffer.data + offset, DBVZ_REG_SIZE);
   swap16BufferIfLittle(palmReg, DBVZ_REG_SIZE / sizeof(uint16_t));
   offset += DBVZ_REG_SIZE;
   memcpy(dbvzBankType, buffer.data + offset, DBVZ_TOTAL_MEMORY_BANKS);
   offset += DBVZ_TOTAL_MEMORY_BANKS;
   for(index = DBVZ_CHIP_BEGIN; index < DBVZ_CHIP_END; index++){
      dbvzChipSelects[index].enable = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].start = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
      dbvzChipSelects[index].lineSize = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
      dbvzChipSelects[index].mask = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
      dbvzChipSelects[index].inBootMode = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].readOnly = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].readOnlyForProtectedMemory = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].supervisorOnlyProtectedMemory = readStateValue8(buffer.data + offset);
      offset += sizeof(uint8_t);
      dbvzChipSelects[index].unprotectedSize = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
   }

   //timing
   dbvzSysclksPerClk32 = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   palmCycleCounter = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   palmClockMultiplier = readStateValueDouble(buffer.data + offset);
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
   palmSdCard.runningCommand = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   for(index = 0; index < 3; index++){
      palmSdCard.runningCommandVars[index] = readStateValue32(buffer.data + offset);
      offset += sizeof(uint32_t);
   }
   memcpy(palmSdCard.runningCommandPacket, buffer.data + offset, SD_CARD_BLOCK_DATA_PACKET_SIZE);
   offset += SD_CARD_BLOCK_DATA_PACKET_SIZE;
   memcpy(palmSdCard.responseFifo, buffer.data + offset, SD_CARD_RESPONSE_FIFO_SIZE);
   offset += SD_CARD_RESPONSE_FIFO_SIZE;
   palmSdCard.responseReadPosition = readStateValue16(buffer.data  + offset);
   offset += sizeof(uint16_t);
   palmSdCard.responseReadPositionBit = readStateValue8(buffer.data + offset);
   offset += sizeof(int8_t);
   palmSdCard.responseWritePosition = readStateValue16(buffer.data  + offset);
   offset += sizeof(uint16_t);
   palmSdCard.commandIsAcmd = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.allowInvalidCrc = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.chipSelect = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.receivingCommand = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.inIdleState = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.writeProtectSwitch = readStateValue8(buffer.data + offset);
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

uint32_t emulatorGetRamSize(void){
   return M515_RAM_SIZE;
}

bool emulatorSaveRam(buffer_t buffer){
   if(buffer.size < M515_RAM_SIZE)
      return false;

   memcpy(buffer.data, palmRam, M515_RAM_SIZE);
   swap16BufferIfLittle(buffer.data, M515_RAM_SIZE / sizeof(uint16_t));

   return true;
}

bool emulatorLoadRam(buffer_t buffer){
   if(buffer.size < M515_RAM_SIZE)
      return false;

   memcpy(palmRam, buffer.data, M515_RAM_SIZE);
   swap16BufferIfLittle(palmRam, M515_RAM_SIZE / sizeof(uint16_t));

   return true;
}

buffer_t emulatorGetSdCardBuffer(void){
   return palmSdCard.flashChip;
}

uint32_t emulatorInsertSdCard(buffer_t image, bool writeProtectSwitch){
   //SD card is currently inserted
   if(palmSdCard.flashChip.data)
      return EMU_ERROR_RESOURCE_LOCKED;

   //no 0 sized chips, max out at 2gb SD card, Palms cant handle higher than that anyway because of incompatibility with FAT32 and SDHC
   if(image.size == 0x00000000 || image.size > 0x20000000)
      return EMU_ERROR_INVALID_PARAMETER;

   //round up to nearest mb, prevents issues with buffer size and too small SD cards
   palmSdCard.flashChip.size = (image.size & 0xFFF00000) + (image.size & 0x000FFFFF ? 0x00100000 : 0x00000000);
   palmSdCard.flashChip.data = malloc(palmSdCard.flashChip.size);
   if(!palmSdCard.flashChip.data){
      palmSdCard.flashChip.size = 0x00000000;
      return EMU_ERROR_OUT_OF_MEMORY;
   }

   //copy over buffer data
   if(image.data)
      memcpy(palmSdCard.flashChip.data, image.data, image.size);
   else
      memset(palmSdCard.flashChip.data, 0x00, image.size);

   //0 out the unused part of the chip
   memset(palmSdCard.flashChip.data + image.size, 0x00, palmSdCard.flashChip.size - image.size);

   //reinit SD card
   palmSdCard.writeProtectSwitch = writeProtectSwitch;
   sdCardReset();

   return EMU_ERROR_NONE;
}

void emulatorEjectSdCard(void){
   //clear SD flash chip, this disables the SD card control chip too
   if(palmSdCard.flashChip.data){
      free(palmSdCard.flashChip.data);
      palmSdCard.flashChip.data = NULL;
      palmSdCard.flashChip.size = 0x00000000;
   }
}

void emulatorRunFrame(void){
#if defined(EMU_SUPPORT_PALM_OS5)
   if(emulatorEmulatingTungstenC)
      emulatorTungstenCFrame();
   else
      emulatorM515Frame();
#else
   emulatorM515Frame();
#endif

#if defined(EMU_SANDBOX)
   sandboxOnFrameRun();
#endif
}
