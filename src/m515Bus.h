#ifndef M515_BUS_H
#define M515_BUS_H

#include <stdint.h>

//address space
//new bank size (0x4000)
#define DBVZ_BANK_SCOOT 14
#define DBVZ_NUM_BANKS(areaSize) (((areaSize) >> DBVZ_BANK_SCOOT) + ((areaSize) & ((1 << DBVZ_BANK_SCOOT) - 1) ? 1 : 0))
#define DBVZ_START_BANK(address) ((address) >> DBVZ_BANK_SCOOT)
#define DBVZ_END_BANK(address, size) (DBVZ_START_BANK(address) + DBVZ_NUM_BANKS(size) - 1)
#define DBVZ_BANK_IN_RANGE(bank, address, size) ((bank) >= DBVZ_START_BANK(address) && (bank) <= DBVZ_END_BANK(address, size))
#define DBVZ_BANK_ADDRESS(bank) ((bank) << DBVZ_BANK_SCOOT)
#define DBVZ_TOTAL_MEMORY_BANKS (1 << (32 - DBVZ_BANK_SCOOT))//0x40000 banks for BANK_SCOOT = 14

//chip addresses and sizes
//after boot RAM is at 0x00000000,
//ROM is at 0x10000000
//and the SED1376 is at 0x1FF80000(+ 0x20000 for framebuffer)
#define DBVZ_EMUCS_START_ADDRESS 0xFFFC0000
#define DBVZ_REG_START_ADDRESS 0xFFFFF000
#define M515_RAM_SIZE (16 * 0x100000)//16mb RAM
#define M515_ROM_SIZE (4 * 0x100000)//4mb ROM
#define DBVZ_EMUCS_SIZE 0x20000
#define DBVZ_REG_SIZE 0x1000//is actually 0xE00 without bootloader
#define DBVZ_BOOTLOADER_SIZE 0x200
#define SED1376_MR_BIT 0x20000

extern uint8_t dbvzBankType[];

void dbvzSetRegisterXXFFAccessMode(void);
void dbvzSetRegisterFFFFAccessMode(void);
void m515SetSed1376Attached(bool attached);
void dbvzResetAddressSpace(void);

#endif
