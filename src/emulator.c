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
#include "tungstenT3Bus.h"
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

//Memory map of Tungsten T3
//This is a map of the boot address ranges, it can be(and is) changed with the MMU
//0x00000000<->0x003FFFFF ROM
//0xA0000000<->0xA3FFFFFF RAM
//TODO: get the default address ranges Palm OS sets up after boot

//VGhpcyBlbXVsYXRvciBpcyBkZWRpY2F0ZWQgdG8gdGhlIGJvdmluZSBtb28gY293cyB0aGF0IG1vby4=


static bool emulatorInitialized = false;

#if defined(EMU_SUPPORT_PALM_OS5)
bool      palmEmulatingTungstenT3;
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
static void emulatorTungstenT3Frame(void){
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

uint32_t emulatorInit(uint8_t* palmRomData, uint32_t palmRomSize, uint8_t* palmBootloaderData, uint32_t palmBootloaderSize, uint32_t enabledEmuFeatures){
   //only accept valid non debug features from the user
   enabledEmuFeatures &= FEATURE_FAST_CPU | FEATURE_SYNCED_RTC | FEATURE_HLE_APIS | FEATURE_DURABLE;

#if defined(EMU_DEBUG)
   //enable debug features if compiled in debug mode
   enabledEmuFeatures |= FEATURE_DEBUG;
#endif

   if(emulatorInitialized)
      return EMU_ERROR_RESOURCE_LOCKED;

   if(!palmRomData || palmRomSize < 0x8)
      return EMU_ERROR_INVALID_PARAMETER;

#if defined(EMU_SUPPORT_PALM_OS5)
   //0x00000004 is boot program counter on 68k, its just 0x00000000 on ARM
   palmEmulatingTungstenT3 = !(palmRomData[0x4] || palmRomData[0x5] || palmRomData[0x6] || palmRomData[0x7]);

   if(palmEmulatingTungstenT3){
      //emulating Tungsten T3
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
      memcpy(palmRom, palmRomData, uintMin(palmRomSize, TUNGSTEN_T3_ROM_SIZE));
      if(palmRomSize < TUNGSTEN_T3_ROM_SIZE)
         memset(palmRom + palmRomSize, 0x00, TUNGSTEN_T3_ROM_SIZE - palmRomSize);
      memset(palmRam, 0x00, TUNGSTEN_T3_RAM_SIZE);
      memset(palmFramebuffer, 0x00, 320 * 320 * sizeof(uint16_t));//TODO:PXA255 code doesnt always output a picture like my SED1376 code, so clear the buffer to prevent garbage from being displayed before the first render
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

      //initialize components, I dont think theres much in a Tungsten T3
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
      memcpy(palmRom, palmRomData, uintMin(palmRomSize, M515_ROM_SIZE));
      if(palmRomSize < M515_ROM_SIZE)
         memset(palmRom + palmRomSize, 0x00, M515_ROM_SIZE - palmRomSize);
      swap16BufferIfLittle(palmRom, M515_ROM_SIZE / sizeof(uint16_t));
      memset(palmRam, 0x00, M515_RAM_SIZE);
      dbvzLoadBootloader(palmBootloaderData, palmBootloaderSize);
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

void emulatorDeinit(void){
   if(emulatorInitialized){
#if defined(EMU_SUPPORT_PALM_OS5)
      if(!palmEmulatingTungstenT3){
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
      if(palmEmulatingTungstenT3)
         pxa255Deinit();
#endif
      free(palmSdCard.flashChipData);
      emulatorInitialized = false;
   }
}

void emulatorHardReset(void){
   //equivalent to taking the battery out and putting it back in
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3){
      memset(palmRam, 0x00, TUNGSTEN_T3_RAM_SIZE);
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
   if(palmEmulatingTungstenT3){
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
   if(palmEmulatingTungstenT3)
      pxa255SetRtc(days, hours, minutes, seconds);
   else
#endif
      dbvzSetRtc(days, hours, minutes, seconds);
}

uint32_t emulatorGetStateSize(void){
   uint32_t size = 0;

   size += sizeof(uint32_t);//save state version
   size += sizeof(uint32_t);//palmEmuFeatures.info
   size += sizeof(uint64_t);//palmSdCard.flashChipSize, needs to be done first to verify the malloc worked
   size += sizeof(uint16_t) * 2;//palmFramebuffer(Width/Height)
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3){
      size += pxa255StateSize();
      size += expansionHardwareStateSize();
      size += TUNGSTEN_T3_RAM_SIZE;//system RAM buffer
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
   size += palmSdCard.flashChipSize;//palmSdCard.flashChipData

   return size;
}

bool emulatorSaveState(uint8_t* data, uint32_t size){
   uint32_t offset = 0;
   uint8_t index;

   if(size < emulatorGetStateSize())
      return false;//state cant fit

   //state validation, wont load states that are not from the same state version
#if defined(EMU_SUPPORT_PALM_OS5)
   writeStateValue32(data + offset, SAVE_STATE_VERSION | (palmEmulatingTungstenT3 ? SAVE_STATE_FOR_TUNGSTEN_T3 : 0));
#else
   writeStateValue32(data + offset, SAVE_STATE_VERSION);
#endif
   offset += sizeof(uint32_t);

   //features, hotpluging emulated hardware is not supported
   writeStateValue32(data + offset, palmEmuFeatures.info);
   offset += sizeof(uint32_t);

   //SD card size
   writeStateValue64(data + offset, palmSdCard.flashChipSize);
   offset += sizeof(uint64_t);

   //screen state
   writeStateValue16(data + offset, palmFramebufferWidth);
   offset += sizeof(uint16_t);
   writeStateValue16(data + offset, palmFramebufferHeight);
   offset += sizeof(uint16_t);

#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3){
      //chips
      pxa255SaveState(data + offset);
      offset += pxa255StateSize();
      expansionHardwareSaveState(data + offset);
      offset += expansionHardwareStateSize();

      //memory
      memcpy(data + offset, palmRam, TUNGSTEN_T3_RAM_SIZE);
      offset += TUNGSTEN_T3_RAM_SIZE;
   }
   else{
#endif
      //chips
      dbvzSaveState(data + offset);
      offset += dbvzStateSize();
      sed1376SaveState(data + offset);
      offset += sed1376StateSize();
      ads7846SaveState(data + offset);
      offset += ads7846StateSize();
      pdiUsbD12SaveState(data + offset);
      offset += pdiUsbD12StateSize();
      expansionHardwareSaveState(data + offset);
      offset += expansionHardwareStateSize();

      //memory
      memcpy(data + offset, palmRam, M515_RAM_SIZE);
      swap16BufferIfLittle(data + offset, M515_RAM_SIZE / sizeof(uint16_t));
      offset += M515_RAM_SIZE;
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif

   //sandbox, does nothing when disabled
   sandboxSaveState(data + offset);
   offset += sandboxStateSize();

   //misc
   writeStateValue8(data + offset, palmMisc.powerButtonLed);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmMisc.lcdOn);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmMisc.backlightLevel);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmMisc.vibratorOn);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmMisc.batteryCharging);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmMisc.batteryLevel);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmMisc.dataPort);
   offset += sizeof(uint8_t);

   //emu features
   writeStateValue32(data + offset, palmEmuFeatures.src);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, palmEmuFeatures.dst);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, palmEmuFeatures.size);
   offset += sizeof(uint32_t);
   writeStateValue32(data + offset, palmEmuFeatures.value);
   offset += sizeof(uint32_t);

   //SD card
   writeStateValue64(data + offset, palmSdCard.command);
   offset += sizeof(uint64_t);
   writeStateValue8(data + offset, palmSdCard.commandBitsRemaining);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmSdCard.runningCommand);
   offset += sizeof(uint8_t);
   for(index = 0; index < 3; index++){
      writeStateValue32(data + offset, palmSdCard.runningCommandVars[index]);
      offset += sizeof(uint32_t);
   }
   memcpy(data + offset, palmSdCard.runningCommandPacket, SD_CARD_BLOCK_DATA_PACKET_SIZE);
   offset += SD_CARD_BLOCK_DATA_PACKET_SIZE;
   memcpy(data + offset, palmSdCard.responseFifo, SD_CARD_RESPONSE_FIFO_SIZE);
   offset += SD_CARD_RESPONSE_FIFO_SIZE;
   writeStateValue16(data  + offset, palmSdCard.responseReadPosition);
   offset += sizeof(uint16_t);
   writeStateValue8(data + offset, palmSdCard.responseReadPositionBit);
   offset += sizeof(int8_t);
   writeStateValue16(data  + offset, palmSdCard.responseWritePosition);
   offset += sizeof(uint16_t);
   writeStateValue8(data + offset, palmSdCard.commandIsAcmd);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmSdCard.allowInvalidCrc);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmSdCard.chipSelect);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmSdCard.receivingCommand);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, palmSdCard.inIdleState);
   offset += sizeof(uint8_t);
   memcpy(data + offset, palmSdCard.sdInfo.csd, 16);
   offset += 16;
   memcpy(data + offset, palmSdCard.sdInfo.cid, 16);
   offset += 16;
   memcpy(data + offset, palmSdCard.sdInfo.scr, 8);
   offset += 8;
   writeStateValue32(data + offset, palmSdCard.sdInfo.ocr);
   offset += sizeof(uint32_t);
   writeStateValue8(data + offset, palmSdCard.sdInfo.writeProtectSwitch);
   offset += sizeof(uint8_t);
   memcpy(data + offset, palmSdCard.flashChipData, palmSdCard.flashChipSize);
   offset += palmSdCard.flashChipSize;

   return true;
}

