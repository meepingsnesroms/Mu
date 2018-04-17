#pragma once

#include <stdint.h>

//address space
#define NUM_BANKS(areaSize) (areaSize & 0x0000FFFF ? (areaSize >> 16) + 1 : areaSize >> 16)
#define START_BANK(address) (address >> 16)
#define END_BANK(address, size) (START_BANK(address) + NUM_BANKS(size) - 1)
#define BANK_IN_RANGE(bank, address, size) (bank >= START_BANK(address) && bank <= END_BANK(address, size))
#define BANK_ADDRESS(bank) (bank << 16)
#define BANK_READ_ONLY(bank, chip) (chips[chip].readOnly || (chips[chip].readOnlyForProtectedMemory && (BANK_ADDRESS(bank) - chips[chip].start) >= chips[chip].unprotectedSize))
#define TOTAL_MEMORY_BANKS 0x10000

//memory chip addresses
#define REG_START_ADDRESS 0xFFFFF000
#define RAM_SIZE (16 * 0x100000)//16mb RAM
#define ROM_SIZE (4 * 0x100000)//4mb ROM
#define REG_SIZE 0x1000//is actually 0xE00 without bootloader
#define BOOTLOADER_SIZE 0x200

//display chip addresses
#define SED1376_REG_SIZE 0x20000//it has 0x20000 used address space entrys but only 0xB4 registers
#define SED1376_FB_SIZE  0x20000//0x14000 in size, likely also has 0x20000 used address space entrys, using 0x20000 to prevent speed penalty of checking validity on every access

//the read/write stuff looks messy here but makes the memory access functions alot cleaner
#define BUFFER_READ_8(segment, accessAddress, startAddress, mask)  segment[accessAddress - startAddress & mask]
#define BUFFER_READ_16(segment, accessAddress, startAddress, mask) segment[accessAddress - startAddress & mask] << 8 | segment[accessAddress - startAddress + 1  & mask]
#define BUFFER_READ_32(segment, accessAddress, startAddress, mask) segment[accessAddress - startAddress & mask] << 24 | segment[accessAddress - startAddress + 1  & mask] << 16 | segment[accessAddress - startAddress + 2  & mask] << 8 | segment[accessAddress - startAddress + 3  & mask]
#define BUFFER_WRITE_8(segment, accessAddress, startAddress, mask, value)  segment[accessAddress - startAddress & mask] = value
#define BUFFER_WRITE_16(segment, accessAddress, startAddress, mask, value) segment[accessAddress - startAddress & mask] = value >> 8; segment[accessAddress - startAddress + 1  & mask] = value & 0xFF
#define BUFFER_WRITE_32(segment, accessAddress, startAddress, mask, value) segment[accessAddress - startAddress & mask] = value >> 24; segment[accessAddress - startAddress + 1  & mask] = (value >> 16) & 0xFF; segment[accessAddress - startAddress + 2  & mask] = (value >> 8) & 0xFF; segment[accessAddress - startAddress + 3  & mask] = value & 0xFF

//memory banks
enum{
   EMPTY_BANK = 0,
   RAM_BANK,
   ROM_BANK,
   REG_BANK,
   SED1376_REG_BANK,
   SED1376_FB_BANK,
   UNSAFE_BANK//if a chip select uses base address bits 15 or 14 accesses wont be bank aligned and will use "if(address >= chips[chip].start && address <= chips[chip].start + chips[chip].size)"
};

//types
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
