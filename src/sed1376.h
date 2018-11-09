#ifndef SED1376_H
#define SED1376_H

#include <stdint.h>
#include <stdbool.h>

extern uint8_t sed1376Framebuffer[];

void sed1376Reset(void);
uint64_t sed1376StateSize(void);
void sed1376SaveState(uint8_t* data);
void sed1376LoadState(uint8_t* data);

bool sed1376PowerSaveEnabled(void);
uint8_t sed1376GetRegister(uint8_t address);
void sed1376SetRegister(uint8_t address, uint8_t value);

void sed1376Render(void);

#endif
