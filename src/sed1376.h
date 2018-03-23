#pragma once

#include <stdint.h>

extern uint8_t sed1376Framebuffer[];

void sed1376SetRegister(unsigned int address, unsigned int value);
unsigned int sed1376GetRegister(unsigned int address);
