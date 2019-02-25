#ifndef EXPANSION_HARDWARE_H
#define EXPANSION_HARDWARE_H

#include <stdint.h>

void expansionHardwareReset(void);
uint32_t expansionHardwareStateSize(void);
void expansionHardwareSaveState(uint8_t* data);
void expansionHardwareLoadState(uint8_t* data);

void expansionHardwareRenderAudio(void);
void expansionHardwareRenderDisplay(void);

uint32_t expansionHardwareGetRegister(uint32_t address);
void expansionHardwareSetRegister(uint32_t address, uint32_t value);

#endif
