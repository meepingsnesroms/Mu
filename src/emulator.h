#pragma once
//this is the only header a frontend needs to include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "memoryAccess.h"//for size macros
#include "emuFeatureRegistersSpec.h"

//to enable degguging define EMU_DEBUG, all options below do nothing unless EMU_DEBUG is defined
//to enable opcode level debugging define EMU_OPCODE_LEVEL_DEBUG
//to log unknown register reads and writes define EMU_LOG_REGISTER_ACCESS_UNKNOWN
//to log all register reads and writes define EMU_LOG_REGISTER_ACCESS_ALL
//to log all api calls define EMU_LOG_APIS

//debug
#if defined(EMU_DEBUG)
#define debugLog(...) printf(__VA_ARGS__)
#else
#define debugLog(...)
#endif

//emu errors
enum{
   EMU_ERROR_NONE = 0,
   EMU_ERROR_NOT_IMPLEMENTED,
   EMU_ERROR_CALLBACKS_NOT_SET,
   EMU_ERROR_OUT_OF_MEMORY,
   EMU_ERROR_INVALID_PARAMETER
};

//sdcard types
enum{
   CARD_BEGIN = 0,
   CARD_SD = 0,
   CARD_MMC,
   CARD_NONE,
   CARD_END
};

//types
typedef struct{
   uint8_t* data;
   uint64_t size;
}buffer_t;

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

//config options
#define EMU_FPS 60.0
#define SDCARD_STATE_CHUNKS_VECTOR_SIZE 100
#define SAVE_STATE_VERSION 1

//emulator data
extern uint8_t*  palmRam;
extern uint8_t*  palmRom;
extern uint8_t   palmReg[];
extern input_t   palmInput;
extern sdcard_t  palmSdCard;
extern misc_hw_t palmMisc;
extern uint16_t  palmFramebuffer[];
extern uint16_t* palmExtendedFramebuffer;
extern uint32_t  palmSpecialFeatures;
extern double    palmCrystalCycles;
extern double    palmCycleCounter;
extern double    palmClockMultiplier;

//callbacks
extern uint64_t (*emulatorGetSysTime)();
extern uint64_t* (*emulatorGetSdCardStateChunkList)(uint64_t sessionId, uint64_t stateId);//returns the bps chunkIds for a stateId in the order they need to be applied
extern void (*emulatorSetSdCardStateChunkList)(uint64_t sessionId, uint64_t stateId, uint64_t* data);//sets the bps chunkIds for a stateId in the order they need to be applied
extern buffer_t (*emulatorGetSdCardChunk)(uint64_t sessionId, uint64_t chunkId);
extern void (*emulatorSetSdCardChunk)(uint64_t sessionId, uint64_t chunkId, buffer_t chunk);

//functions
uint32_t emulatorInit(buffer_t palmRomDump, buffer_t palmBootDump, uint32_t specialFeatures);//calling any emulator functions before emulatorInit results in undefined behavior
void emulatorExit();
void emulatorReset();
void emulatorSetRtc(uint32_t days, uint32_t hours, uint32_t minutes, uint32_t seconds);
uint32_t emulatorSetNewSdCard(uint64_t size, uint8_t type);
buffer_t emulatorGetSdCardImage();//doing anything with the emulator will alter this, do not free it, it is a direct pointer to the sdcard data
uint32_t emulatorSetSdCardFromImage(buffer_t image, uint8_t type);
uint64_t emulatorGetStateSize();
void emulatorSaveState(uint8_t* data);
void emulatorLoadState(uint8_t* data);
uint32_t emulatorInstallPrcPdb(buffer_t file);
void emulateFrame();
bool emulateUntilDebugEventOrFrameEnd();//false for frame end, true for debug event
   
#ifdef __cplusplus
}
#endif
