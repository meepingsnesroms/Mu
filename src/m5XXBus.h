#ifndef M5XX_BUS_H
#define M5XX_BUS_H

#include <stdint.h>
#include <stdbool.h>

//address space
//new bank size (0x4000)
#define DBVZ_BANK_SCOOT 14
#define DBVZ_NUM_BANKS(areaSize) (((areaSize) >> DBVZ_BANK_SCOOT) + ((areaSize) & ((1 << DBVZ_BANK_SCOOT) - 1) ? 1 : 0))
#define DBVZ_START_BANK(address) ((address) >> DBVZ_BANK_SCOOT)
#define DBVZ_END_BANK(address, size) (DBVZ_START_BANK(address) + DBVZ_NUM_BANKS(size) - 1)
#define DBVZ_BANK_IN_RANGE(bank, address, size) ((bank) >= DBVZ_START_BANK(address) && (bank) <= DBVZ_END_BANK(address, size))
#define DBVZ_BANK_ADDRESS(bank) ((bank) << DBVZ_BANK_SCOOT)
#define DBVZ_TOTAL_MEMORY_BANKS (1 << (32 - DBVZ_BANK_SCOOT))//0x40000 banks for *_BANK_SCOOT = 14

//chip addresses and sizes
//after boot RAM is at 0x00000000,
//ROM is at 0x10000000
//and the SED1376 is at 0x1FF80000(+ 0x20000 for framebuffer)
#define DBVZ_EMUCS_START_ADDRESS 0xFFFC0000
#define DBVZ_REG_START_ADDRESS 0xFFFFF000
#define M5XX_ROM_SIZE (4 * 0x100000)//4mb ROM
#define M500_RAM_SIZE (8 * 0x100000)//16mb RAM
#define M515_RAM_SIZE (16 * 0x100000)//16mb RAM
#define DBVZ_EMUCS_SIZE 0x20000
#define DBVZ_REG_SIZE 0x1000//is actually 0xE00 without bootloader
#define DBVZ_BOOTLOADER_SIZE 0x200
#define SED1376_MR_BIT 0x20000

//buffers
//the read/write stuff looks messy here but makes the memory access functions alot cleaner
#if defined(EMU_BIG_ENDIAN)
//memory layout is the same as the Palm m515, just cast to pointer and access, 32 bit accesses are split to prevent unaligned access issues
#define M68K_BUFFER_READ_8(segment, accessAddress, mask)  (*(uint8_t*)(segment + ((accessAddress) & (mask))))
#define M68K_BUFFER_READ_16(segment, accessAddress, mask) (*(uint16_t*)(segment + ((accessAddress) & (mask))))
#define M68K_BUFFER_READ_32(segment, accessAddress, mask) (*(uint16_t*)(segment + ((accessAddress) & (mask))) << 16 | *(uint16_t*)(segment + ((accessAddress) + 2 & (mask))))
#define M68K_BUFFER_WRITE_8(segment, accessAddress, mask, value)  (*(uint8_t*)(segment + ((accessAddress) & (mask))) = (value))
#define M68K_BUFFER_WRITE_16(segment, accessAddress, mask, value) (*(uint16_t*)(segment + ((accessAddress) & (mask))) = (value))
#define M68K_BUFFER_WRITE_32(segment, accessAddress, mask, value) (*(uint16_t*)(segment + ((accessAddress) & (mask))) = (value) >> 16 , *(uint16_t*)(segment + ((accessAddress) + 2 & (mask))) = (value) & 0xFFFF)
#define M68K_BUFFER_READ_8_BIG_ENDIAN  M68K_BUFFER_READ_8
#define M68K_BUFFER_READ_16_BIG_ENDIAN M68K_BUFFER_READ_16
#define M68K_BUFFER_READ_32_BIG_ENDIAN M68K_BUFFER_READ_32
#define M68K_BUFFER_WRITE_8_BIG_ENDIAN  M68K_BUFFER_WRITE_8
#define M68K_BUFFER_WRITE_16_BIG_ENDIAN M68K_BUFFER_WRITE_16
#define M68K_BUFFER_WRITE_32_BIG_ENDIAN M68K_BUFFER_WRITE_32
#else
//memory layout is different from the Palm m515, optimize for opcode fetches(16 bit reads)
#define M68K_BUFFER_READ_8(segment, accessAddress, mask)  (*(uint8_t*)(segment + ((accessAddress) & (mask) ^ 1)))
#define M68K_BUFFER_READ_16(segment, accessAddress, mask) (*(uint16_t*)(segment + ((accessAddress) & (mask))))
#define M68K_BUFFER_READ_32(segment, accessAddress, mask) (*(uint16_t*)(segment + ((accessAddress) & (mask))) << 16 | *(uint16_t*)(segment + ((accessAddress) + 2 & (mask))))
#define M68K_BUFFER_WRITE_8(segment, accessAddress, mask, value)  (*(uint8_t*)(segment + ((accessAddress) & (mask) ^ 1)) = (value))
#define M68K_BUFFER_WRITE_16(segment, accessAddress, mask, value) (*(uint16_t*)(segment + ((accessAddress) & (mask))) = (value))
#define M68K_BUFFER_WRITE_32(segment, accessAddress, mask, value) (*(uint16_t*)(segment + ((accessAddress) & (mask))) = (value) >> 16 , *(uint16_t*)(segment + ((accessAddress) + 2 & (mask))) = (value) & 0xFFFF)
#define M68K_BUFFER_READ_8_BIG_ENDIAN(segment, accessAddress, mask)  (segment[(accessAddress) & (mask)])
#define M68K_BUFFER_READ_16_BIG_ENDIAN(segment, accessAddress, mask) (segment[(accessAddress) & (mask)] << 8 | segment[(accessAddress) + 1 & (mask)])
#define M68K_BUFFER_READ_32_BIG_ENDIAN(segment, accessAddress, mask) (segment[(accessAddress) & (mask)] << 24 | segment[(accessAddress) + 1 & (mask)] << 16 | segment[(accessAddress) + 2 & (mask)] << 8 | segment[(accessAddress) + 3 & (mask)])
#define M68K_BUFFER_WRITE_8_BIG_ENDIAN(segment, accessAddress, mask, value)  (segment[(accessAddress) & (mask)] = (value))
#define M68K_BUFFER_WRITE_16_BIG_ENDIAN(segment, accessAddress, mask, value) (segment[(accessAddress) & (mask)] = (value) >> 8, segment[(accessAddress) + 1 & (mask)] = (value) & 0xFF)
#define M68K_BUFFER_WRITE_32_BIG_ENDIAN(segment, accessAddress, mask, value) (segment[(accessAddress) & (mask)] = (value) >> 24, segment[(accessAddress) + 1 & (mask)] = ((value) >> 16) & 0xFF, segment[(accessAddress) + 2 & (mask)] = ((value) >> 8) & 0xFF, segment[(accessAddress) + 3 & (mask)] = (value) & 0xFF)
#endif

extern uint8_t dbvzBankType[];

void dbvzSetRegisterXXFFAccessMode(void);
void dbvzSetRegisterFFFFAccessMode(void);
void m515SetSed1376Attached(bool attached);
void dbvzResetAddressSpace(void);

#endif
