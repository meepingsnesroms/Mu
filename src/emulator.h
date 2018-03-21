#pragma once

#include <stdint.h>

#include <boolean.h>

//emu errors
enum emu_error_t{
   EMU_ERROR_NONE = 0,
   EMU_ERROR_NOT_IMPLEMENTED
};

//buttons
#define buttonUp 0
//etc...

//chip addresses
#define RAM_START_ADDRESS 0x00000000
#define ROM_START_ADDRESS 0x10000000
#define REG_START_ADDRESS 0xFFFFF000
#define RAM_SIZE (16 * 0x100000)//16mb ram
#define ROM_SIZE (4 * 0x100000)//4mb rom
#define REG_SIZE 0xDFF

//emulator data
extern uint16_t palmFramebuffer[];
extern uint8_t  palmRam[];
extern uint8_t  palmRom[];
extern uint16_t palmButtonState;
extern uint16_t palmTouchscreenX;
extern uint16_t palmTouchscreenY;
extern bool     palmTouchscreenTouched;

//functions
void emulatorInit(uint8_t* palmRomDump);
void emulatorReset();
uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size);
void emulateFrame();
