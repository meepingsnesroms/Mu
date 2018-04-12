#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <boolean.h>

#include "m68k/m68k.h"
#include "cpu32Opcodes.h"
#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "sed1376.h"
#include "sdcard.h"
#include "silkscreen.h"
#include "portability.h"
#include "emuFeatureRegistersSpec.h"


//Memory Map of Palm m515
//0x00000000-0x00FFFFFF RAM, the first 256(0x100) bytes is copyed from the first 256 bytes of ROM before boot, this applys to all palms with the 68k architecture
//0x10000000-0x103FFFFF ROM, palmos41-en-m515.rom, substitute "en" for your language code
//0x1FF80000-0x1FF800B3 sed1376(Display Controller) Registers
//0x1FFA0000-0x1FFB3FFF sed1376(Display Controller) Framebuffer, this is not the same as the palm framebuffer which is always 16 bit color,
//this buffer must be processed depending on whats in the sed1376 registers, the result is the palm framebuffer
//0xFFFFF000-0xFFFFFDFF Hardware Registers
//0xFFFFFF00-0xFFFFFFFF Bootloader, pesumably does the 256 byte ROM to RAM copy, never been dumped


uint8_t   palmRam[RAM_SIZE + 3];//+ 3 to prevent 32 bit writes on last byte from corrupting memory
uint8_t   palmRom[ROM_SIZE + 3];//+ 3 to prevent 32 bit writes on last byte from corrupting memory
uint8_t   palmReg[REG_SIZE + 3];//+ 3 to prevent 32 bit writes on last byte from corrupting memory
input_t   palmInput;
sdcard_t  palmSdCard;
misc_hw_t palmMisc;
uint16_t  palmFramebuffer[160 * (160 + 60)];//really 160*160, the extra pixels are the silkscreened digitizer area
uint32_t  palmSpecialFeatures;
double    palmCrystalCycles;//how many cycles before toggling the 32.768 kHz crystal
double    palmCycleCounter;//can be greater then 0 if too many cycles where run
double    palmClockMultiplier;//used by the emulator to overclock the emulated palm

uint64_t (*emulatorGetSysTime)();
uint64_t* (*emulatorGetSdCardStateChunkList)(uint64_t sessionId, uint64_t stateId);//returns the bps chunkIds for a stateId in the order they need to be applied
void (*emulatorSetSdCardStateChunkList)(uint64_t sessionId, uint64_t stateId, uint64_t* data);//sets the bps chunkIds for a stateId in the order they need to be applied
uint8_t* (*emulatorGetSdCardChunk)(uint64_t sessionId, uint64_t chunkId);
void (*emulatorSetSdCardChunk)(uint64_t sessionId, uint64_t chunkId, uint8_t* data, uint64_t size);


static inline bool allSdCardCallbacksPresent(){
   if(emulatorGetSysTime && emulatorGetSdCardStateChunkList && emulatorSetSdCardStateChunkList && emulatorGetSdCardChunk && emulatorSetSdCardChunk)
      return true;
   return false;
}


