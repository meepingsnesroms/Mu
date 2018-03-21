#include <stdint.h>

#include <boolean.h>

#include "m68k/m68k.h"
#include "emulator.h"

//Memory Map of Palm m515
//0x00000000-0x01000000 RAM, the first 256(0x100) bytes is copyed from the first 256 bytes of ROM before boot, this applys to all palms with the 68k architecture
//0x10000000-0x10400000 ROM, palmos41-en-m515.rom, substitute "en" for your language code
//0xFFFFF000-0xFFFFFDFF Hardware Registers
//0xFFFFFF00-0xFFFFFFFF Bootloader, pesumably does the 256 byte ROM to RAM copy, never been dumped


uint16_t palmFramebuffer[160 * 160];
uint8_t  palmRam[RAM_SIZE];
uint8_t  palmRom[ROM_SIZE];
uint16_t palmButtonState;
uint16_t palmTouchscreenX;
uint16_t palmTouchscreenY;
bool     palmTouchscreenTouched;


void emulatorInit(uint8_t* palmRomDump){
   m68k_init();
   m68k_set_cpu_type(M68K_CPU_TYPE_68EC000);
   
   memset(palmRam, 0x00, RAM_SIZE);
   memcpy(palmRom, palmRomDump, ROM_SIZE);
   memcpy(palmRam, palmRom, 256);//copy ROM header
   
   m68k_pulse_reset();

   
}

uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size){
   
}

void emulateFrame(){
   int cycles = m68k_execute(33000000 / 60);//33mhz / 60fps
}
