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
#include "sdCard.h"
#include "silkscreen.h"
#include "portability.h"
#include "debug/sandbox.h"
#include "specs/emuFeatureRegistersSpec.h"


//Memory Map of Palm m515
//0x00000000-0x00FFFFFF RAM, CSC0 as RAS0, CSC1 as RAS1, CSD0 as CAS0 and CSD1 as CAS1
//0x10000000-0x103FFFFF ROM, CSA0, at address 0x00000000 during boot, palmos41-en-m515.rom, substitute "en" for your language code
//0x10400000-0x10C00000 USB, CSC0, 8mb range(if DSIZ3 applys when CSC is not an extension of CSD, otherwise 32k), actual size unknown
//0x1FF80000-0x1FF800B3 SED1376(Display Controller) Registers, CSB0
//0x1FFA0000-0x1FFB3FFF SED1376(Display Controller) Framebuffer, CSB0, this is not the same as the Palm framebuffer which is always 16 bit color,
//this buffer must be processed depending on whats in the SED1376 registers, the result is the Palm framebuffer
//0xFFFFF000-0xFFFFFDFF Hardware Registers
//0xFFFFFE00-0xFFFFFFFF Bootloader, only reads from UART into RAM and jumps to it, never executed in consumer Palms
//VGhpcyBlbXVsYXRvciBpcyBkZWRpY2F0ZWQgdG8gdGhlIGJvdmluZSBtb28gY293cyB0aGF0IG1vby4=


static bool emulatorInitialized = false;

uint8_t*  palmRam;
uint8_t*  palmRom;
uint8_t   palmReg[REG_SIZE];
input_t   palmInput;
buffer_t  palmSdCard;
misc_hw_t palmMisc;
uint16_t  palmFramebuffer[160 * (160 + 60)];//really 160*160, the extra pixels are the silkscreened digitizer area
uint16_t* palmExtendedFramebuffer;
uint32_t  palmSpecialFeatures;
double    palmCrystalCycles;//how many cycles before toggling the 32.768 kHz crystal
double    palmCycleCounter;//can be greater then 0 if too many cycles where run
double    palmClockMultiplier;//used by the emulator to overclock the emulated Palm


uint32_t emulatorInit(buffer_t palmRomDump, buffer_t palmBootDump, uint32_t specialFeatures){
   if(emulatorInitialized)
      return EMU_ERROR_NONE;
   
   if(!palmRomDump.data)
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
   sandboxInit();
   
   memset(&palmInput, 0x00, sizeof(palmInput));
   memset(&palmSdCard, 0x00, sizeof(palmSdCard));
   memset(&palmMisc, 0x00, sizeof(palmMisc));
   palmMisc.batteryLevel = 100;
   
   //config
   palmClockMultiplier = (specialFeatures & FEATURE_FAST_CPU) ? 2.0 : 1.0;//overclock
   palmClockMultiplier *= 0.80;//run at 80% speed, 20% is likely memory waitstates, at 100% it crashes on the spinning Palm welcome screen, 90% works though
   palmSpecialFeatures = specialFeatures;
   setRtc(0,0,0,0);//RTCTIME and DAYR are not cleared by reset, clear them manually in case the front end doesnt set the RTC
   
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
      if(palmSdCard.data)
         free(palmSdCard.data);
      emulatorInitialized = false;
   }
}

void emulatorReset(){
   //reset doesnt clear RAM or SD card, all programs are stored in RAM or on SD card
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
   size += sizeof(uint64_t);//palmSdCard.size
   size += palmSdCard.size;//palmSdCard.data
   
   return size;
}

