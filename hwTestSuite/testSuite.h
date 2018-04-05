#ifndef TESTSUITE_HEADER
#define TESTSUITE_HEADER

#include <PalmOS.h>
#include <stdint.h>
#include "testSuiteConfig.h"


/*defines*/

#define PalmOS35 sysMakeROMVersion(3,5,0,sysROMStageRelease,0)
#define PalmOS50 sysMakeROMVersion(5,0,0,sysROMStageRelease,0)

#define buttonLeft   keyBitHard1
#define buttonRight  keyBitHard2
#define buttonUp     keyBitPageUp
#define buttonDown   keyBitPageDown
#define buttonBack   keyBitHard3
#define buttonSelect keyBitHard4

#define TYPE_NULL    0x00
#define TYPE_BOOL    0x01/*bool can share with uint*/
#define TYPE_UINT    0x01
#define TYPE_INT     0x02
#define TYPE_FLOAT   0x03
#define TYPE_PTR     0x04
#define LENGTH_0     0x00/*for null*/
#define LENGTH_1     0x10
#define LENGTH_8     0x20
#define LENGTH_16    0x30
#define LENGTH_32    0x40
#define LENGTH_64    0x50
#define LENGTH_PTR   0x60/*first half of 64bit value is length, second is pointer*/
#define LENGTH_ANY   0x70/*for pointers with a user defined end or no end*/
#define LENGTH_STR   0xF0/*for pointers to string*/


/*types*/
typedef uint64_t var_value;/*can hold all other types*/

typedef struct{
   uint8_t   type;
   var_value value;
}var;

typedef var (*activity_t)();

typedef struct{
   char       name[TEST_NAME_LENGTH];
   activity_t testFunction;
}test_t;


/*variables*/
extern uint16_t palmButtons;
extern uint16_t palmButtonsLastFrame;
extern Boolean  unsafeMode;

/*
functions
no inline functions
old gcc versions have broken handling of inline functions where variables with the same name
inside the function and outside can result in the outside variable being written
c89 also doesnt support them
*/
/*hardware buttons*/
Boolean getButton(uint16_t button);
Boolean getButtonLastFrame(uint16_t button);
Boolean getButtonChanged(uint16_t button);
Boolean getButtonPressed(uint16_t button);
Boolean getButtonReleased(uint16_t button);

/*var type operators*/
uint8_t   getVarType(var thisVar);
uint8_t   getVarLength(var thisVar);
uint32_t  getVarDataLength(var thisVar);
void*     getVarPointer(var thisVar);
uint32_t  getVarPointerSize(var thisVar);
var_value getVarValue(var thisVar);
var       makeVar(uint8_t length, uint8_t type, uint64_t value);
Boolean   varsEqual(var var1, var var2);

/*kernel memory access*/
uint8_t  readArbitraryMemory8(uint32_t address);
uint16_t readArbitraryMemory16(uint32_t address);
uint32_t readArbitraryMemory32(uint32_t address);
void     writeArbitraryMemory8(uint32_t address, uint8_t value);
void     writeArbitraryMemory16(uint32_t address, uint16_t value);
void     writeArbitraryMemory32(uint32_t address, uint32_t value);

/*graphics*/
void forceFrameRedraw();

/*execution state*/
void callSubprogram(activity_t activity);
void exitSubprogram();
void execSubprogram(activity_t activity);/*replace current subprogram with a new one*/
var  getSubprogramReturnValue();
var  getSubprogramArgs();
void setSubprogramArgs(var args);
var  subprogramGetData();/*for subprograms to get data they stored*/
void subprogramSetData(var data);/*for subprograms to save data when calling a new subprogram*/

/*special subprograms*/
var memoryAllocationError();

#endif
