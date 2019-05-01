#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "audio/blip_buf.h"
#include "emulator.h"
#include "dbvz.h"
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
bool      palmEmulatingTungstenC;
#endif
uint8_t*  palmRam;
uint8_t*  palmRom;
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
   //CPU
   pxa255Execute();
}
#endif

static void emulatorM515Frame(void){
   //CPU
   dbvzExecute();

   //LCD controller
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
   palmEmulatingTungstenC = !(palmRomDump.data[0x4] || palmRomDump.data[0x5] || palmRomDump.data[0x6] || palmRomDump.data[0x7]);

   if(palmEmulatingTungstenC){
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
      memset(palmFramebuffer, 0x00, 320 * 320 * sizeof(uint16_t));//TODO:PXA255 code doesnt always output a picture like my SED1376 code
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
      pxa255Framebuffer = palmFramebuffer;
      blip_set_rates(palmAudioResampler, AUDIO_CLOCK_RATE, AUDIO_SAMPLE_RATE);
      sandboxInit();

      //reset everything
      emulatorSoftReset();
      pxa255SetRtc(0, 0, 0, 0);
   }
   else{
#endif
      //emulating Palm m515
      //allocate buffers, add 4 to memory regions to prevent SIGSEGV from accessing off the end
      palmRom = malloc(M515_ROM_SIZE + 4);
      palmRam = malloc(M515_RAM_SIZE + 4);
      palmFramebuffer = malloc(160 * 220 * sizeof(uint16_t));
      palmAudio = malloc(AUDIO_SAMPLES_PER_FRAME * 2 * sizeof(int16_t));
      palmAudioResampler = blip_new(AUDIO_SAMPLE_RATE);//have 1 second of samples
      if(!palmRom || !palmRam || !palmFramebuffer || !palmAudio || !palmAudioResampler){
         free(palmRom);
         free(palmRam);
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
      dbvzLoadBootloader(palmBootDump.data, palmBootDump.size);
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
      if(!palmEmulatingTungstenC){
#endif
         free(palmRom);
         free(palmRam);
#if defined(EMU_SUPPORT_PALM_OS5)
      }
#endif
      free(palmFramebuffer);
      free(palmAudio);
      blip_delete(palmAudioResampler);
#if defined(EMU_SUPPORT_PALM_OS5)
      if(palmEmulatingTungstenC)
         pxa255Deinit();
#endif
      free(palmSdCard.flashChip.data);
      emulatorInitialized = false;
   }
}

void emulatorHardReset(void){
   //equivalent to taking the battery out and putting it back in
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenC){
      memset(palmRam, 0x00, TUNGSTEN_C_RAM_SIZE);
      emulatorSoftReset();
      sdCardReset();
      pxa255SetRtc(0, 0, 0, 0);
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
   if(palmEmulatingTungstenC){
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
      dbvzReset();
      sandboxReset();
      //sdCardReset() should not be called here, the SD card does not have a reset line and should only be reset by a power cycle
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif
}

void emulatorSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenC)
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
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenC){
      size += pxa255StateSize();
      size += expansionHardwareStateSize();
      size += TUNGSTEN_C_RAM_SIZE;//system RAM buffer
   }
   else{
#endif
      size += dbvzStateSize();
      size += sed1376StateSize();
      size += ads7846StateSize();
      size += pdiUsbD12StateSize();
      size += expansionHardwareStateSize();
      size += M515_RAM_SIZE;//system RAM buffer
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif
   size += sandboxStateSize();
   size += sizeof(uint8_t) * 7;//palmMisc
   size += sizeof(uint32_t) * 4;//palmEmuFeatures.(src/dst/size/value)
   size += sizeof(uint64_t);//palmSdCard.command
   size += sizeof(uint8_t) * 7;//palmSdCard.(commandBitsRemaining/runningCommand/commandIsAcmd/allowInvalidCrc/chipSelect/receivingCommand/inIdleState)
   size += sizeof(uint16_t) * 2;//palmSdCard.response(Read/Write)Position
   size += sizeof(int8_t);//palmSdCard.responseReadPositionBit
   size += sizeof(uint32_t) * 3;//palmSdCard.runningCommandVars
   size += SD_CARD_BLOCK_DATA_PACKET_SIZE;//palmSdCard.runningCommandPacket
   size += SD_CARD_RESPONSE_FIFO_SIZE;//palmSdCard.responseFifo
   size += 16 * 2;//palmSdCard.sdInfo.csd/cid
   size += 8;//palmSdCard.sdInfo.scr
   size += sizeof(uint32_t);//palmSdCard.sdInfo.ocr
   size += sizeof(uint8_t);//palmSdCard.sdInfo.writeProtectSwitch
   size += palmSdCard.flashChip.size;//palmSdCard.flashChip.data

   return size;
}