bool emulatorSaveState(buffer_t buffer){
   uint64_t offset = 0;

   if(buffer.size < emulatorGetStateSize())
      return false;//state cant fit
   
   //state validation, wont load states that are not from the same state version
   writeStateValueUint32(buffer.data + offset, SAVE_STATE_VERSION);
   offset += sizeof(uint32_t);

   //features
   writeStateValueUint32(buffer.data + offset, palmSpecialFeatures);
   offset += sizeof(uint32_t);
   
   //CPU
   for(uint32_t cpuReg = 0; cpuReg <=  M68K_REG_CAAR; cpuReg++){
      writeStateValueUint32(buffer.data + offset, m68k_get_reg(NULL, cpuReg));
      offset += sizeof(uint32_t);
   }
   writeStateValueBool(buffer.data + offset, lowPowerStopActive);
   offset += sizeof(uint8_t);
   
   //memory
   if(palmSpecialFeatures & FEATURE_RAM_HUGE){
      memcpy(buffer.data + offset, palmRam, SUPERMASSIVE_RAM_SIZE);
      offset += SUPERMASSIVE_RAM_SIZE;
   }
   else{
      memcpy(buffer.data + offset, palmRam, RAM_SIZE);
      offset += RAM_SIZE;
   }
   memcpy(buffer.data + offset, palmReg, REG_SIZE);
   offset += REG_SIZE;
   memcpy(buffer.data + offset, bankType, TOTAL_MEMORY_BANKS);
   offset += TOTAL_MEMORY_BANKS;
   for(uint32_t chip = CHIP_BEGIN; chip < CHIP_END; chip++){
      writeStateValueBool(buffer.data + offset, chips[chip].enable);
      offset += sizeof(uint8_t);
      writeStateValueUint32(buffer.data + offset, chips[chip].start);
      offset += sizeof(uint32_t);
      writeStateValueUint32(buffer.data + offset, chips[chip].lineSize);
      offset += sizeof(uint32_t);
      writeStateValueUint32(buffer.data + offset, chips[chip].mask);
      offset += sizeof(uint32_t);
      writeStateValueBool(buffer.data + offset, chips[chip].inBootMode);
      offset += sizeof(uint8_t);
      writeStateValueBool(buffer.data + offset, chips[chip].readOnly);
      offset += sizeof(uint8_t);
      writeStateValueBool(buffer.data + offset, chips[chip].readOnlyForProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValueBool(buffer.data + offset, chips[chip].supervisorOnlyProtectedMemory);
      offset += sizeof(uint8_t);
      writeStateValueUint32(buffer.data + offset, chips[chip].unprotectedSize);
      offset += sizeof(uint32_t);
   }

   //SED1376
   memcpy(buffer.data + offset, sed1376Registers, SED1376_REG_SIZE);
   offset += SED1376_REG_SIZE;
   memcpy(buffer.data + offset, sed1376RLut, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(buffer.data + offset, sed1376GLut, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(buffer.data + offset, sed1376BLut, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(buffer.data + offset, sed1376Framebuffer, SED1376_FB_SIZE);
   offset += SED1376_FB_SIZE;

   //timing
   writeStateValueDouble(buffer.data + offset, palmCrystalCycles);
   offset += sizeof(uint64_t);
   writeStateValueDouble(buffer.data + offset, palmCycleCounter);
   offset += sizeof(uint64_t);
   writeStateValueInt32(buffer.data + offset, pllWakeWait);
   offset += sizeof(int32_t);
   writeStateValueUint32(buffer.data + offset, clk32Counter);
   offset += sizeof(uint32_t);
   writeStateValueDouble(buffer.data + offset, timerCycleCounter[0]);
   offset += sizeof(uint64_t);
   writeStateValueDouble(buffer.data + offset, timerCycleCounter[1]);
   offset += sizeof(uint64_t);
   writeStateValueUint16(buffer.data + offset, timerStatusReadAcknowledge[0]);
   offset += sizeof(uint16_t);
   writeStateValueUint16(buffer.data + offset, timerStatusReadAcknowledge[1]);
   offset += sizeof(uint16_t);
   writeStateValueUint32(buffer.data + offset, edgeTriggeredInterruptLastValue);
   offset += sizeof(uint32_t);

   //SPI1
   for(uint8_t fifoPosition = 0; fifoPosition < 8; fifoPosition++){
      writeStateValueUint16(buffer.data + offset, spi1RxFifo[fifoPosition]);
      offset += sizeof(uint16_t);
   }
   for(uint8_t fifoPosition = 0; fifoPosition < 8; fifoPosition++){
      writeStateValueUint16(buffer.data + offset, spi1TxFifo[fifoPosition]);
      offset += sizeof(uint16_t);
   }
   writeStateValueUint8(buffer.data + offset, spi1RxPosition);
   offset += sizeof(uint8_t);
   writeStateValueUint8(buffer.data + offset, spi1TxPosition);
   offset += sizeof(uint8_t);

   //ADS7846
   writeStateValueUint8(buffer.data + offset, ads7846BitsToNextControl);
   offset += sizeof(uint8_t);
   writeStateValueUint8(buffer.data + offset, ads7846ControlByte);
   offset += sizeof(uint8_t);
   writeStateValueBool(buffer.data + offset, ads7846PenIrqEnabled);
   offset += sizeof(uint8_t);
   writeStateValueUint16(buffer.data + offset, ads7846OutputValue);
   offset += sizeof(uint16_t);
   
   //misc
   writeStateValueBool(buffer.data + offset, palmMisc.powerButtonLed);
   offset += sizeof(uint8_t);
   writeStateValueBool(buffer.data + offset, palmMisc.lcdOn);
   offset += sizeof(uint8_t);
   writeStateValueUint8(buffer.data + offset, palmMisc.backlightLevel);
   offset += sizeof(uint8_t);
   writeStateValueBool(buffer.data + offset, palmMisc.vibratorOn);
   offset += sizeof(uint8_t);
   writeStateValueBool(buffer.data + offset, palmMisc.batteryCharging);
   offset += sizeof(uint8_t);
   writeStateValueUint8(buffer.data + offset, palmMisc.batteryLevel);
   offset += sizeof(uint8_t);
   writeStateValueUint8(buffer.data + offset, palmMisc.dataPort);
   offset += sizeof(uint8_t);

   //SD card
   writeStateValueUint64(buffer.data + offset, palmSdCard.size);
   offset += sizeof(uint64_t);
   memcpy(palmSdCard.data, buffer.data + offset, palmSdCard.size);
   offset += palmSdCard.size;

   return true;
}

bool emulatorLoadState(buffer_t buffer){
   uint64_t offset = 0;

   if(buffer.size < emulatorGetStateSize())
      return false;//not a valid state
   
   //state validation, wont load states that are not from the same state version
   if(readStateValueUint32(buffer.data + offset) != SAVE_STATE_VERSION)
      return false;
   offset += sizeof(uint32_t);

   //features
   palmSpecialFeatures = readStateValueUint32(buffer.data + offset);
   offset += sizeof(uint32_t);

   //CPU
   for(uint32_t cpuReg = 0; cpuReg <=  M68K_REG_CAAR; cpuReg++){
      m68k_set_reg(cpuReg, readStateValueUint32(buffer.data + offset));
      offset += sizeof(uint32_t);
   }
   lowPowerStopActive = readStateValueBool(buffer.data + offset);
   offset += 1;
   
   //memory
   if(palmSpecialFeatures & FEATURE_RAM_HUGE){
      memcpy(palmRam, buffer.data + offset, SUPERMASSIVE_RAM_SIZE);
      offset += SUPERMASSIVE_RAM_SIZE;
   }
   else{
      memcpy(palmRam, buffer.data + offset, RAM_SIZE);
      offset += RAM_SIZE;
   }
   memcpy(palmReg, buffer.data + offset, REG_SIZE);
   offset += REG_SIZE;
   memcpy(bankType, buffer.data + offset, TOTAL_MEMORY_BANKS);
   offset += TOTAL_MEMORY_BANKS;
   for(uint32_t chip = CHIP_BEGIN; chip < CHIP_END; chip++){
      chips[chip].enable = readStateValueBool(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[chip].start = readStateValueUint32(buffer.data + offset);
      offset += sizeof(uint32_t);
      chips[chip].lineSize = readStateValueUint32(buffer.data + offset);
      offset += sizeof(uint32_t);
      chips[chip].mask = readStateValueUint32(buffer.data + offset);
      offset += sizeof(uint32_t);
      chips[chip].inBootMode = readStateValueBool(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[chip].readOnly = readStateValueBool(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[chip].readOnlyForProtectedMemory = readStateValueBool(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[chip].supervisorOnlyProtectedMemory = readStateValueBool(buffer.data + offset);
      offset += sizeof(uint8_t);
      chips[chip].unprotectedSize = readStateValueUint32(buffer.data + offset);
      offset += sizeof(uint32_t);
   }

   //SED1376
   memcpy(sed1376Registers, buffer.data + offset, SED1376_REG_SIZE);
   offset += SED1376_REG_SIZE;
   memcpy(sed1376RLut, buffer.data + offset, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(sed1376GLut, buffer.data + offset, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(sed1376BLut, buffer.data + offset, SED1376_LUT_SIZE);
   offset += SED1376_LUT_SIZE;
   memcpy(sed1376Framebuffer, buffer.data + offset, SED1376_FB_SIZE);
   offset += SED1376_FB_SIZE;
   sed1376RefreshLut();

   //timing
   palmCrystalCycles = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   palmCycleCounter = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   pllWakeWait = readStateValueInt32(buffer.data + offset);
   offset += sizeof(int32_t);
   clk32Counter = readStateValueUint32(buffer.data + offset);
   offset += sizeof(uint32_t);
   timerCycleCounter[0] = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   timerCycleCounter[1] = readStateValueDouble(buffer.data + offset);
   offset += sizeof(uint64_t);
   timerStatusReadAcknowledge[0] = readStateValueUint16(buffer.data + offset);
   offset += sizeof(uint16_t);
   timerStatusReadAcknowledge[1] = readStateValueUint16(buffer.data + offset);
   offset += sizeof(uint16_t);
   edgeTriggeredInterruptLastValue = readStateValueUint32(buffer.data + offset);
   offset += sizeof(uint32_t);

   //SPI1
   for(uint8_t fifoPosition = 0; fifoPosition < 8; fifoPosition++){
      spi1RxFifo[fifoPosition] = readStateValueUint16(buffer.data + offset);
      offset += sizeof(uint16_t);
   }
   for(uint8_t fifoPosition = 0; fifoPosition < 8; fifoPosition++){
      spi1TxFifo[fifoPosition] = readStateValueUint16(buffer.data + offset);
      offset += sizeof(uint16_t);
   }
   spi1RxPosition = readStateValueUint8(buffer.data + offset);
   offset += sizeof(uint8_t);
   spi1TxPosition = readStateValueUint8(buffer.data + offset);
   offset += sizeof(uint8_t);

   //ADS7846
   ads7846BitsToNextControl = readStateValueUint8(buffer.data + offset);
   offset += sizeof(uint8_t);
   ads7846ControlByte = readStateValueUint8(buffer.data + offset);
   offset += sizeof(uint8_t);
   ads7846PenIrqEnabled = readStateValueBool(buffer.data + offset);
   offset += sizeof(uint8_t);
   ads7846OutputValue = readStateValueUint16(buffer.data + offset);
   offset += sizeof(uint16_t);
   
   //misc
   palmMisc.powerButtonLed = readStateValueBool(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.lcdOn = readStateValueBool(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.backlightLevel = readStateValueUint8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.vibratorOn = readStateValueBool(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryCharging = readStateValueBool(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryLevel = readStateValueUint8(buffer.data + offset);
   offset += sizeof(uint8_t);
   palmMisc.dataPort = readStateValueUint8(buffer.data + offset);
   offset += sizeof(uint8_t);

   //SD card
   if(palmSdCard.data){
      free(palmSdCard.data);
      palmSdCard.data = NULL;
   }
   palmSdCard.size = readStateValueUint64(buffer.data + offset);
   offset += sizeof(uint64_t);
   if(palmSdCard.size > 0){
      palmSdCard.data = malloc(palmSdCard.size);

      if(!palmSdCard.data)
         return false;

      memcpy(palmSdCard.data, buffer.data + offset, palmSdCard.size);
   }
   offset += palmSdCard.size;

   return true;
}

buffer_t emulatorGetRamBuffer(){
   buffer_t ramInfo;

   ramInfo.data = palmRam;
   ramInfo.size = palmSpecialFeatures & FEATURE_RAM_HUGE ? SUPERMASSIVE_RAM_SIZE : RAM_SIZE;

   return ramInfo;
}

buffer_t emulatorGetSdCardBuffer(){
   return palmSdCard;
}

uint32_t emulatorInsertSdCard(buffer_t image){
   //SD card is currently inserted
   if(palmSdCard.data)
      return EMU_ERROR_RESOURCE_LOCKED;

   palmSdCard.data = malloc(image.size);
   if(!palmSdCard.data)
      return EMU_ERROR_OUT_OF_MEMORY;

   if(image.data)
      memcpy(palmSdCard.data, image.data, image.size);
   else
      memset(palmSdCard.data, 0x00, image.size);

   palmSdCard.size = image.size;

   return EMU_ERROR_NONE;
}

void emulatorEjectSdCard(){
   if(palmSdCard.data)
      free(palmSdCard.data);
   palmSdCard.data = NULL;
   palmSdCard.size = 0;
}

uint32_t emulatorInstallPrcPdb(buffer_t file){
   //pretend to pass for now
   //return EMU_ERROR_NOT_IMPLEMENTED;
   return EMU_ERROR_NONE;
}

void emulateFrame(){
   refreshInputState();

   static uint16_t oldTouchX = 0;
   static uint16_t oldTouchY = 0;
   static bool oldTouchDown = false;
   if(palmInput.touchscreenX != oldTouchX || palmInput.touchscreenY != oldTouchY || palmInput.touchscreenTouched != oldTouchDown){
      sandboxTest(SANDBOX_SEND_OS_TOUCH);
      oldTouchX = palmInput.touchscreenX;
      oldTouchY = palmInput.touchscreenY;
      oldTouchDown = palmInput.touchscreenTouched;
   }

   //hack, this is just an easy way to use the sandbox without attaching it to the emulator frontend
   /*
   static bool allowRun1 = true;
   if(palmInput.buttonUp){
      palmInput.buttonUp = false;

      if(allowRun1){
         sandboxTest(SANDBOX_SEND_OS_TOUCH);
         allowRun1 = false;
      }
   }
   else{
      allowRun1 = true;
   }
   */

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
