#ifndef PXA260_H
#define PXA260_H

//this is the main module for the PXA260, with all the files it may be kind of confusing
//this is the only file from this directory that the emu should interface with

#include <stdint.h>
#include <stdbool.h>

#include "pxa260_IC.h"
#include "pxa260_PwrClk.h"
#include "pxa260_GPIO.h"
#include "pxa260_RTC.h"
#include "pxa260_TIMR.h"
#include "pxa260_CPU.h"

extern ArmCpu       pxa260CpuState;
extern uint16_t*    pxa260Framebuffer;
extern Pxa260pwrClk pxa260PwrClk;
extern Pxa260ic     pxa260Ic;
extern Pxa260rtc    pxa260rtc;
extern Pxa260gpio   pxa260Gpio;
extern Pxa260timr   pxa260Timer;

uint8_t read_byte(uint32_t address);
uint16_t read_half(uint32_t address);
uint32_t read_word(uint32_t address);

void write_byte(uint32_t address, uint8_t byte);
void write_half(uint32_t address, uint16_t half);
void write_word(uint32_t address, uint32_t word);

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