void emulatorInit(uint8_t* palmRomDump, uint32_t specialFeatures){
   //cpu
   m68k_init();
   m68k_set_cpu_type(M68K_CPU_TYPE_68020);
   m68k_set_reset_instr_callback(emulatorReset);
   m68k_set_int_ack_callback(interruptAcknowledge);
   patchMusashiOpcodeHandlerCpu32();
   resetHwRegisters();
   lowPowerStopActive = false;
   palmCrystalCycles = 2.0 * (14.0 * (71.0/*p*/ + 1.0) + 3.0/*q*/ + 1.0) / 2.0/*prescaler1*/;
   palmCycleCounter = 0.0;
   
   //memory
   memset(palmRam, 0x00, RAM_SIZE);
   memcpy(palmRom, palmRomDump, ROM_SIZE);
   memcpy(palmRam, palmRom, 256);//copy ROM header
   memcpy(&palmFramebuffer[160 * 160], silkscreenData, SILKSCREEN_WIDTH * SILKSCREEN_HEIGHT * (SILKSCREEN_BPP / 8));
   resetAddressSpace();
   sed1376Reset();
   sdCardInit();
   
   //input
   palmInput.buttonUp = false;
   palmInput.buttonDown = false;
   palmInput.buttonLeft = false;//only used in hybrid mode
   palmInput.buttonRight = false;//only used in hybrid mode
   palmInput.buttonSelect = false;//only used in hybrid mode
   
   palmInput.buttonCalender = false;
   palmInput.buttonAddress = false;
   palmInput.buttonTodo = false;
   palmInput.buttonNotes = false;
   
   palmInput.buttonPower = false;
   palmInput.buttonContrast = false;
   
   palmInput.touchscreenX = 0;
   palmInput.touchscreenY = 0;
   palmInput.touchscreenTouched = false;
   
   //sdcard
   palmSdCard.sessionId = 0x0000000000000000;
   palmSdCard.stateId = 0x0000000000000000;
   palmSdCard.size = 0;
   palmSdCard.type = CARD_NONE;
   palmSdCard.inserted = false;
   
   //misc attributes
   palmMisc.batteryCharging = false;
   palmMisc.batteryLevel = 100;
   
   //config
   palmClockMultiplier = (specialFeatures & FEATURE_FAST_CPU) ? 2.0 : 1.0;//overclock
   palmSpecialFeatures = specialFeatures;
   
   //start running
   m68k_pulse_reset();
   
   /*
   double test = 342553325.13436322;
   uint64_t fixed3232 = getUint64FromDouble(test);
   double rebuilt = getDoubleFromUint64(fixed3232);
   printf("Original double:%f\n", test);
   printf("Fixed 32.32:0x%08lX\n", fixed3232);
   printf("Rebuilt double:%f\n", rebuilt);
   */
}

void emulatorExit(){
   sdCardExit();
}

void emulatorReset(){
   //reset doesnt clear ram or sdcard, all programs are stored in ram or on sdcard
   resetHwRegisters();
   resetAddressSpace();//address space must be reset after hardware registers because it is dependant on them
   sed1376Reset();
   m68k_pulse_reset();
}

void emulatorSetRtc(uint32_t days, uint32_t hours, uint32_t minutes, uint32_t seconds){
   setRtc(days, hours, minutes, seconds);
}

uint32_t emulatorSetSdCard(uint64_t size, uint8_t type){
   if(!allSdCardCallbacksPresent())
      return EMU_ERROR_CALLBACKS_NOT_SET;
   
   palmSdCard.sessionId = emulatorGetSysTime();
   palmSdCard.stateId = 0x0000000000000000;//set when saving state
   palmSdCard.size = size;
   palmSdCard.type = type;
   palmSdCard.inserted = true;
   
   return EMU_ERROR_NONE;
}

uint32_t emulatorGetStateSize(){
   uint32_t size = 0;
   
   size += sizeof(uint32_t) * (M68K_REG_CAAR + 1);//cpu registers
   size += sizeof(uint8_t);//lowPowerStopActive
   size += RAM_SIZE;//system ram buffer
   size += REG_SIZE;//hardware registers
   size += TOTAL_MEMORY_BANKS;//bank handlers
   size += sizeof(uint64_t) * 3;//palmSdCard
   size += sizeof(uint8_t) * 2;//palmSdCard
   size += sizeof(uint64_t) * 4;//32.32 fixed point double precision timers
   size += sizeof(uint32_t);//clk32Counter
   size += sizeof(uint8_t) * 6;//palmMisc
   size += sizeof(uint32_t);//palmSpecialFeatures
   
   return size;
}

