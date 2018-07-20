#pragma once

#include <stdint.h>
#include <stdbool.h>

enum{
   SANDBOX_TEST_OS_VER = 0,
   SANDBOX_TEST_TOUCH_READ
};

typedef struct{
   void* hostPointer;
   uint32_t emuPointer;
   uint64_t bytes;//may be used for strings or structs in the future
}return_pointer_t;

void sandboxInit();
void sandboxTest(uint32_t test);
void sandboxOnOpcodeRun();
bool sandboxRunning();
void sandboxReturn();//should only be called called by 68k code