bool emulatorLoadState(uint8_t* data, uint32_t size){
   uint32_t offset = 0;
   uint8_t index;
   uint32_t stateSdCardSize;
   uint8_t* stateSdCardBuffer;

   //state validation, wont load states that are not from the same state version
#if defined(EMU_SUPPORT_PALM_OS5)
   if(readStateValue32(data + offset) != (SAVE_STATE_VERSION | (palmEmulatingTungstenT3 ? SAVE_STATE_FOR_TUNGSTEN_T3 : 0)))
      return false;
#else
   if(readStateValue32(data + offset) != SAVE_STATE_VERSION)
      return false;
#endif
   offset += sizeof(uint32_t);

   //features, hotpluging emulated hardware is not supported
   if(readStateValue32(data + offset) != palmEmuFeatures.info)
      return false;
   offset += sizeof(uint32_t);

   //SD card size, the malloc when loading can make it fail, make sure if it fails the emulator state doesnt change
   stateSdCardSize = readStateValue64(data + offset);
   stateSdCardBuffer = stateSdCardSize > 0 ? malloc(stateSdCardSize) : NULL;
   if(stateSdCardSize > 0 && !stateSdCardBuffer)
      return false;
   offset += sizeof(uint64_t);

   //screen state
   palmFramebufferWidth = readStateValue16(data + offset);
   offset += sizeof(uint16_t);
   palmFramebufferHeight = readStateValue16(data + offset);
   offset += sizeof(uint16_t);

#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3){
      //chips
      pxa255LoadState(data + offset);
      offset += pxa255StateSize();
      expansionHardwareLoadState(data + offset);
      offset += expansionHardwareStateSize();

      //memory
      memcpy(palmRam, data + offset, TUNGSTEN_T3_RAM_SIZE);
      offset += TUNGSTEN_T3_RAM_SIZE;
   }
   else{
#endif
      //chips
      dbvzLoadState(data + offset);
      offset += dbvzStateSize();
      sed1376LoadState(data + offset);
      offset += sed1376StateSize();
      ads7846LoadState(data + offset);
      offset += ads7846StateSize();
      pdiUsbD12LoadState(data + offset);
      offset += pdiUsbD12StateSize();
      expansionHardwareLoadState(data + offset);
      offset += expansionHardwareStateSize();

      //memory
      memcpy(palmRam, data + offset, M515_RAM_SIZE);
      swap16BufferIfLittle(palmRam, M515_RAM_SIZE / sizeof(uint16_t));
      offset += M515_RAM_SIZE;
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif

   //sandbox, does nothing when disabled
   sandboxLoadState(data + offset);
   offset += sandboxStateSize();

   //misc
   palmMisc.powerButtonLed = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.lcdOn = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.backlightLevel = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.vibratorOn = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryCharging = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryLevel = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.dataPort = readStateValue8(data + offset);
   offset += sizeof(uint8_t);

   //emu features
   palmEmuFeatures.src = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   palmEmuFeatures.dst = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   palmEmuFeatures.size = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   palmEmuFeatures.value = readStateValue32(data + offset);
   offset += sizeof(uint32_t);

   //SD card
   palmSdCard.command = readStateValue64(data + offset);
   offset += sizeof(uint64_t);
   palmSdCard.commandBitsRemaining = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.runningCommand = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   for(index = 0; index < 3; index++){
      palmSdCard.runningCommandVars[index] = readStateValue32(data + offset);
      offset += sizeof(uint32_t);
   }
   memcpy(palmSdCard.runningCommandPacket, data + offset, SD_CARD_BLOCK_DATA_PACKET_SIZE);
   offset += SD_CARD_BLOCK_DATA_PACKET_SIZE;
   memcpy(palmSdCard.responseFifo, data + offset, SD_CARD_RESPONSE_FIFO_SIZE);
   offset += SD_CARD_RESPONSE_FIFO_SIZE;
   palmSdCard.responseReadPosition = readStateValue16(data  + offset);
   offset += sizeof(uint16_t);
   palmSdCard.responseReadPositionBit = readStateValue8(data + offset);
   offset += sizeof(int8_t);
   palmSdCard.responseWritePosition = readStateValue16(data  + offset);
   offset += sizeof(uint16_t);
   palmSdCard.commandIsAcmd = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.allowInvalidCrc = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.chipSelect = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.receivingCommand = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.inIdleState = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   memcpy(palmSdCard.sdInfo.csd, data + offset, 16);
   offset += 16;
   memcpy(palmSdCard.sdInfo.cid, data + offset, 16);
   offset += 16;
   memcpy(palmSdCard.sdInfo.scr, data + offset, 8);
   offset += 8;
   palmSdCard.sdInfo.ocr = readStateValue32(data + offset);
   offset += sizeof(uint32_t);
   palmSdCard.sdInfo.writeProtectSwitch = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   if(palmSdCard.flashChipData)
      free(palmSdCard.flashChipData);
   palmSdCard.flashChipData = stateSdCardBuffer;
   palmSdCard.flashChipSize = stateSdCardSize;
   memcpy(palmSdCard.flashChipData, data + offset, stateSdCardSize);
   offset += stateSdCardSize;

   //some modules depend on all the state memory being loaded before certian required actions can occur(refreshing cached data, freeing memory blocks)
   dbvzLoadStateFinished();

   return true;
}