void emulatorSaveState(uint8_t* data){
   uint32_t offset = 0;
   
   //update sdcard struct and save sdcard data
   if(allSdCardCallbacksPresent() && palmSdCard.type != CARD_NONE){
      uint64_t stateId = emulatorGetSysTime();
      sdCardSaveState(palmSdCard.sessionId, stateId);
      palmSdCard.stateId = stateId;
   }
   
   //cpu
   for(uint32_t cpuReg = 0; cpuReg <=  M68K_REG_CAAR; cpuReg++){
      writeStateValueUint32(data + offset, m68k_get_reg(NULL, cpuReg));
      offset += sizeof(uint32_t);
   }
   writeStateValueBool(data + offset, lowPowerStopActive);
   offset += sizeof(uint8_t);
   
   //memory
   memcpy(data + offset, palmRam, RAM_SIZE);
   offset += RAM_SIZE;
   memcpy(data + offset, palmReg, REG_SIZE);
   offset += REG_SIZE;
   memcpy(data + offset, bankType, TOTAL_MEMORY_BANKS);
   offset += TOTAL_MEMORY_BANKS;

   //sdcard
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
   writeStateValueUint32(data + offset, clk32Counter);
   offset += sizeof(uint32_t);
   writeStateValueDouble(data + offset, timer1CycleCounter);
   offset += sizeof(uint64_t);
   writeStateValueDouble(data + offset, timer2CycleCounter);
   offset += sizeof(uint64_t);
   
   //misc
   writeStateValueBool(data + offset, palmMisc.alarmLed);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, palmMisc.lcdOn);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, palmMisc.backlightOn);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, palmMisc.vibratorOn);
   offset += sizeof(uint8_t);
   writeStateValueBool(data + offset, palmMisc.batteryCharging);
   offset += sizeof(uint8_t);
   writeStateValueUint8(data + offset, palmMisc.batteryLevel);
   offset += sizeof(uint8_t);

   //features
   writeStateValueUint32(data + offset, palmSpecialFeatures);
   offset += sizeof(uint32_t);
}

void emulatorLoadState(uint8_t* data){
   uint32_t offset = 0;
   
   //cpu
   for(uint32_t cpuReg = 0; cpuReg <=  M68K_REG_CAAR; cpuReg++){
      m68k_set_reg(cpuReg, readStateValueUint32(data + offset));
      offset += sizeof(uint32_t);
   }
   lowPowerStopActive = readStateValueBool(data + offset);
   offset += 1;
   
   //memory
   memcpy(palmRam, data + offset, RAM_SIZE);
   offset += RAM_SIZE;
   memcpy(palmReg, data + offset, REG_SIZE);
   offset += REG_SIZE;
   memcpy(bankType, data + offset, TOTAL_MEMORY_BANKS);
   offset += TOTAL_MEMORY_BANKS;

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

   //timing
   palmCrystalCycles = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   palmCycleCounter = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   clk32Counter = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   timer1CycleCounter = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   timer2CycleCounter = readStateValueDouble(data + offset);
   offset += sizeof(uint64_t);
   
   //misc
   palmMisc.alarmLed = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.lcdOn = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.backlightOn = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.vibratorOn = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryCharging = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   palmMisc.batteryLevel = readStateValueUint8(data + offset);
   offset += sizeof(uint8_t);

   //features
   palmSpecialFeatures = readStateValueUint32(data + offset);
   offset += sizeof(uint32_t);
   
   refreshBankHandlers();
   
   //update sdcard data from sdcard struct
   if(allSdCardCallbacksPresent() && palmSdCard.type != CARD_NONE){
      sdCardLoadState(palmSdCard.sessionId, palmSdCard.stateId);
   }
}

uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size){
   //pretend to pass for now
   //return EMU_ERROR_NOT_IMPLEMENTED;
   return EMU_ERROR_NONE;
}

void emulateFrame(){
   while(palmCycleCounter < CPU_FREQUENCY / EMU_FPS){
      if(cpuIsOn())
         palmCycleCounter += m68k_execute(palmCrystalCycles * palmClockMultiplier) / palmClockMultiplier;//normaly 33mhz / 60fps
      else
         palmCycleCounter += palmCrystalCycles;
      clk32();
   }
   palmCycleCounter -= CPU_FREQUENCY / EMU_FPS;

   printf("Ran frame, executed %f cycles.\n", palmCycleCounter + CPU_FREQUENCY / EMU_FPS);
}
