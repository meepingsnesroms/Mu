#ifndef SANDBOX_H
#define SANDBOX_H

#include <stdint.h>
#include <stdbool.h>

enum{
   SANDBOX_TEST_OS_VER = 0,
   SANDBOX_SEND_OS_TOUCH,
   SANDBOX_PATCH_OS,
   SANDBOX_INSTALL_APP
};

void sandboxInit(void);
uint32_t sandboxCommand(uint32_t test, void* data);
void sandboxOnOpcodeRun(void);
bool sandboxRunning(void);
void sandboxReturn(void);//should only be called called by 68k code

#endif
