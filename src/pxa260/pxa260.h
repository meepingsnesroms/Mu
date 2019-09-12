#ifndef PXA260_H
#define PXA260_H

//this is the main module for the PXA260, with all the files it may be kind of confusing
//this is the only file from this directory that the emu should interface with

#include <stdint.h>
#include <stdbool.h>

#include "pxa260_IC.h"
#include "pxa260_PwrClk.h"
#include "pxa260_GPIO.h"

extern uint16_t*    pxa260Framebuffer;
extern Pxa260pwrClk pxa260PwrClk;
extern Pxa260ic     pxa260Ic;
extern Pxa260gpio   pxa260Gpio;

bool pxa260Init(uint8_t** returnRom, uint8_t** returnRam);
void pxa260Deinit(void);
void pxa260Reset(void);
void pxa260SetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
uint32_t pxa260StateSize(void);
void pxa260SaveState(uint8_t* data);
void pxa260LoadState(uint8_t* data);

void pxa260Execute(bool wantVideo);//runs the CPU for 1 frame

uint32_t pxa260GetRegister(uint8_t reg);//only for debugging
#define pxa260GetPc() pxa260GetRegister(15)//only for debugging
uint32_t pxa260GetCpsr(void);//only for debugging
uint32_t pxa260GetSpsr(void);//only for debugging
uint64_t pxa260ReadArbitraryMemory(uint32_t address, uint8_t size);//only for debugging, uses physical address not MMU mapped

#endif
