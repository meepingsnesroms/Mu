#pragma once

unsigned int getHwRegister8(unsigned int address);
unsigned int getHwRegister16(unsigned int address);
unsigned int getHwRegister32(unsigned int address);

void setHwRegister8(unsigned int address, unsigned int value);
void setHwRegister16(unsigned int address, unsigned int value);
void setHwRegister32(unsigned int address, unsigned int value);

void rtcAddSecond();

void initHwRegisters();