uint32_t emulatorGetRamSize(void){
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3)
      return TUNGSTEN_T3_RAM_SIZE;
#endif
   return M515_RAM_SIZE;
}

bool emulatorSaveRam(uint8_t* data, uint32_t size){
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3){
      if(size < TUNGSTEN_T3_RAM_SIZE)
         return false;

      memcpy(data, palmRam, TUNGSTEN_T3_RAM_SIZE);
   }
   else{
#endif
      if(size < M515_RAM_SIZE)
         return false;

      memcpy(data, palmRam, M515_RAM_SIZE);
      swap16BufferIfLittle(data, M515_RAM_SIZE / sizeof(uint16_t));
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif

   return true;
}

bool emulatorLoadRam(uint8_t* data, uint32_t size){
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3){
      if(size < TUNGSTEN_T3_RAM_SIZE)
         return false;

      memcpy(palmRam, data, TUNGSTEN_T3_RAM_SIZE);
   }
   else{
#endif
      if(size < M515_RAM_SIZE)
         return false;

      memcpy(palmRam, data, M515_RAM_SIZE);
      swap16BufferIfLittle(palmRam, M515_RAM_SIZE / sizeof(uint16_t));
#if defined(EMU_SUPPORT_PALM_OS5)
   }
#endif

   return true;
}

