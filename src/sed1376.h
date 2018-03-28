#pragma once

#include <stdint.h>

extern uint8_t sed1376Registers[];
extern uint8_t sed1376Framebuffer[];

unsigned int sed1376GetRegister(unsigned int address);
void sed1376SetRegister(unsigned int address, unsigned int value);

void sed1376Reset();

void sed1376Render();
