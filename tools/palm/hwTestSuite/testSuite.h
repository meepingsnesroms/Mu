#ifndef TEST_SUITE_HEADER
#define TEST_SUITE_HEADER

#include <PalmOS.h>
#include <stdint.h>

#include "testSuiteConfig.h"

/*defines*/
#define CODE_SECTION(codeSection) __attribute__((section(codeSection)))
#define ALIGN(size) __attribute__((aligned(size)))

#define SHARED_DATA_BUFFER_SIZE 1000

#define PalmOS35 sysMakeROMVersion(3, 5, 0, sysROMStageRelease, 0)
#define PalmOS50 sysMakeROMVersion(5, 0, 0, sysROMStageRelease, 0)

#define buttonLeft   keyBitHard1
#define buttonRight  keyBitHard2
#define buttonUp     keyBitPageUp
#define buttonDown   keyBitPageDown
#define buttonBack   keyBitHard3
#define buttonSelect keyBitHard4

#define TYPE_NULL  0x00
#define TYPE_BOOL  0x01
#define TYPE_UINT  0x02
#define TYPE_INT   0x03
#define TYPE_FLOAT 0x04
#define TYPE_PTR   0x05
#define LENGTH_0   0x00/*for null*/
#define LENGTH_1   0x10
#define LENGTH_8   0x20
#define LENGTH_16  0x30
#define LENGTH_32  0x40
#define LENGTH_64  0x50
#define LENGTH_PTR 0x60/*first half of 64bit value is length, second is pointer*/
#define LENGTH_ANY 0x70/*for pointers with a user defined end or no end*/
#define LENGTH_STR 0xF0/*for pointers to string*/

/*types*/
typedef struct{
   uint8_t  type;
   uint64_t value;
}var;

typedef var (*activity_t)();

typedef struct{
   char       name[TEST_NAME_LENGTH];
   activity_t testFunction;
}test_t;

/*variables*/
extern uint16_t palmButtons;
extern uint16_t palmButtonsLastFrame;
extern Boolean  isM515;
extern Boolean  haveKsyms;
extern Boolean  skipFrameDelay;
extern uint8_t* sharedDataBuffer;

/*
functions
no inline functions
old gcc versions have broken handling of inline functions where variables with the same name
inside the function and outside can result in the outside variable being written
c89 also doesnt support them
*/
/*hardware buttons*/
#define getButton(button) ((palmButtons & button) != 0)
#define getButtonLastFrame(button) ((palmButtonsLastFrame & button) != 0)
#define getButtonChanged(button) ((palmButtons & button) != (palmButtonsLastFrame & button))
#define getButtonPressed(button) ((palmButtons & button) && !(palmButtonsLastFrame & button))
#define getButtonReleased(button) (!(palmButtons & button) && (palmButtonsLastFrame & button))

/*var type operators*/
#define getVarType(x) (x.type & 0x0F)
#define getVarValue(x) (x.value)
#define getVarLength(x) (x.type & 0xF0)
#define getVarPointer(x) ((void*)(uint32_t)(x.value & 0xFFFFFFFF))
#define getVarDataLength(x) ((uint32_t)(x.value >> 32))
#define varsEqual(x, y) (x.type == y.type && x.value == y.value)
var     makeVar(uint8_t length, uint8_t type, uint64_t value);

/*kernel memory access*/
#define readArbitraryMemory8(address) (*((volatile uint8_t*)(address)))
#define readArbitraryMemory16(address) (*((volatile uint16_t*)(address)))
#define readArbitraryMemory32(address) (*((volatile uint32_t*)(address)))
#define writeArbitraryMemory8(address, value) (*((volatile uint8_t*)(address)) = (value))
#define writeArbitraryMemory16(address, value) (*((volatile uint16_t*)(address)) = (value))
#define writeArbitraryMemory32(address, value) (*((volatile uint32_t*)(address)) = (value))

/*graphics*/
void forceFrameRedraw();

/*Palm OS library patches*/
char* floatToString(float data);

/*execution state*/
void callSubprogram(activity_t activity);
void exitSubprogram();
void execSubprogram(activity_t activity);/*replace current subprogram with a new one*/
var getSubprogramReturnValue();
var getSubprogramArgs();
void setSubprogramArgs(var args);
var subprogramGetData();/*for subprograms to get data they stored*/
void subprogramSetData(var data);/*for subprograms to save data when calling a new subprogram*/

/*special subprograms*/
var memoryAllocationError();

#endif