bool emulatorSaveState(buffer_t buffer){
   uint32_t offset = 0;
   uint8_t index;

   if(buffer.size < emulatorGetStateSize())
      return false;//state cant fit

   //state validation, wont load states that are not from the same state version
#if defined(EMU_SUPPORT_PALM_OS5)
   writeStateValue32(buffer.data + offset, SAVE_STATE_VERSION | (palmEmulatingTungstenC ? SAVE_STATE_FOR_TUNGSTEN_C : 0));
#else
   writeStateValue32(buffer.data + offset, SAVE_STATE_VERSION);
#endif
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

#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenC){
      //chips
      pxa255SaveState(buffer.data + offset);
      offset += pxa255StateSize();
      expansionHardwareSaveState(buffer.data + offset);
      offset += expansionHardwareStateSize();

      //memory
      memcpy(buffer.data + offset, palmRam, TUNGSTEN_C_RAM_SIZE);
      swap16BufferIfLittle(buffer.data + offset, TUNGSTEN_C_RAM_SIZE / sizeof(uint16_t));
      offset += TUNGSTEN_C_RAM_SIZE;
   }
   else{
#endif
      //chips
      dbvzSaveState(buffer.data + offset);
      offset += dbvzStateSize();
      sed1376SaveState(buffer.data + offset);
      offset += sed1376StateSize();
      ads7846SaveState(buffer.data + offset);
      offset += ads7846StateSize();
      pdiUsbD12SaveState(buffer.data + offset);
      offset += pdiUsbD12StateSize();
      expansionHardwareSaveState(buffer.data + offset);
      offset += expansionHardwareStateSize();

      //memory
      memcpy(buffer.data + offset, palmRam, M515_RAM_SIZE);
      swap16BufferIfLittle(buffer.data + offset, M515_RAM_SIZE / sizeof(uint16_t));
      offset += M515_RAM_SIZE;
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif

   //sandbox, does nothing when disabled
   sandboxSaveState(buffer.data + offset);
   offset += sandboxStateSize();

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
   memcpy(buffer.data + offset, palmSdCard.sdInfo.csd, 16);
   offset += 16;
   memcpy(buffer.data + offset, palmSdCard.sdInfo.cid, 16);
   offset += 16;
   memcpy(buffer.data + offset, palmSdCard.sdInfo.scr, 8);
   offset += 8;
   writeStateValue32(buffer.data + offset, palmSdCard.sdInfo.ocr);
   offset += sizeof(uint32_t);
   writeStateValue8(buffer.data + offset, palmSdCard.sdInfo.writeProtectSwitch);
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
#if defined(EMU_SUPPORT_PALM_OS5)
   if(readStateValue32(buffer.data + offset) != (SAVE_STATE_VERSION | (palmEmulatingTungstenC ? SAVE_STATE_FOR_TUNGSTEN_C : 0)))
      return false;
#else
   if(readStateValue32(buffer.data + offset) != SAVE_STATE_VERSION)
      return false;
#endif
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

#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenC){
      //chips
      pxa255LoadState(buffer.data + offset);
      offset += pxa255StateSize();
      expansionHardwareLoadState(buffer.data + offset);
      offset += expansionHardwareStateSize();

      //memory
      memcpy(palmRam, buffer.data + offset, TUNGSTEN_C_RAM_SIZE);
      swap16BufferIfLittle(palmRam, TUNGSTEN_C_RAM_SIZE / sizeof(uint16_t));
      offset += TUNGSTEN_C_RAM_SIZE;
   }
   else{
#endif
      //chips
      dbvzLoadState(buffer.data + offset);
      offset += dbvzStateSize();
      sed1376LoadState(buffer.data + offset);
      offset += sed1376StateSize();
      ads7846LoadState(buffer.data + offset);
      offset += ads7846StateSize();
      pdiUsbD12LoadState(buffer.data + offset);
      offset += pdiUsbD12StateSize();
      expansionHardwareLoadState(buffer.data + offset);
      offset += expansionHardwareStateSize();

      //memory
      memcpy(palmRam, buffer.data + offset, M515_RAM_SIZE);
      swap16BufferIfLittle(palmRam, M515_RAM_SIZE / sizeof(uint16_t));
      offset += M515_RAM_SIZE;
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif

   //sandbox, does nothing when disabled
   sandboxLoadState(buffer.data + offset);
   offset += sandboxStateSize();

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
   memcpy(palmSdCard.sdInfo.csd, buffer.data + offset, 16);
   offset += 16;
   memcpy(palmSdCard.sdInfo.cid, buffer.data + offset, 16);
   offset += 16;
   memcpy(palmSdCard.sdInfo.scr, buffer.data + offset, 8);
   offset += 8;
   palmSdCard.sdInfo.ocr = readStateValue32(buffer.data + offset);
   offset += sizeof(uint32_t);
   palmSdCard.sdInfo.writeProtectSwitch = readStateValue8(buffer.data + offset);
   offset += sizeof(uint8_t);
   if(palmSdCard.flashChip.data)
      free(palmSdCard.flashChip.data);
   palmSdCard.flashChip.data = stateSdCardBuffer;
   palmSdCard.flashChip.size = stateSdCardSize;
   memcpy(palmSdCard.flashChip.data, buffer.data + offset, stateSdCardSize);
   offset += stateSdCardSize;

   //some modules depend on all the state memory being loaded before certian required actions can occur(refreshing cached data, freeing memory blocks)
   dbvzLoadStateFinished();

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

uint32_t emulatorInsertSdCard(buffer_t image, sd_card_info_t* sdInfo){
   //from the no name SD card that came instered in my test device
   static const sd_card_info_t defaultSdInfo = {
      {0x00, 0x2F, 0x00, 0x32, 0x5F, 0x59, 0x83, 0xB8, 0x6D, 0xB7, 0xFF, 0x9F, 0x96, 0x40, 0x00, 0x00},//csd
      {0x1D, 0x41, 0x44, 0x53, 0x44, 0x20, 0x20, 0x20, 0x10, 0xA0, 0x50, 0x33, 0xA4, 0x00, 0x81, 0x00},//cid
      {0x01, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},//scr
      0x01FF8000,//ocr
      false//writeProtectSwitch
   };

   //SD card is currently inserted
   if(palmSdCard.flashChip.data)
      return EMU_ERROR_RESOURCE_LOCKED;

   //no 0 sized chips, max out at 2gb SD card, Palms cant handle higher than that anyway because of incompatibility with FAT32 and SDHC
   if(image.size == 0x00000000 || image.size > 0x20000000)
      return EMU_ERROR_INVALID_PARAMETER;

   //add SD_CARD_BLOCK_SIZE to buffer to prevent buffer overflows
   palmSdCard.flashChip.size = image.size;
   palmSdCard.flashChip.data = malloc(palmSdCard.flashChip.size + SD_CARD_BLOCK_SIZE);
   if(!palmSdCard.flashChip.data){
      palmSdCard.flashChip.size = 0x00000000;
      return EMU_ERROR_OUT_OF_MEMORY;
   }

   //copy over buffer data
   if(image.data)
      memcpy(palmSdCard.flashChip.data, image.data, image.size);
   else
      memset(palmSdCard.flashChip.data, 0x00, image.size);

   //clear the padding block
   memset(palmSdCard.flashChip.data + palmSdCard.flashChip.size, 0x00, SD_CARD_BLOCK_SIZE);

   //reinit SD card
   if(sdInfo)
      palmSdCard.sdInfo = *sdInfo;
   else
      palmSdCard.sdInfo = defaultSdInfo;
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
   if(palmEmulatingTungstenC)
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
