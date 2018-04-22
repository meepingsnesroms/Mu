#pragma once

#include <stdint.h>

extern uint8_t  sed1376Registers[];
extern uint8_t  sed1376RLut[];
extern uint8_t  sed1376GLut[];
extern uint8_t  sed1376BLut[];
extern uint8_t  sed1376Framebuffer[];

bool sed1376PowerSaveEnabled();

uint8_t sed1376GetRegister(uint8_t address);
void sed1376SetRegister(uint8_t address, uint8_t value);

void sed1376Reset();
void sed1376RefreshLut();

void sed1376Render();
