#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "m68k/m68k.h"
#include "68328Functions.h"
#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "sed1376.h"
#include "ads7846.h"
#include "sdcard.h"
#include "silkscreen.h"
#include "portability.h"
#include "debug/sandbox.h"
#include "specs/emuFeatureRegistersSpec.h"


//Memory Map of Palm m515
//0x00000000-0x00FFFFFF RAM, the first 256(0x100) bytes is copyed from the first 256 bytes of ROM before boot, this applys to all Palms with the 68k architecture
//0x10000000-0x103FFFFF ROM, palmos41-en-m515.rom, substitute "en" for your language code
//0x1FF80000-0x1FF800B3 SED1376(Display Controller) Registers
//0x1FFA0000-0x1FFB3FFF SED1376(Display Controller) Framebuffer, this is not the same as the Palm framebuffer which is always 16 bit color,
//this buffer must be processed depending on whats in the SED1376 registers, the result is the Palm framebuffer
//0xFFFFF000-0xFFFFFDFF Hardware Registers
//0xFFFFFF00-0xFFFFFFFF Bootloader, only reads from UART into RAM and jumps to it, never executed in consumer Palms
//VGhpcyBlbXVsYXRvciBpcyBkZWRpY2F0ZWQgdG8gdGhlIGJvdmluZSBtb28gY293cyB0aGF0IG1vby4=


static bool emulatorInitialized = false;

uint8_t*  palmRam;
uint8_t*  palmRom;
uint8_t   palmReg[REG_SIZE];
input_t   palmInput;
sdcard_t  palmSdCard;
misc_hw_t palmMisc;
uint16_t  palmFramebuffer[160 * (160 + 60)];//really 160*160, the extra pixels are the silkscreened digitizer area
uint16_t* palmExtendedFramebuffer;
uint32_t  palmSpecialFeatures;
double    palmCrystalCycles;//how many cycles before toggling the 32.768 kHz crystal
double    palmCycleCounter;//can be greater then 0 if too many cycles where run
double    palmClockMultiplier;//used by the emulator to overclock the emulated Palm


uint64_t (*emulatorGetSysTime)();
uint64_t* (*emulatorGetSdCardStateChunkList)(uint64_t sessionId, uint64_t stateId);//returns the bps chunkIds for a stateId in the order they need to be applied
void (*emulatorSetSdCardStateChunkList)(uint64_t sessionId, uint64_t stateId, uint64_t* data);//sets the bps chunkIds for a stateId in the order they need to be applied
buffer_t (*emulatorGetSdCardChunk)(uint64_t sessionId, uint64_t chunkId);
void (*emulatorSetSdCardChunk)(uint64_t sessionId, uint64_t chunkId, buffer_t chunk);

static inline bool allSdCardCallbacksPresent(){
   if(emulatorGetSysTime && emulatorGetSdCardStateChunkList && emulatorSetSdCardStateChunkList && emulatorGetSdCardChunk && emulatorSetSdCardChunk)
      return true;
   return false;
}

