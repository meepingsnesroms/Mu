#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <boolean.h>

#include "m68k/m68k.h"
#include "cpu32Opcodes.h"
#include "emulator.h"
#include "hardwareRegisters.h"
#include "sed1376.h"
#include "silkscreen.h"


//Memory Map of Palm m515
//0x00000000-0x00FFFFFF RAM, the first 256(0x100) bytes is copyed from the first 256 bytes of ROM before boot, this applys to all palms with the 68k architecture
//0x10000000-0x103FFFFF ROM, palmos41-en-m515.rom, substitute "en" for your language code
//0x1FF80000-0x1FF800B3 sed1376(Display Controller) Registers
//0x1FFA0000-0x1FFB3FFF sed1376(Display Controller) Framebuffer, this is not the same as the palm framebuffer which is always 16 bit color,
//this buffer must be processed depending on whats in the sed1376 registers, the result is the palm framebuffer
//0xFFFFF000-0xFFFFFDFF Hardware Registers
//0xFFFFFF00-0xFFFFFFFF Bootloader, pesumably does the 256 byte ROM to RAM copy, never been dumped


uint8_t   palmRam[RAM_SIZE];
uint8_t   palmRom[ROM_SIZE];
uint8_t   palmReg[REG_SIZE];
input_t   palmIo;
sdcard_t  palmSdCard;
misc_hw_t palmMisc;
uint16_t  palmFramebuffer[160 * (160 + 60)];//really 160*160, the extra pixels are the silkscreened digitizer area
double    palmCrystalCycles;//how many cycles before toggling the 32.768 kHz crystal
double    palmCycleCounter;//can be greater then 0 if too many cycles where run
double    palmClockMultiplier;//used by the emulator to overclock the emulated palm


void emulatorInit(uint8_t* palmRomDump, uint16_t specialFeatures){
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
   sed1376Reset();
   
   //interfaces
   palmIo.buttonUp = false;
   palmIo.buttonDown = false;
   palmIo.buttonLeft = false;//only used in hybrid mode
   palmIo.buttonRight = false;//only used in hybrid mode
   palmIo.buttonCalender = false;
   palmIo.buttonAddress = false;
   palmIo.buttonTodo = false;
   palmIo.buttonNotes = false;
   palmIo.buttonPower = false;
   palmIo.buttonContrast = false;
   palmIo.touchscreenX = 0;
   palmIo.touchscreenY = 0;
   palmIo.touchscreenTouched = false;
   
   palmSdCard.getSdCardChunk = NULL;
   palmSdCard.setSdCardChunk = NULL;
   palmSdCard.size = 0;
   palmSdCard.inserted = false;
   
   palmMisc.powerButtonLed = false;
   palmMisc.batteryCharging = false;
   palmMisc.batteryLevel = 100;
   
   //config
   palmClockMultiplier = (specialFeatures & INACCURATE_FAST_CPU) ? 2.0 : 1.0;//Overclock disabled
   
   //start running
   m68k_pulse_reset();
}

void emulatorReset(){
   //reset doesnt clear ram, all programs are stored in ram
   resetHwRegisters();
   sed1376Reset();
   m68k_pulse_reset();
}

void emulatorSetRtc(uint32_t days, uint32_t hours, uint32_t minutes, uint32_t seconds){
   setRtc(days, hours, minutes, seconds);
}

uint32_t emulatorGetStateSize(){
   return RAM_SIZE + REG_SIZE + m68k_context_size();
}

void emulatorSaveState(uint8_t* data){
   uint32_t offset = 0;
   m68k_get_context(data + offset);
   offset += m68k_context_size();
   memcpy(data + offset, palmRam, RAM_SIZE);
   offset += RAM_SIZE;
   memcpy(data + offset, palmReg, REG_SIZE);
   offset += REG_SIZE;
   memcpy(data + offset, &palmCrystalCycles, sizeof(double));
   offset += sizeof(double);
   memcpy(data + offset, &palmCycleCounter, sizeof(double));
   offset += sizeof(double);
   memcpy(data + offset, &clk32Counter, sizeof(uint32_t));
   offset += sizeof(double);
   memcpy(data + offset, &timer1CycleCounter, sizeof(uint32_t));
   offset += sizeof(double);
   memcpy(data + offset, &timer2CycleCounter, sizeof(uint32_t));
   offset += sizeof(double);
   data[offset] = lowPowerStopActive;
   offset += 1;
}

void emulatorLoadState(uint8_t* data){
   uint32_t offset = 0;
   m68k_set_context(data + offset);
   offset += m68k_context_size();
   memcpy(palmRam, data + offset, RAM_SIZE);
   offset += RAM_SIZE;
   memcpy(palmReg, data + offset, REG_SIZE);
   offset += REG_SIZE;
   memcpy(&palmCrystalCycles, data + offset, sizeof(double));
   offset += sizeof(double);
   memcpy(&palmCycleCounter, data + offset, sizeof(double));
   offset += sizeof(double);
   memcpy(&clk32Counter, data + offset, sizeof(double));
   offset += sizeof(double);
   memcpy(&timer1CycleCounter, data + offset, sizeof(uint32_t));
   offset += sizeof(double);
   memcpy(&timer2CycleCounter, data + offset, sizeof(uint32_t));
   offset += sizeof(double);
   lowPowerStopActive = data[offset];
   offset += 1;
}

uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size){
   return EMU_ERROR_NOT_IMPLEMENTED;
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
