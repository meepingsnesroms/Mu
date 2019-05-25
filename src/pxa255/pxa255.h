#ifndef PXA255_H
#define PXA255_H

//this is the main module for the PXA255, with all the files it may be kind of confusing
//this is the only file from this directory that the emu should interface with

#include <stdint.h>
#include <stdbool.h>

uint16_t* pxa255Framebuffer;

bool pxa255Init(uint8_t** returnRom, uint8_t** returnRam);
void pxa255Deinit(void);
void pxa255Reset(void);
void pxa255SetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
uint32_t pxa255StateSize(void);
void pxa255SaveState(uint8_t* data);
void pxa255LoadState(uint8_t* data);

void pxa255Execute(bool wantVideo);//runs the CPU for 1 frame

uint32_t pxa255GetRegister(uint8_t reg);//only for debugging

#endif
