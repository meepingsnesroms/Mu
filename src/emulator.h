#pragma once

#include <stdint.h>

#include <boolean.h>

//emu errors
enum emu_error_t{
   EMU_ERROR_NONE = 0,
   EMU_ERROR_NOT_IMPLEMENTED,
   EMU_ERROR_NOT_DOCUMENTED
};

//buttons
#define buttonUp 0
//etc...

//config options
#define EMU_FPS 60
#define CRYSTAL_FREQUENCY 32768

//memory chip addresses
#define RAM_START_ADDRESS 0x00000000
#define ROM_START_ADDRESS 0x10000000
#define REG_START_ADDRESS 0xFFFFF000
#define RAM_SIZE (16 * 0x100000)//16mb ram
#define ROM_SIZE (4 * 0x100000)//4mb rom
#define REG_SIZE 0xE00

//display chip addresses
#define SED1376_REG_START_ADDRESS 0x1FF80000
#define SED1376_FB_START_ADDRESS 0x1FFA0000
#define SED1376_REG_SIZE 0xB4//it has 0x20000 used address space entrys but only 0xB4 registers
#define SED1376_FB_SIZE  0x14000//likely also has 0x20000 used address space entrys

//emulator data
extern uint8_t  palmRam[];
extern uint8_t  palmRom[];
extern uint8_t  palmReg[];
extern uint32_t palmCpuFrequency;
extern uint32_t palmCrystalCycles;
extern uint32_t palmCycleCounter;
extern uint32_t palmRtcFrameCounter;

//i/o
extern uint16_t palmFramebuffer[];
extern uint16_t palmButtonState;
extern uint16_t palmTouchscreenX;
extern uint16_t palmTouchscreenY;
extern bool     palmTouchscreenTouched;
extern uint32_t palmClockMultiplier;

//functions
void emulatorInit(uint8_t* palmRomDump);
void emulatorReset();
void emulatorSetRtc(uint32_t days, uint32_t hours, uint32_t minutes, uint32_t seconds);
uint32_t emulatorGetStateSize();
void emulatorSaveState(uint8_t* data);
void emulatorLoadState(uint8_t* data);
uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size);
void emulateFrame();
