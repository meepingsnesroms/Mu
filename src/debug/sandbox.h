#ifndef SANDBOX_H
#define SANDBOX_H

#include <stdint.h>
#include <stdbool.h>

enum{
   SANDBOX_PATCH_OS = 0,
   SANDBOX_DEBUG_INSTALL_APP
};

void sandboxInit(void);
void sandboxReset(void);
uint32_t sandboxStateSize(void);
void sandboxSaveState(uint8_t* data);
void sandboxLoadState(uint8_t* data);
uint32_t sandboxCommand(uint32_t command, void* data);
void sandboxOnOpcodeRun(void);
void sandboxOnMemoryAccess(uint32_t address, uint8_t size, bool write, uint32_t value);
bool sandboxRunning(void);
void sandboxReturn(void);//should only be called called by 68k code
uint16_t sandboxSetWatchRegion(uint32_t address, uint32_t size);
void sandboxClearWatchRegion(uint16_t index);

#endif