uint32_t emulatorInit(buffer_t palmRomDump, buffer_t palmBootDump, uint32_t specialFeatures){
   if(emulatorInitialized)
      return EMU_ERROR_NONE;
   
   if(palmRomDump.data == NULL)
      return EMU_ERROR_INVALID_PARAMETER;

   //allocate the buffers
   palmRam = malloc((specialFeatures & FEATURE_RAM_HUGE) ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE);
   palmRom = malloc(ROM_SIZE);
   if(specialFeatures & FEATURE_320x320)
      palmExtendedFramebuffer = malloc(320 * (320 + 120) * sizeof(uint16_t));//really 320*320, the extra pixels are the silkscreened digitizer area
   else
      palmExtendedFramebuffer = NULL;
   if(!palmRam || !palmRom || (!palmExtendedFramebuffer && (specialFeatures & FEATURE_320x320))){
      if(palmRam)
         free(palmRam);
      if(palmRom)
         free(palmRom);
      if(palmExtendedFramebuffer)
         free(palmExtendedFramebuffer);
      return EMU_ERROR_OUT_OF_MEMORY;
   }

   //CPU
   m68k_init();
   m68k_set_cpu_type(M68K_CPU_TYPE_68000);
   patchTo68328();
   m68k_set_reset_instr_callback(emulatorReset);
   m68k_set_int_ack_callback(interruptAcknowledge);
   resetHwRegisters();
   lowPowerStopActive = false;
   palmCycleCounter = 0.0;
   
   //memory
   memset(palmRam, 0x00, (specialFeatures & FEATURE_RAM_HUGE) ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE);
   memcpy(palmRom, palmRomDump.data, uMin(palmRomDump.size, ROM_SIZE));
   if(palmRomDump.size < ROM_SIZE)
      memset(palmRom + palmRomDump.size, 0x00, ROM_SIZE - palmRomDump.size);
   if(palmBootDump.data){
      memcpy(palmReg + REG_SIZE - 1 - BOOTLOADER_SIZE, palmBootDump.data, uMin(palmBootDump.size, BOOTLOADER_SIZE));
      if(palmBootDump.size < BOOTLOADER_SIZE)
         memset(palmReg + REG_SIZE - 1 - BOOTLOADER_SIZE + palmBootDump.size, 0x00, BOOTLOADER_SIZE - palmBootDump.size);
   }
   else{
      memset(palmReg + REG_SIZE - 1 - BOOTLOADER_SIZE, 0x00, BOOTLOADER_SIZE);
   }
   memset(palmFramebuffer, 0x00, 160 * 160 * sizeof(uint16_t));
   memcpy(&palmFramebuffer[160 * 160], silkscreenData, SILKSCREEN_WIDTH * SILKSCREEN_HEIGHT * (SILKSCREEN_BPP / 8));
   if(palmExtendedFramebuffer){
      memset(palmExtendedFramebuffer, 0x00, 320 * 320 * sizeof(uint16_t));
      //add 320*320 silkscreen image later, 2xBRZ should be able to make 320*320 version of the 160*160 silkscreen
   }
   resetAddressSpace();
   sed1376Reset();
   ads7846Reset();
   sdCardInit();
   
   memset(&palmInput, 0x00, sizeof(palmInput));
   memset(&palmSdCard, 0x00, sizeof(palmSdCard));
   memset(&palmMisc, 0x00, sizeof(palmMisc));
   palmMisc.batteryLevel = 100;
   
   //config
   palmClockMultiplier = (specialFeatures & FEATURE_FAST_CPU) ? 2.0 : 1.0;//overclock
   palmClockMultiplier *= 0.80;//run at 80% speed, 20% is likely memory waitstates
   palmSpecialFeatures = specialFeatures;
   setRtc(0, 0, 0, 0);
   
   //start running
   m68k_pulse_reset();

   emulatorInitialized = true;
   return EMU_ERROR_NONE;
}

void emulatorExit(){
   if(emulatorInitialized){
      free(palmRam);
      free(palmRom);
      if(palmSpecialFeatures & FEATURE_320x320)
         free(palmExtendedFramebuffer);
      sdCardExit();
      emulatorInitialized = false;
   }
}

void emulatorReset(){
   //reset doesnt clear RAM or sdcard, all programs are stored in RAM or on sdcard
   debugLog("Reset triggered, PC:0x%08X\n", m68k_get_reg(NULL, M68K_REG_PPC));
   resetHwRegisters();
   resetAddressSpace();//address space must be reset after hardware registers because it is dependant on them
   sed1376Reset();
   ads7846Reset();
   m68k_pulse_reset();
}

void emulatorSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds){
   setRtc(days, hours, minutes, seconds);
}

