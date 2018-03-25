#pragma once

#include <stdint.h>

#include <boolean.h>

//memory accessors
unsigned int getHwRegister8(unsigned int address);
unsigned int getHwRegister16(unsigned int address);
unsigned int getHwRegister32(unsigned int address);
void setHwRegister8(unsigned int address, unsigned int value);
void setHwRegister16(unsigned int address, unsigned int value);
void setHwRegister32(unsigned int address, unsigned int value);

//timing
void rtcAddSecond();
void toggleClk32();
bool cpuIsOn();
void updateInterrupts();


//config
void resetHwRegisters();
void setRtc(uint32_t days, uint32_t hours, uint32_t minutes, uint32_t seconds);
