#pragma once

#include <stdint.h>
#include <stdbool.h>

extern uint8_t sed1376Framebuffer[];

void sed1376Reset();
uint64_t sed1376StateSize();
void sed1376SaveState();
void sed1376LoadState();

bool sed1376PowerSaveEnabled();
uint8_t sed1376GetRegister(uint8_t address);
void sed1376SetRegister(uint8_t address, uint8_t value);

void sed1376Render();
