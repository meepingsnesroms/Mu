#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <boolean.h>

#include "m68k/m68k.h"
#include "emulator.h"
#include "hardwareRegisters.h"
#include "sed1376.h"


//Memory Map of Palm m515
//0x00000000-0x00FFFFFF RAM, the first 256(0x100) bytes is copyed from the first 256 bytes of ROM before boot, this applys to all palms with the 68k architecture
//0x10000000-0x103FFFFF ROM, palmos41-en-m515.rom, substitute "en" for your language code
//0x1FF80000-0x1FF800B3 sed1376(Display Controller) Registers
//0x1FFA0000-0x1FFB3FFF sed1376(Display Controller) Framebuffer, this is not the same as the palm framebuffer which is always 16 bit color,
//this buffer must be processed depending on whats in the sed1376 registers, the result is the palm framebuffer
//0xFFFFF000-0xFFFFFDFF Hardware Registers
//0xFFFFFF00-0xFFFFFFFF Bootloader, pesumably does the 256 byte ROM to RAM copy, never been dumped


uint8_t  palmRam[RAM_SIZE];
uint8_t  palmRom[ROM_SIZE];
uint8_t  palmReg[REG_SIZE];
uint32_t palmCpuFrequency;//cycles per second
uint32_t palmCrystalCycles;//how many cycles before toggling the 32.768 kHz crystal
uint32_t palmCycleCounter;//can be greater then 0 if too many cycles where run
uint32_t palmRtcFrameCounter;//when this reaches EMU_FPS increment the real time clock

uint16_t palmFramebuffer[160 * 240];//really 160*160, the extra pixels are the silkscreened digitizer area
uint16_t palmButtonState;
uint16_t palmTouchscreenX;
uint16_t palmTouchscreenY;
bool     palmTouchscreenTouched;
uint32_t palmClockMultiplier;//used by the emulator to overclock the emulated palm


void emulatorInit(uint8_t* palmRomDump){
   m68k_init();
   m68k_set_cpu_type(M68K_CPU_TYPE_68020);
   
   memset(palmRam, 0x00, RAM_SIZE);
   memcpy(palmRom, palmRomDump, ROM_SIZE);
   memcpy(palmRam, palmRom, 256);//copy ROM header
   resetHwRegisters();
   sed1376Reset();
   palmCrystalCycles = 2 * (14 * (71/*p*/ + 1) + 3/*q*/ + 1) / 2/*prescaler1*/;
   palmCpuFrequency = palmCrystalCycles * CRYSTAL_FREQUENCY;
   palmCycleCounter = 0;
   palmRtcFrameCounter = 0;
   
   palmClockMultiplier = 1;//Overclock disabled
   
   m68k_set_reset_instr_callback(emulatorReset);
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
   memcpy(data + offset, &palmCpuFrequency, sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(data + offset, &palmCrystalCycles, sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(data + offset, &palmCycleCounter, sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(data + offset, &palmRtcFrameCounter, sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(data + offset, &rtiInterruptCounter, sizeof(uint32_t));
   offset += sizeof(uint32_t);
}

void emulatorLoadState(uint8_t* data){
   uint32_t offset = 0;
   m68k_set_context(data + offset);
   offset += m68k_context_size();
   memcpy(palmRam, data + offset, RAM_SIZE);
   offset += RAM_SIZE;
   memcpy(palmReg, data + offset, REG_SIZE);
   offset += REG_SIZE;
   memcpy(&palmCpuFrequency, data + offset, sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(&palmCrystalCycles, data + offset, sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(&palmCycleCounter, data + offset, sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(&palmRtcFrameCounter, data + offset, sizeof(uint32_t));
   offset += sizeof(uint32_t);
   memcpy(&rtiInterruptCounter, data + offset, sizeof(uint32_t));
   offset += sizeof(uint32_t);
}

uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size){
   return EMU_ERROR_NOT_IMPLEMENTED;
}

void emulateFrame(){
   while(palmCycleCounter < palmCpuFrequency / EMU_FPS){
      if(cpuIsOn())
         palmCycleCounter += m68k_execute(palmCrystalCycles * palmClockMultiplier) / palmClockMultiplier;//normaly 33mhz / 60fps
      else
         palmCycleCounter += palmCrystalCycles;
      clk32();
   }
   palmCycleCounter -= palmCpuFrequency / EMU_FPS;
   
   palmRtcFrameCounter++;
   if(palmRtcFrameCounter >= EMU_FPS){
      rtcAddSecond();
      palmRtcFrameCounter = 0;
   }
   printf("Ran frame, executed %d cycles.\n", palmCycleCounter + palmCpuFrequency / EMU_FPS);
}
