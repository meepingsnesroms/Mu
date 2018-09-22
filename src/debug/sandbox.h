#pragma once

#include <stdint.h>
#include <stdbool.h>

enum{
   SANDBOX_TEST_OS_VER = 0,
   SANDBOX_SEND_OS_TOUCH,
   SANDBOX_PATCH_OS,
   SANDBOX_INSTALL_APP
};

void sandboxInit();
uint32_t sandboxCommand(uint32_t test, void* data);
void sandboxOnOpcodeRun();
bool sandboxRunning();
void sandboxReturn();//should only be called called by 68k code
