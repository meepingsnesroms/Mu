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
#include "specs/emuFeatureRegistersSpec.h"

//to enable degguging define EMU_DEBUG, all options below do nothing unless EMU_DEBUG is defined
//to enable sandbox debugging define EMU_SANDBOX
//to enable opcode level debugging define EMU_SANDBOX_OPCODE_LEVEL_DEBUG
//to log all api calls define EMU_SANDBOX_LOG_APIS, EMU_SANDBOX_OPCODE_LEVEL_DEBUG must also be defined for this to work

//debug
#if defined(EMU_DEBUG)
#if defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
extern uint32_t frontendDebugStringSize;
extern char*    frontendDebugString;
void frontendHandleDebugPrint();
void frontendHandleDebugClearLogs();
#define debugClearLogs() frontendHandleDebugClearLogs()
#define debugLog(...) (snprintf(frontendDebugString, frontendDebugStringSize, __VA_ARGS__), frontendHandleDebugPrint())
#else
#define debugClearLogs()
#define debugLog(...) printf(__VA_ARGS__)
#endif
#else
#define debugLog(...)
#endif

//threads
#if defined(EMU_MULTITHREADED)
#define MULTITHREAD_LOOP _Pragma("omp parallel for")
#define MULTITHREAD_DOUBLE_LOOP _Pragma("omp parallel for collapse(2)")
#else
#define MULTITHREAD_LOOP
#define MULTITHREAD_DOUBLE_LOOP
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
   CARD_NONE = 0,
   CARD_SD,
   CARD_MMC,
   CARD_END
};

//port types
enum{
   PORT_NONE = 0,
   PORT_USB_CRADLE,
   PORT_SERIAL_CRADLE,
   PORT_USB_PERIPHERAL,
   PORT_SERIAL_PERIPHERAL,
   PORT_END
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
   
   bool     buttonCalendar;//hw button 1
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
   //sdcard(STATEID).info specifys the bps files to use
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
   bool    powerButtonLed;
   bool    lcdOn;
   uint8_t backlightLevel;
   bool    vibratorOn;
   bool    batteryCharging;
   uint8_t batteryLevel;
   uint8_t dataPort;
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
void emulatorSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
uint32_t emulatorSetNewSdCard(uint64_t size, uint8_t type);
buffer_t emulatorGetSdCardImage();//this is a direct pointer to the sdcard data, do not free it
uint32_t emulatorSetSdCardFromImage(buffer_t image, uint8_t type);
uint64_t emulatorGetStateSize();
bool emulatorSaveState(buffer_t buffer);//true = success
bool emulatorLoadState(buffer_t buffer);//true = success
buffer_t emulatorGetRamBuffer();//this is a direct pointer to RAM, do not free it
uint32_t emulatorInstallPrcPdb(buffer_t file);
void emulateFrame();
   
#ifdef __cplusplus
}
#endif
