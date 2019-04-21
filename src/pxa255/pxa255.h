#ifndef PXA255_H
#define PXA255_H

//this is the main module for the PXA255, with all the files it may be kind of confusing
//this is the only file from this directory that the emu should interface with

#include <stdint.h>

void pxa255Init(void);
void pxa255Reset(void);
uint32_t pxa255StateSize(void);
void pxa255SaveState(uint8_t* data);
void pxa255LoadState(uint8_t* data);

void pxa255Execute(void);//runs the CPU for 1 frame

#endif
