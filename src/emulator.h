#pragma once
//this is the only header a frontend needs to include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

//#include "memoryAccess.h"//for size macros
#include "specs/emuFeatureRegistersSpec.h"

//to enable degguging define EMU_DEBUG, all options below do nothing unless EMU_DEBUG is defined
//to enable sandbox debugging define EMU_SANDBOX
//to enable opcode level debugging define EMU_SANDBOX_OPCODE_LEVEL_DEBUG
//to log all API calls define EMU_SANDBOX_LOG_APIS, EMU_SANDBOX_OPCODE_LEVEL_DEBUG must also be defined for this to work

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
   EMU_ERROR_INVALID_PARAMETER,
   EMU_ERROR_RESOURCE_LOCKED
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
   //CPU -> SD
   uint64_t command;
   uint8_t  commandBitsRemaining;

   //SD -> CPU
   uint8_t  response;
   uint8_t  dataPacket[1 + 2048 + 2];

   buffer_t flashChip;
}sd_card_t;

typedef struct{
   bool    powerButtonLed;
   bool    lcdOn;
   uint8_t backlightLevel;
   bool    vibratorOn;
   bool    batteryCharging;
   uint8_t batteryLevel;
   uint8_t dataPort;
}misc_hw_t;

//config options
#define AUDIO_DUTY_CYCLE_SIZE 20//the amount of frequencys the PWM can play
#define SAVE_STATE_VERSION 0

//system constants
#define CRYSTAL_FREQUENCY 32768
#define EMU_FPS 60
#define AUDIO_SAMPLES (CRYSTAL_FREQUENCY / EMU_FPS * AUDIO_DUTY_CYCLE_SIZE)

//emulator data, some are GUI interface variables, some should be left alone
extern uint8_t*  palmRam;//dont touch
extern uint8_t*  palmRom;//dont touch
extern uint8_t   palmReg[];//dont touch
extern input_t   palmInput;//write allowed
extern sd_card_t palmSdCard;//dont touch
extern misc_hw_t palmMisc;//read/write allowed
extern uint16_t  palmFramebuffer[];//read allowed if FEATURE_320x320 is off, or else invalid data will be displayed
extern uint16_t* palmExtendedFramebuffer;//read allowed if FEATURE_320x320 is on, or else SIGSEGV
extern int16_t   palmAudio[];//read allowed, 2 channel signed 16 bit audio
extern uint32_t  palmAudioSampleIndex;//dont touch
extern uint32_t  palmSpecialFeatures;//read allowed
extern double    palmCrystalCycles;//dont touch
extern double    palmCycleCounter;//dont touch
extern double    palmClockMultiplier;//read/write allowed

//functions
uint32_t emulatorInit(buffer_t palmRomDump, buffer_t palmBootDump, uint32_t specialFeatures);//calling any emulator functions before emulatorInit results in undefined behavior
void emulatorExit();
void emulatorReset();
void emulatorSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
uint64_t emulatorGetStateSize();
bool emulatorSaveState(buffer_t buffer);//true = success
bool emulatorLoadState(buffer_t buffer);//true = success
buffer_t emulatorGetRamBuffer();//this is a direct pointer to RAM, do not free it
buffer_t emulatorGetSdCardBuffer();//this is a direct pointer to the SD card data, do not free it
uint32_t emulatorInsertSdCard(buffer_t image);//use (NULL, desired size) to create a new empty SD card
void emulatorEjectSdCard();
uint32_t emulatorInstallPrcPdb(buffer_t file);
void emulateFrame();
   
#ifdef __cplusplus
}
#endif
