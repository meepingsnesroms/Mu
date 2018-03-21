#pragma once

#include <stdint.h>

#include <boolean.h>

#define buttonUp 0
//etc...

#define RAM_START_ADDRESS 0x00000000
#define ROM_START_ADDRESS 0x10000000
#define REG_START_ADDRESS 0xFFFFF000
#define RAM_SIZE (16 * 0x100000)//16mb ram
#define ROM_SIZE (4 * 0x100000)//4mb rom
#define REG_SIZE 0xDFF


extern uint16_t palmFramebuffer[];
extern uint8_t  palmRam[];
extern uint8_t  palmRom[];
extern uint16_t palmButtonState;
extern uint16_t palmTouchscreenX;
extern uint16_t palmTouchscreenY;
extern bool     palmTouchscreenTouched;

void emulatorInit(uint8_t* palmRomDump);
void emulatorReset();
uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size);
void emulateFrame();
