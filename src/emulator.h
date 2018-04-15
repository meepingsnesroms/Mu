#pragma once
//This is the only header a frontend needs to include

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include <boolean.h>

#ifdef EMU_DEBUG
#define debugLog(...) printf(__VA_ARGS__)
#else
#define debugLog(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "emuFeatureRegistersSpec.h"

//emu errors
enum{
   EMU_ERROR_NONE = 0,
   EMU_ERROR_NOT_IMPLEMENTED,
   EMU_ERROR_CALLBACKS_NOT_SET,
   EMU_ERROR_OUT_OF_MEMORY,
   EMU_ERROR_NOT_DOCUMENTED
};

//sdcard types
enum{
   CARD_NONE = 0,
   CARD_SD,
   CARD_MMC
};

//memory banks
enum{
   EMPTY_BANK = 0,
   RAM_BANK,
   ROM_BANK,
   REG_BANK,
   SED1376_REG_BANK,
   SED1376_FB_BANK
};

//types
typedef struct{
   bool     buttonUp;
   bool     buttonDown;
   bool     buttonLeft;//only used in hybrid mode
   bool     buttonRight;//only used in hybrid mode
   bool     buttonSelect;//only used in hybrid mode
   
   bool     buttonCalender;//hw button 1
   bool     buttonAddress;//hw button 2
   bool     buttonTodo;//hw button 3
   bool     buttonNotes;//hw button 4
   
   bool     buttonPower;
   bool     buttonContrast;
   
   uint16_t touchscreenX;
   uint16_t touchscreenY;
   bool     touchscreenTouched;
}input_t;

typedef struct{
   //the sdcard is stored as a directory structure with an /(SESSIONID)/sdcard(STATEID).info and /(SESSIONID)/sdcard(CHUNKID).bps
   //each emulator instance has its own sdcard SESSIONID directory if it has an sdcard
   //sdcard(STATEID).info specifys the bps files to used
   //every savestate a new bps file is created with the changes made to the sdcard since the last savestate
   //the first bps is a patch over a buffer of sdcard size filled with 0x00
   //the frontend provides file access but the emulator does all the patching
   uint64_t sessionId;//64 bit system time when emulator starts
   uint64_t stateId;//64 bit system time when the savestate was taken
   uint64_t size;
   uint8_t  type;
   bool     inserted;
}sdcard_t;

typedef struct{
   bool    alarmLed;
   bool    lcdOn;
   bool    backlightOn;
   bool    vibratorOn;
   bool    batteryCharging;
   uint8_t batteryLevel;
   bool    inDock;
}misc_hw_t;

//CPU
#define CRYSTAL_FREQUENCY 32768.0
#define CPU_FREQUENCY (palmCrystalCycles * CRYSTAL_FREQUENCY)

//address space
#define NUM_BANKS(areaSize) (areaSize & 0x0000FFFF ? (areaSize >> 16) + 1 : areaSize >> 16)
#define START_BANK(address) (address >> 16)
#define END_BANK(address, size) (START_BANK(address) + NUM_BANKS(size) - 1)
#define BANK_IN_RANGE(bank, address, size) (bank >= START_BANK(address) && bank <= END_BANK(address, size))
#define TOTAL_MEMORY_BANKS 0x10000

//memory chip addresses
#define RAM_START_ADDRESS 0x00000000
#define ROM_START_ADDRESS 0x10000000
#define REG_START_ADDRESS 0xFFFFF000
#define RAM_SIZE (16 * 0x100000)//16mb RAM
#define ROM_SIZE (4 * 0x100000)//4mb ROM
#define REG_SIZE 0xE00
#define BOOTLOADER_SIZE 0x200

//display chip addresses
#define SED1376_REG_START_ADDRESS 0x1FF80000
#define SED1376_FB_START_ADDRESS  0x1FFA0000
#define SED1376_REG_SIZE 0xB4//it has 0x20000 used address space entrys but only 0xB4 registers
#define SED1376_FB_SIZE  0x20000//0x14000 in size, likely also has 0x20000 used address space entrys, using 0x20000 to prevent speed penalty of checking validity on every access

//config options
#define EMU_FPS 60.0
#define SDCARD_STATE_CHUNKS_VECTOR_SIZE 100

//emulator data
extern uint8_t   palmRam[];
extern uint8_t   palmRom[];
extern uint8_t   palmReg[];
extern uint8_t   palmBootloader[];
extern input_t   palmInput;
extern sdcard_t  palmSdCard;
extern misc_hw_t palmMisc;
extern uint16_t  palmFramebuffer[];
extern double    palmCrystalCycles;
extern double    palmCycleCounter;
extern double    palmClockMultiplier;

//callbacks
extern uint64_t (*emulatorGetSysTime)();
extern uint64_t* (*emulatorGetSdCardStateChunkList)(uint64_t sessionId, uint64_t stateId);//returns the bps chunkIds for a stateId in the order they need to be applied
extern void (*emulatorSetSdCardStateChunkList)(uint64_t sessionId, uint64_t stateId, uint64_t* data);//sets the bps chunkIds for a stateId in the order they need to be applied
extern uint8_t* (*emulatorGetSdCardChunk)(uint64_t sessionId, uint64_t chunkId);
extern void (*emulatorSetSdCardChunk)(uint64_t sessionId, uint64_t chunkId, uint8_t* data, uint64_t size);

//functions
void emulatorInit(uint8_t* palmRomDump, uint8_t* palmBootDump, uint32_t specialFeatures);
void emulatorExit();
void emulatorReset();
void emulatorSetRtc(uint32_t days, uint32_t hours, uint32_t minutes, uint32_t seconds);
uint32_t emulatorSetSdCard(uint64_t size, uint8_t type);
uint32_t emulatorGetStateSize();
void emulatorSaveState(uint8_t* data);
void emulatorLoadState(uint8_t* data);
uint32_t emulatorInstallPrcPdb(uint8_t* data, uint32_t size);
void emulateFrame();
   
#ifdef __cplusplus
}
#endif
