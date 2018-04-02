#ifndef TESTSUITE_HEADER
#define TESTSUITE_HEADER

#include <PalmOS.h>
#include <stdbool.h>
#include <stdint.h>


//defines
#define buttonLeft   keyBitHard1
#define buttonRight  keyBitHard2
#define buttonUp     keyBitPageUp
#define buttonDown   keyBitPageDown
#define buttonBack   keyBitHard3
#define buttonSelect keyBitHard4

#define TYPE_NULL    0x00
#define TYPE_BOOL    0x01//bool can share with uint
#define TYPE_UINT    0x01
#define TYPE_INT     0x02
#define TYPE_FLOAT   0x03
#define TYPE_POINTER 0x04
#define LENGTH_0     0x00//for null
#define LENGTH_1     0x10
#define LENGTH_8     0x20
#define LENGTH_16    0x30
#define LENGTH_32    0x40
#define LENGTH_64    0x50
#define LENGTH_PTR   0x60//first half of 64bit value is length, second is pointer
#define LENGTH_STR   0xF0//for pointers to string


//types
typedef uint64_t var_value;//can hold all other types

typedef struct{
   uint8_t   type;
   var_value value;
}var;

typedef var (*activity_t)();

typedef struct{
   bool       isSimpleTest;//if simple run once, if complex take control over user input and runloop
   var        expectedResult;
   activity_t callTest;
}test_t;


//variables
extern uint16_t palmButtons;
extern uint16_t palmButtonsLastFrame;


//functions
//no inline functions
//old gcc versions have broken handling of inline functions where variables with the same name
//inside the function and outside can result in the outside variable being written
#define getButton(button)          ((palmButtons & button) != 0)
#define getButtonLastFrame(button) ((palmButtonsLastFrame & button) != 0)
#define getButtonChanged(button)   (getButton(button) != getButtonLastFrame(button))
#define getButtonPressed(button)   (getButton(button) && !getButtonLastFrame(button));
#define getButtonReleased(button)  (!getButton(button) && getButtonLastFrame(button))

#define getVarType(thisVar)          (thisVar.type & 0x0F)
#define getVarLength(thisVar)        (thisVar.type >> 4)
#define getVarDataLength(thisVar)    (thisVar.value >> 32)
#define getVarPointer(thisVar)       (thisVar.value & 0xFFFFFFFF);
#define getVarValue(thisVar)         (thisVar.value);
#define makeVar(length, type, value) {(uint8_t)((length & 0xF0) | (type & 0x0F)), (uint64_t)value}

void callSubprogram(activity_t activity);
void exitSubprogram();
void execSubprogram(activity_t activity);//replace current subprogram with a new one
var  getSubprogramArgs();
void setSubprogramArgs(var args);
var  subprogramGetData();//for subprograms to get data they stored
void subprogramSetData(var data);//for subprograms to save data when calling a new subprogram

var memoryAllocationError();

#endif
