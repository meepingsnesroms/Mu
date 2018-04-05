#pragma once

#include <stdint.h>

//the read/write stuff looks messy here but makes the memory access functions alot cleaner
#define BUFFER_READ_8(segment, accessAddress, startAddress)  segment[accessAddress - startAddress]
#define BUFFER_READ_16(segment, accessAddress, startAddress) segment[accessAddress - startAddress] << 8 | segment[accessAddress - startAddress + 1]
#define BUFFER_READ_32(segment, accessAddress, startAddress) segment[accessAddress - startAddress] << 24 | segment[accessAddress - startAddress + 1] << 16 | segment[accessAddress - startAddress + 2] << 8 | segment[accessAddress - startAddress + 3]
#define BUFFER_WRITE_8(segment, accessAddress, startAddress, value)  segment[accessAddress - startAddress] = value
#define BUFFER_WRITE_16(segment, accessAddress, startAddress, value) segment[accessAddress - startAddress] = value >> 8; segment[accessAddress - startAddress + 1] = value & 0xFF
#define BUFFER_WRITE_32(segment, accessAddress, startAddress, value) segment[accessAddress - startAddress] = value >> 24; segment[accessAddress - startAddress + 1] = (value >> 16) & 0xFF; segment[accessAddress - startAddress + 2] = (value >> 8) & 0xFF; segment[accessAddress - startAddress + 3] = value & 0xFF

typedef struct{
   unsigned int (*read8)(unsigned int address);
   unsigned int (*read16)(unsigned int address);
   unsigned int (*read32)(unsigned int address);
   
   void         (*write8)(unsigned int address, unsigned int value);
   void         (*write16)(unsigned int address, unsigned int value);
   void         (*write32)(unsigned int address, unsigned int value);
}memory_access_t;

extern uint8_t bankType[];

void setRegisterXXFFAccessMode();
void setRegisterFFFFAccessMode();
void setSed1376Attached(bool attached);
void refreshBankHandlers();//must call after loadstate or you will SIGSEGV
void resetAddressSpace();
