#ifndef TUNGSTEN_C_BUS_H
#define TUNGSTEN_C_BUS_H

#define PXA255_ROM_START_ADDRESS 0x00000000
#define PXA255_RAM_START_ADDRESS 0xA0000000
#define PXA255_REG_START_ADDRESS 0x40000000
#define TUNGSTEN_C_ROM_SIZE (10 * 0x100000)//10mb ROM
#define TUNGSTEN_C_RAM_SIZE (64 * 0x100000)//64mb RAM

#define PXA255_BANK_SCOOT 26
#define PXA255_NUM_BANKS(areaSize) (((areaSize) >> PXA255_BANK_SCOOT) + ((areaSize) & ((1 << PXA255_BANK_SCOOT) - 1) ? 1 : 0))
#define PXA255_START_BANK(address) ((address) >> PXA255_BANK_SCOOT)
#define PXA255_END_BANK(address, size) (PXA255_START_BANK(address) + PXA255_NUM_BANKS(size) - 1)
#define PXA255_BANK_IN_RANGE(bank, address, size) ((bank) >= PXA255_START_BANK(address) && (bank) <= PXA255_END_BANK(address, size))
#define PXA255_BANK_ADDRESS(bank) ((bank) << PXA255_BANK_SCOOT)
#define PXA255_TOTAL_MEMORY_BANKS (1 << (32 - PXA255_BANK_SCOOT))//64 banks for *_BANK_SCOOT = 26


#endif
