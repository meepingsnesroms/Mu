#pragma once

#include <stdint.h>

//address space
//new bank size (0x4000)
#define BANK_SCOOT 14
#define NUM_BANKS(areaSize) ((areaSize) & 0x00003FFF ? ((areaSize) >> BANK_SCOOT) + 1 : (areaSize) >> BANK_SCOOT)
#define START_BANK(address) ((address) >> BANK_SCOOT)
#define END_BANK(address, size) (START_BANK(address) + NUM_BANKS(size) - 1)
#define BANK_IN_RANGE(bank, address, size) ((bank) >= START_BANK(address) && (bank) <= END_BANK(address, size))
#define BANK_ADDRESS(bank) ((bank) << BANK_SCOOT)
#define TOTAL_MEMORY_BANKS (1 << (32 - BANK_SCOOT))//0x40000 banks for BANK_SCOOT = 14

//chip addresses and sizes
#define REG_START_ADDRESS 0xFFFFF000
#define SUPERMASSIVE_RAM_SIZE (128 * 0x100000)//128mb RAM
#define RAM_SIZE (16 * 0x100000)//16mb RAM
#define ROM_SIZE (4 * 0x100000)//4mb ROM
#define REG_SIZE 0x1000//is actually 0xE00 without bootloader
#define BOOTLOADER_SIZE 0x200
#define SED1376_REG_SIZE 0x20000//it has 0x20000 used address space entrys but only 0xB4 registers
#define SED1376_FB_SIZE  0x14000//0x14000 in size

//the read/write stuff looks messy here but makes the memory access functions alot cleaner
#define BUFFER_READ_8(segment, accessAddress, startAddress, mask)  segment[accessAddress - startAddress & mask]
#define BUFFER_READ_16(segment, accessAddress, startAddress, mask) segment[accessAddress - startAddress & mask] << 8 | segment[accessAddress - startAddress + 1  & mask]
#define BUFFER_READ_32(segment, accessAddress, startAddress, mask) segment[accessAddress - startAddress & mask] << 24 | segment[accessAddress - startAddress + 1  & mask] << 16 | segment[accessAddress - startAddress + 2  & mask] << 8 | segment[accessAddress - startAddress + 3  & mask]
#define BUFFER_WRITE_8(segment, accessAddress, startAddress, mask, value)  segment[accessAddress - startAddress & mask] = value
#define BUFFER_WRITE_16(segment, accessAddress, startAddress, mask, value) segment[accessAddress - startAddress & mask] = value >> 8; segment[accessAddress - startAddress + 1  & mask] = value & 0xFF
#define BUFFER_WRITE_32(segment, accessAddress, startAddress, mask, value) segment[accessAddress - startAddress & mask] = value >> 24; segment[accessAddress - startAddress + 1  & mask] = (value >> 16) & 0xFF; segment[accessAddress - startAddress + 2  & mask] = (value >> 8) & 0xFF; segment[accessAddress - startAddress + 3  & mask] = value & 0xFF

extern uint8_t bankType[];

void setRegisterXXFFAccessMode();
void setRegisterFFFFAccessMode();
void setSed1376Attached(bool attached);
void resetAddressSpace();
