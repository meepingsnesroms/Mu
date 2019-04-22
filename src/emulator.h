#ifndef EMULATOR_H
#define EMULATOR_H
//this is the only header a frontend needs to include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "audio/blip_buf.h"
#include "m515Bus.h"//for size macros
#include "specs/emuFeatureRegisterSpec.h"//for feature names

//DEFINE INFO!!!
//define EMU_SUPPORT_PALM_OS5 to compile in Tungsten C support(not reccomended for low power devices)
//define EMU_MULTITHREADED to speed up long loops
//define EMU_NO_SAFETY to remove all safety checks
//define EMU_BIG_ENDIAN on big endian systems
//to enable degguging define EMU_DEBUG, all options below do nothing unless EMU_DEBUG is defined
//to enable sandbox debugging define EMU_SANDBOX
//to enable memory access logging define EMU_SANDBOX_LOG_MEMORY_ACCESSES
//to enable opcode level debugging define EMU_SANDBOX_OPCODE_LEVEL_DEBUG
//to enable flow control logging define EMU_SANDBOX_LOG_JUMPS, EMU_SANDBOX_OPCODE_LEVEL_DEBUG must also be defined for this to work
//to log all API calls define EMU_SANDBOX_LOG_APIS, EMU_SANDBOX_OPCODE_LEVEL_DEBUG must also be defined for this to work

//debug
#if defined(EMU_DEBUG)
#if defined(EMU_CUSTOM_DEBUG_LOG_HANDLER)
extern uint32_t frontendDebugStringSize;
extern char*    frontendDebugString;
void frontendHandleDebugPrint();
#define debugLog(...) (snprintf(frontendDebugString, frontendDebugStringSize, __VA_ARGS__), frontendHandleDebugPrint())
#else
#define debugLog(...) printf(__VA_ARGS__)
#endif
#else
//msvc2003 doesnt support variadic macros, so just use an empty variadic function instead, EMU_DEBUG is not supported at all on msvc2003
static void debugLog(char* str, ...){};
#endif

//config options
#define EMU_FPS 60
#define DBVZ_SYSCLK_PRECISION 2000000//the amount of cycles to run before adding SYSCLKs, higher = faster, higher values may skip timer events and lower audio accuracy
#define DBVZ_CPU_PERCENT_WAITING 0.30//account for wait states when reading memory, tested with SysInfo.prc
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CLOCK_RATE 235929600//smallest amount of time a second can be split into:(2.0 * (14.0 * (255 + 1.0) + 15 + 1.0)) * 32768 == 235929600, used to convert the variable timing of SYSCLK and CLK32 to a fixed location in the current frame 0<->AUDIO_END_OF_FRAME
#define AUDIO_SPEAKER_RANGE 0x6000//prevent hitting the top or bottom of the speaker when switching direction rapidly
#define SD_CARD_BLOCK_SIZE 512//all newer SDSC cards have this fixed at 512
#define SD_CARD_BLOCK_DATA_PACKET_SIZE (1 + SD_CARD_BLOCK_SIZE + 2)
#define SD_CARD_RESPONSE_FIFO_SIZE (SD_CARD_BLOCK_DATA_PACKET_SIZE * 3)
#define SD_CARD_NCR_BYTES 1//how many 0xFF bytes come before the R1 response
#define SAVE_STATE_VERSION 0

//system constants
#define M515_CRYSTAL_FREQUENCY 32768
#define AUDIO_SAMPLES_PER_FRAME (AUDIO_SAMPLE_RATE / EMU_FPS)
#define AUDIO_END_OF_FRAME (AUDIO_CLOCK_RATE / EMU_FPS)

//emu errors
enum{
   EMU_ERROR_NONE = 0,
   EMU_ERROR_UNKNOWN,
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
   uint32_t size;
}buffer_t;

typedef struct{
   bool  buttonUp;
   bool  buttonDown;
   
   bool  buttonCalendar;//hw button 1
   bool  buttonAddress;//hw button 2
   bool  buttonTodo;//hw button 3
   bool  buttonNotes;//hw button 4
   
   bool  buttonPower;
   
   float touchscreenX;//0.0 = left, 1.0 = right
   float touchscreenY;//0.0 = top, 1.0 = bottom
   bool  touchscreenTouched;
}input_t;

typedef struct{
   uint64_t command;
   uint8_t  commandBitsRemaining;
   uint8_t  runningCommand;
   uint32_t runningCommandVars[3];
   uint8_t  runningCommandPacket[SD_CARD_BLOCK_DATA_PACKET_SIZE];
   uint8_t  responseFifo[SD_CARD_RESPONSE_FIFO_SIZE];
   uint16_t responseReadPosition;
   int8_t   responseReadPositionBit;
   uint16_t responseWritePosition;
   bool     commandIsAcmd;
   bool     allowInvalidCrc;
   bool     chipSelect;
   bool     receivingCommand;
   bool     inIdleState;
   bool     writeProtectSwitch;
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

typedef struct{
   uint32_t info;
   uint32_t src;
   uint32_t dst;
   uint32_t size;
   uint32_t value;
   //uint32_t cmd;//one time use, has no variable
}emu_reg_t;

//emulator data, some are GUI interface variables, some should be left alone
extern uint8_t*  palmRom;//dont touch
extern uint8_t*  palmRam;//access allowed to read save RAM without allocating a giant buffer, but endianness must be taken into account
extern uint8_t*  palmReg;//dont touch
extern input_t   palmInput;//write allowed
extern sd_card_t palmSdCard;//dont touch
extern misc_hw_t palmMisc;//read/write allowed
extern emu_reg_t palmEmuFeatures;//dont touch
extern uint16_t* palmFramebuffer;//read allowed
extern uint16_t  palmFramebufferWidth;//read allowed
extern uint16_t  palmFramebufferHeight;//read allowed
extern int16_t*  palmAudio;//read allowed, 2 channel signed 16 bit audio
extern blip_t*   palmAudioResampler;//dont touch
extern double    palmCycleCounter;//dont touch
extern double    palmClockMultiplier;//dont touch

//functions
uint32_t emulatorInit(buffer_t palmRomDump, buffer_t palmBootDump, uint32_t enabledEmuFeatures);//calling any emulator functions before emulatorInit results in undefined behavior
void emulatorExit(void);
void emulatorHardReset(void);
void emulatorSoftReset(void);
void emulatorSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
uint32_t emulatorGetStateSize(void);
bool emulatorSaveState(buffer_t buffer);//true = success
bool emulatorLoadState(buffer_t buffer);//true = success
uint32_t emulatorGetRamSize(void);
bool emulatorSaveRam(buffer_t buffer);//true = success
bool emulatorLoadRam(buffer_t buffer);//true = success
buffer_t emulatorGetSdCardBuffer(void);//this is a direct pointer to the SD card data, do not free it
uint32_t emulatorInsertSdCard(buffer_t image, bool writeProtectSwitch);//use (NULL, desired size) to create a new empty SD card
void emulatorEjectSdCard(void);
void emulatorRunFrame(void);
   
#ifdef __cplusplus
}
#endif

#endif
