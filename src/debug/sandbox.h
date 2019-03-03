#ifndef SANDBOX_H
#define SANDBOX_H

#include <stdint.h>
#include <stdbool.h>

enum{
   SANDBOX_PATCH_OS = 0,
   SANDBOX_INSTALL_APP
};

void sandboxInit(void);
uint32_t sandboxCommand(uint32_t command, void* data);
void sandboxOnOpcodeRun(void);
void sandboxOnMemoryAccess(uint32_t address, uint8_t size, bool write, uint32_t value);
bool sandboxRunning(void);
void sandboxReturn(void);//should only be called called by 68k code

#endif