uint32_t emulatorSetNewSdCard(uint64_t size, uint8_t type){
   if(!allSdCardCallbacksPresent())
      return EMU_ERROR_CALLBACKS_NOT_SET;

   //more than 2gb/too large for FAT16 or not a card type
   if(size > 0x80000000 || type >= CARD_END)
      return EMU_ERROR_INVALID_PARAMETER;

   if(type != CARD_NONE){
      palmSdCard.sessionId = emulatorGetSysTime();//completely new sdcard, reset delta state chain
      palmSdCard.stateId = 0x0000000000000000;//set when saving state
      palmSdCard.size = size;
      palmSdCard.type = type;
      palmSdCard.inserted = true;
   }
   else{
      memset(&palmSdCard, 0x00, sizeof(palmSdCard));
   }
   
   return EMU_ERROR_NONE;
}

buffer_t emulatorGetSdCardImage(){
   return sdCardGetImage();
}

uint32_t emulatorSetSdCardFromImage(buffer_t image, uint8_t type){
   if(!allSdCardCallbacksPresent())
      return EMU_ERROR_CALLBACKS_NOT_SET;

   //invalid pointer, more than 2gb/too large for FAT16 or not a card type
   if(!image.data || image.size > 0x80000000 || type == CARD_NONE || type >= CARD_END)
      return EMU_ERROR_INVALID_PARAMETER;

   sdCardSetFromImage(image);

   palmSdCard.sessionId = emulatorGetSysTime();//completely new sdcard, reset delta state chain
   palmSdCard.stateId = 0x0000000000000000;//set when saving state
   palmSdCard.size = image.size;
   palmSdCard.type = type;
   palmSdCard.inserted = true;

   return EMU_ERROR_NONE;
}

uint64_t emulatorGetStateSize(){
   uint64_t size = 0;
   
   size += sizeof(uint32_t);//save state version
   size += sizeof(uint32_t);//palmSpecialFeatures
   size += sizeof(uint32_t) * (M68K_REG_CAAR + 1);//CPU registers
   size += sizeof(uint8_t);//lowPowerStopActive
   if(palmSpecialFeatures & FEATURE_RAM_HUGE)
      size += SUPERMASSIVE_RAM_SIZE;//system RAM buffer
   else
      size += RAM_SIZE;//system RAM buffer
   size += REG_SIZE;//hardware registers
   size += SED1376_FB_SIZE;//SED1376
   size += SED1376_LUT_SIZE * 3;//SED1376 r, g and b luts
   size += SED1376_REG_SIZE;//SED1376
   size += TOTAL_MEMORY_BANKS;//bank handlers
   size += sizeof(uint32_t) * 4 * CHIP_END;//chip select states
   size += sizeof(uint8_t) * 5 * CHIP_END;//chip select states
   size += sizeof(uint64_t) * 3;//palmSdCard
   size += sizeof(uint8_t) * 2;//palmSdCard
   size += sizeof(uint64_t) * 4;//32.32 fixed point double, timerXCycleCounter and CPU cycle timers
   size += sizeof(int32_t);//pllWakeWait
   size += sizeof(uint32_t);//clk32Counter
   size += sizeof(uint16_t) * 2;//timerStatusReadAcknowledge
   size += sizeof(uint32_t);//edgeTriggeredInterruptLastValue
   size += sizeof(uint16_t) * 8;//RX 8 * 16 SPI1 FIFO
   size += sizeof(uint16_t) * 8;//TX 8 * 16 SPI1 FIFO
   size += sizeof(uint8_t) * 2;//spi1(R/T)xPosition
   size += sizeof(uint8_t) * 2;//ads7846InputBitsLeft, ads7846ControlByte
   size += sizeof(uint8_t);//ads7846PenIrqEnabled
   size += sizeof(uint16_t);//ads7846
   size += sizeof(uint8_t) * 7;//palmMisc
   
   return size;
}

