#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <boolean.h>

#include "m68k/m68k.h"
#include "emulator.h"


//Memory Map of Palm m515
//0x00000000-0x01000000 RAM, the first 256(0x100) bytes is copyed from the first 256 bytes of ROM before boot, this applys to all palms with the 68k architecture
//0x10000000-0x10400000 ROM, palmos41-en-m515.rom, substitute "en" for your language code
//0xFFFFF000-0xFFFFFDFF Hardware Registers
//0xFFFFFF00-0xFFFFFFFF Bootloader, pesumably does the 256 byte ROM to RAM copy, never been dumped


uint16_t palmFramebuffer[160 * 240];//really 160*160, the extra pixels are the silkscreened digitizer area
uint8_t  palmRam[RAM_SIZE];
uint8_t  palmRom[ROM_SIZE];
uint8_t  palmReg[REG_SIZE];
uint32_t palmCpuFrequency;//cycles per second
uint32_t palmCrystalCycles;//how many cycles before toggling the 32.768 kHz crystal
uint32_t palmCycleCounter;//can be greater then 0 if too many cycles where run
bool     palmCrystal;//current crystal state
uint32_t palmRtcFrameCounter;//when this reaches EMU_FPS increment the real time clock

uint16_t palmButtonState;
uint16_t palmTouchscreenX;
uint16_t palmTouchscreenY;
bool     palmTouchscreenTouched;


void emulatorInit(uint8_t* palmRomDump){
   m68k_init();
   m68k_set_cpu_type(M68K_CPU_TYPE_68020);
   
   memset(palmRam, 0x00, RAM_SIZE);
   memset(palmReg, 0x00, REG_SIZE);
   memcpy(palmRom, palmRomDump, ROM_SIZE);
   memcpy(palmRam, palmRom, 256);//copy ROM header
   initHwRegisters();
   palmCrystalCycles = 14 * (71 + 1) + 3 + 1;
   palmCpuFrequency = palmCrystalCycles * 32768;
   palmCycleCounter = 0;
   palmCrystal = false;
   palmRtcFrameCounter = 0;
   
   m68k_pulse_reset();
   
   //printf("Boot ProgramCounter is:0x%08X\n", m68k_get_reg(NULL, M68K_REG_PC));
}

void emulatorReset(){
   m68k_pulse_reset();
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
   data[offset] = palmCrystal;
   offset += 1;
   memcpy(data + offset, &palmRtcFrameCounter, sizeof(uint32_t));
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
   palmCrystal = data[offset];
   offset += 1;
   memcpy(&palmRtcFrameCounter, data + offset, sizeof(uint32_t));
   offset += sizeof(uint32_t);
}

uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size){
   return EMU_ERROR_NOT_IMPLEMENTED;
}

void emulateFrame(){
   while(palmCycleCounter < palmCpuFrequency / EMU_FPS){
      palmCycleCounter += m68k_execute(palmCrystalCycles);//normaly 33mhz / 60fps
      palmCrystal = !palmCrystal;
   }
   palmCycleCounter -= palmCpuFrequency / EMU_FPS;
   
   palmRtcFrameCounter++;
   if(palmRtcFrameCounter >= EMU_FPS){
      rtcAddSecond();
      palmRtcFrameCounter = 0;
   }
   printf("Ran frame, executed %d cycles.\n", palmCycleCounter + palmCpuFrequency / EMU_FPS);
}
