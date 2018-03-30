#pragma once

#include <stdint.h>

#include <boolean.h>

//emu errors
enum emu_error_t{
   EMU_ERROR_NONE = 0,
   EMU_ERROR_NOT_IMPLEMENTED,
   EMU_ERROR_NOT_DOCUMENTED
};

//types
typedef struct{
   uint16_t buttonState;
   uint16_t touchscreenX;
   uint16_t touchscreenY;
   bool     touchscreenTouched;
}input_t;

typedef struct{
   void (*getSdCardChunk)(uint8_t* data, uint32_t size);
   void (*setSdCardChunk)(uint8_t* data, uint32_t size);
   bool inserted;
}sdcard_t;

//buttons
#define buttonUp 0
//etc...

//special features, these make the emulator inaccurate in a good way, but still inaccurate and are therefore optional, they dont do anything yet
#define ACCURATE                0x0000 //no hacks/addons
#define INACCURATE_RAM_HUGE     0x0001 //128 mb ram
#define INACCURATE_FAST_CPU     0x0002 //doubles cpu speed
#define INACCURATE_HYBRID_CPU   0x0004 //allows running arm opcodes in an OS 4 enviroment

//config options
#define EMU_FPS 60.0
#define CRYSTAL_FREQUENCY 32768.0
#define CPU_FREQUENCY (palmCrystalCycles * CRYSTAL_FREQUENCY)

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
extern input_t  palmIo;
extern sdcard_t palmSdCard;
extern uint16_t palmFramebuffer[];
extern double   palmCrystalCycles;
extern double   palmCycleCounter;
extern double   palmClockMultiplier;

//functions
void emulatorInit(uint8_t* palmRomDump, uint16_t specialFeatures);
void emulatorReset();
void emulatorSetRtc(uint32_t days, uint32_t hours, uint32_t minutes, uint32_t seconds);
uint32_t emulatorGetStateSize();
void emulatorSaveState(uint8_t* data);
void emulatorLoadState(uint8_t* data);
uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size);
void emulateFrame();