void emulatorSaveState(uint8_t* data){
   uint64_t offset = 0;
   
   //state validation, wont load states that are not from the same state version
   writeStateValueUint32(data + offset, SAVE_STATE_VERSION);
   offset += sizeof(uint32_t);

   //features
   writeStateValueUint32(data + offset, palmSpecialFeatures);
   offset += sizeof(uint32_t);
   
   //CPU
   for(uint32_t cpuReg = 0; cpuReg <=  M68K_REG_CAAR; cpuReg++){
      writeStateValueUint32(data + offset, m68k_get_reg(NULL, cpuReg));
      offset += sizeof(uint32_t);
   }
   writeStateValueBool(data + offset, lowPowerStopActive);
   offset += sizeof(uint8_t);
   
   //memory
   if(palmSpecialFeatures & FEATURE_RAM_HUGE){
      memcpy(data + offset, palmRam, SUPERMASSIVE_RAM_SIZE);
      offset += SUPERMASSIVE_RAM_SIZE;
   }
   else{
      memcpy(data + offset, palmRam, RAM_SIZE);
      offset += RAM_SIZE;
   }
   memcpy(data + offset, palmReg, REG_SIZE);
   offset += REG_SIZE;
   memcpy(data + offset, bankType, TOTAL_MEMORY_BANKS);
   offset += TOTAL_MEMORY_BANKS;
   for(uint32_t chip = CHIP_BEGIN; chip < CHIP_END; chip++){
      writeStateValueBool(data + offset, chips[chip].enable);
      offset += sizeof(uint8_t);
      writeStateValueUint32(data + offset, chips[chip].start);
      offset += sizeof(uint32_t);
      writeStateValueUint32(data + offset, chips[chip].size);
      offset += sizeof(uint32_t);
      writeStateValueUint32(data + offset, chips[chip].mask);
      offset += sizeof(uint32_t);
      writeStateValueBool(data + offset, chips[chip].inBootMode);
      offset += sizeof(uint8_t);
      writeStateValueBool(data + offset, chips[chip].readOnly);
      offset += sizeof(uint8_t);
      writeStateValueBool(data + offset, chips[chip].readOnlyForProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValueBool(data + offset, chips[chip].supervisorOnlyProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValueUint32(data + offset, chips[chip].unprotectedSize);
      offset += sizeof(uint32_t);
   }

   //SED1376
   memcpy(data + offset, sed1376Registers, SED1376_REG_SIZE);
   offset += SED1376_REG_SIZE;
   memcpy(data + offset, sed1376RLut, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(data + offset, sed1376GLut, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(data + offset, sed1376BLut, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(data + offset, sed1376Framebuffer, SED1376_FB_SIZE);
   offset += SED1376_FB_SIZE;

   //sdcard
   //update sdcard struct and save sdcard data
   if(allSdCardCallbacksPresent() && palmSdCard.type != CARD_NONE){
      palmSdCard.stateId = emulatorGetSysTime();
      sdCardSaveState(palmSdCard.sessionId, palmSdCard.stateId);
   }
   writeStateValueUint64(data + offset, palmSdCard.sessionId);
   offset += sizeof(uint64_t);
   writeStateValueUint64(data + offset, palmSdCard.stateId);
   offset += sizeof(uint64_t);
   writeStateValueUint64(data + offset, palmSdCard.size);
   offset += sizeof(uint64_t);
   writeStateValueUint8(data + offset, palmSdCard.type);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, palmSdCard.inserted);
   offset += sizeof(uint8_t);

   //timing
   writeStateValueDouble(data + offset, palmCrystalCycles);
   offset += sizeof(uint64_t);
   writeStateValueDouble(data + offset, palmCycleCounter);
   offset += sizeof(uint64_t);
   writeStateValueInt32(data + offset, pllWakeWait);
   offset += sizeof(int32_t);
   writeStateValueUint32(data + offset, clk32Counter);
   offset += sizeof(uint32_t);
   writeStateValueDouble(data + offset, timerCycleCounter[0]);
   offset += sizeof(uint64_t);
   writeStateValueDouble(data + offset, timerCycleCounter[1]);
   offset += sizeof(uint64_t);
   writeStateValueUint16(data + offset, timerStatusReadAcknowledge[0]);
   offset += sizeof(uint16_t);
   writeStateValueUint16(data + offset, timerStatusReadAcknowledge[1]);
   offset += sizeof(uint16_t);
   writeStateValueUint32(data + offset, edgeTriggeredInterruptLastValue);
   offset += sizeof(uint32_t);

   //SPI1
   for(uint8_t fifoPosition = 0; fifoPosition < 8; fifoPosition++){
      writeStateValueUint16(data + offset, spi1RxFifo[fifoPosition]);
      offset += sizeof(uint16_t);
   }
   for(uint8_t fifoPosition = 0; fifoPosition < 8; fifoPosition++){
      writeStateValueUint16(data + offset, spi1TxFifo[fifoPosition]);
      offset += sizeof(uint16_t);
   }
   writeStateValueUint8(data + offset, spi1RxPosition);
   offset += sizeof(uint8_t);
   writeStateValueUint8(data + offset, spi1TxPosition);
   offset += sizeof(uint8_t);

   //ADS7846
   writeStateValueUint8(data + offset, ads7846BitsToNextControl);
   offset += sizeof(uint8_t);
   writeStateValueUint8(data + offset, ads7846ControlByte);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, ads7846PenIrqEnabled);
   offset += sizeof(uint8_t);
   writeStateValueUint16(data + offset, ads7846OutputValue);
   offset += sizeof(uint16_t);
   
   //misc
   writeStateValueBool(data + offset, palmMisc.powerButtonLed);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, palmMisc.lcdOn);
   offset += sizeof(uint8_t);
   writeStateValueUint8(data + offset, palmMisc.backlightLevel);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, palmMisc.vibratorOn);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, palmMisc.batteryCharging);
   offset += sizeof(uint8_t);
   writeStateValueUint8(data + offset, palmMisc.batteryLevel);
   offset += sizeof(uint8_t);
   writeStateValueUint8(data + offset, palmMisc.dataPort);
   offset += sizeof(uint8_t);
}