uint32_t emulatorInsertSdCard(uint8_t* data, uint32_t size, sd_card_info_t* sdInfo){
   //from the no name SD card that came instered in my test device
   static const sd_card_info_t defaultSdInfo = {
      {0x00, 0x2F, 0x00, 0x32, 0x5F, 0x59, 0x83, 0xB8, 0x6D, 0xB7, 0xFF, 0x9F, 0x96, 0x40, 0x00, 0x00},//csd
      {0x1D, 0x41, 0x44, 0x53, 0x44, 0x20, 0x20, 0x20, 0x10, 0xA0, 0x50, 0x33, 0xA4, 0x00, 0x81, 0x00},//cid
      {0x01, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},//scr
      0x01FF8000,//ocr
      false//writeProtectSwitch
   };

   //SD card is currently inserted
   if(palmSdCard.flashChipData)
      return EMU_ERROR_RESOURCE_LOCKED;

   //no 0 sized chips, max out at 2gb SD card, Palms cant handle higher than that anyway because of incompatibility with FAT32 and SDHC
   if(size == 0x00000000 || size > 0x20000000)
      return EMU_ERROR_INVALID_PARAMETER;

   //add SD_CARD_BLOCK_SIZE to buffer to prevent buffer overflows
   palmSdCard.flashChipSize = size;
   palmSdCard.flashChipData = malloc(palmSdCard.flashChipSize + SD_CARD_BLOCK_SIZE);
   if(!palmSdCard.flashChipData){
      palmSdCard.flashChipSize = 0x00000000;
      return EMU_ERROR_OUT_OF_MEMORY;
   }

   //copy over buffer data
   if(data)
      memcpy(palmSdCard.flashChipData, data, size);
   else
      memset(palmSdCard.flashChipData, 0x00, size);

   //clear the padding block
   memset(palmSdCard.flashChipData + palmSdCard.flashChipSize, 0x00, SD_CARD_BLOCK_SIZE);

   //reinit SD card
   if(sdInfo)
      palmSdCard.sdInfo = *sdInfo;
   else
      palmSdCard.sdInfo = defaultSdInfo;
   sdCardReset();

   return EMU_ERROR_NONE;
}

uint32_t emulatorGetSdCardSize(void){
   if(!palmSdCard.flashChipData)
      return 0;

   return palmSdCard.flashChipSize;
}

uint32_t emulatorGetSdCardData(uint8_t* data, uint32_t size){
   if(!palmSdCard.flashChipData)
      return EMU_ERROR_RESOURCE_LOCKED;

   if(size < palmSdCard.flashChipSize)
      return EMU_ERROR_OUT_OF_MEMORY;

   memcpy(data, palmSdCard.flashChipData, palmSdCard.flashChipSize);

   return EMU_ERROR_NONE;
}

void emulatorEjectSdCard(void){
   //clear SD flash chip, this disables the SD card control chip too
   if(palmSdCard.flashChipData){
      free(palmSdCard.flashChipData);
      palmSdCard.flashChipData = NULL;
      palmSdCard.flashChipSize = 0x00000000;
   }
}

void emulatorRunFrame(void){
#if defined(EMU_SUPPORT_PALM_OS5)
   if(palmEmulatingTungstenT3)
      emulatorTungstenT3Frame();
   else
#endif
      emulatorM515Frame();

#if defined(EMU_SANDBOX)
   sandboxOnFrameRun();
#endif
}