void emulatorLoadState(uint8_t* data){
   uint64_t offset = 0;
   
   //state validation, wont load states that are not from the same state version
   if(readStateValueUint32(data + offset) != SAVE_STATE_VERSION)
      return;
   offset += sizeof(uint32_t);

   //features
   palmSpecialFeatures = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);

   //CPU
   for(uint32_t cpuReg = 0; cpuReg <=  M68K_REG_CAAR; cpuReg++){
      m68k_set_reg(cpuReg, readStateValueUint32(data + offset));
      offset += sizeof(uint32_t);
   }
   lowPowerStopActive = readStateValueBool(data + offset);
   offset += 1;
   
   //memory
   if(palmSpecialFeatures & FEATURE_RAM_HUGE){
      memcpy(palmRam, data + offset, SUPERMASSIVE_RAM_SIZE);
      offset += SUPERMASSIVE_RAM_SIZE;
   }
   else{
      memcpy(palmRam, data + offset, RAM_SIZE);
      offset += RAM_SIZE;
   }
   memcpy(palmReg, data + offset, REG_SIZE);
   offset += REG_SIZE;
   memcpy(bankType, data + offset, TOTAL_MEMORY_BANKS);
   offset += TOTAL_MEMORY_BANKS;
   for(uint32_t chip = CHIP_BEGIN; chip < CHIP_END; chip++){
      chips[chip].enable = readStateValueBool(data + offset);
      offset += sizeof(uint8_t);
      chips[chip].start = readStateValueUint32(data + offset);
      offset += sizeof(uint32_t);
      chips[chip].size = readStateValueUint32(data + offset);
      offset += sizeof(uint32_t);
      chips[chip].mask = readStateValueUint32(data + offset);
      offset += sizeof(uint32_t);
      chips[chip].inBootMode = readStateValueBool(data + offset);
      offset += sizeof(uint8_t);
      chips[chip].readOnly = readStateValueBool(data + offset);
      offset += sizeof(uint8_t);
      chips[chip].readOnlyForProtectedMemory = readStateValueBool(data + offset);
      offset += sizeof(uint8_t);
      chips[chip].supervisorOnlyProtectedMemory = readStateValueBool(data + offset);
      offset += sizeof(uint8_t);
      chips[chip].unprotectedSize = readStateValueUint32(data + offset);
      offset += sizeof(uint32_t);
   }

   //SED1376
   memcpy(sed1376Registers, data + offset, SED1376_REG_SIZE);
   offset += SED1376_REG_SIZE;
   memcpy(sed1376RLut, data + offset, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(sed1376GLut, data + offset, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(sed1376BLut, data + offset, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(sed1376Framebuffer, data + offset, SED1376_FB_SIZE);
   offset += SED1376_FB_SIZE;
   sed1376RefreshLut();

   //sdcard
   palmSdCard.sessionId = readStateValueUint64(data + offset);
   offset += sizeof(uint64_t);
   palmSdCard.stateId = readStateValueUint64(data + offset);
   offset += sizeof(uint64_t);
   palmSdCard.size = readStateValueUint64(data + offset);
   offset += sizeof(uint64_t);
   palmSdCard.type = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
   palmSdCard.inserted = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   //update sdcard data from sdcard struct
   if(allSdCardCallbacksPresent() && palmSdCard.type != CARD_NONE)
      sdCardLoadState(palmSdCard.sessionId, palmSdCard.stateId);

   //timing
   palmCrystalCycles = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   palmCycleCounter = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   pllWakeWait = readStateValueInt32(data + offset);
   offset += sizeof(int32_t);
   clk32Counter = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   timerCycleCounter[0] = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   timerCycleCounter[1] = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   timerStatusReadAcknowledge[0] = readStateValueUint16(data + offset);
   offset += sizeof(uint16_t);
   timerStatusReadAcknowledge[1] = readStateValueUint16(data + offset);
   offset += sizeof(uint16_t);
   edgeTriggeredInterruptLastValue = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);

   //SPI1
   for(uint8_t fifoPosition = 0; fifoPosition < 8; fifoPosition++){
      spi1RxFifo[fifoPosition] = readStateValueUint16(data + offset);
      offset += sizeof(uint16_t);
   }
   for(uint8_t fifoPosition = 0; fifoPosition < 8; fifoPosition++){
      spi1TxFifo[fifoPosition] = readStateValueUint16(data + offset);
      offset += sizeof(uint16_t);
   }
   spi1RxPosition = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
   spi1TxPosition = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);

   //ADS7846
   ads7846BitsToNextControl = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
   ads7846ControlByte = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
   ads7846PenIrqEnabled = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   ads7846OutputValue = readStateValueUint16(data + offset);
   offset += sizeof(uint16_t);
   
   //misc
   palmMisc.powerButtonLed = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.lcdOn = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.backlightLevel = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.vibratorOn = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryCharging = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryLevel = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.dataPort = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);
}

uint32_t emulatorInstallPrcPdb(buffer_t file){
   //pretend to pass for now
   //return EMU_ERROR_NOT_IMPLEMENTED;
   return EMU_ERROR_NONE;
}

void emulateFrame(){
   refreshInputState();

   while(palmCycleCounter < CRYSTAL_FREQUENCY / EMU_FPS){
      if(palmCrystalCycles != 0.0 && !lowPowerStopActive){
         //the frequency can change mid frame and get stuck in an infinite loop because of a divide by 0.0
         //+= m68k_execute(palmCrystalCycles{old value} * palmClockMultiplier) / (palmCrystalCycles{new value of 0.0} * palmClockMultiplier) == infinity
         double currentFrequency = palmCrystalCycles;
         palmCycleCounter += m68k_execute(currentFrequency * palmClockMultiplier) / (currentFrequency * palmClockMultiplier);
      }
      else{
         palmCycleCounter += 1.0;
      }
      clk32();
   }
   palmCycleCounter -= CRYSTAL_FREQUENCY / EMU_FPS;

   //CPU can run more then the requested clock cycles, add missed CLK32 pulses from those cycles
   while(palmCycleCounter >= 1.0){
      clk32();
      palmCycleCounter -= 1.0;
   }

   sed1376Render();
}
