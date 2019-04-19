#include <stdint.h>
#include <string.h>

#include "emulator.h"
#include "dbvzRegisters.h"
#include "expansionHardware.h"
#include "m515Bus.h"
#include "portability.h"
#include "flx68000.h"
#include "sed1376.h"
#include "pdiUsbD12.h"
#include "debug/sandbox.h"


uint8_t dbvzBankType[DBVZ_TOTAL_MEMORY_BANKS];


//RAM accesses
static uint8_t ramRead8(uint32_t address){return BUFFER_READ_8(palmRam, address, chips[DBVZ_CHIP_DX_RAM].mask);}
static uint16_t ramRead16(uint32_t address){return BUFFER_READ_16(palmRam, address, chips[DBVZ_CHIP_DX_RAM].mask);}
static uint32_t ramRead32(uint32_t address){return BUFFER_READ_32(palmRam, address, chips[DBVZ_CHIP_DX_RAM].mask);}
static void ramWrite8(uint32_t address, uint8_t value){BUFFER_WRITE_8(palmRam, address, chips[DBVZ_CHIP_DX_RAM].mask, value);}
static void ramWrite16(uint32_t address, uint16_t value){BUFFER_WRITE_16(palmRam, address, chips[DBVZ_CHIP_DX_RAM].mask, value);}
static void ramWrite32(uint32_t address, uint32_t value){BUFFER_WRITE_32(palmRam, address, chips[DBVZ_CHIP_DX_RAM].mask, value);}

//ROM accesses
static uint8_t romRead8(uint32_t address){return BUFFER_READ_8(palmRom, address, chips[DBVZ_CHIP_A0_ROM].mask);}
static uint16_t romRead16(uint32_t address){return BUFFER_READ_16(palmRom, address, chips[DBVZ_CHIP_A0_ROM].mask);}
static uint32_t romRead32(uint32_t address){return BUFFER_READ_32(palmRom, address, chips[DBVZ_CHIP_A0_ROM].mask);}

//SED1376 accesses
static uint8_t sed1376Read8(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if(sed1376PowerSaveEnabled())
      return 0x00;
#endif
   if(address & SED1376_MR_BIT)
      return BUFFER_READ_8_BIG_ENDIAN(sed1376Ram, address, chips[DBVZ_CHIP_B0_SED].mask);
   else
      return sed1376GetRegister(address & chips[DBVZ_CHIP_B0_SED].mask);
}
static uint16_t sed1376Read16(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if(sed1376PowerSaveEnabled())
      return 0x0000;
#endif
   if(address & SED1376_MR_BIT)
      return BUFFER_READ_16_BIG_ENDIAN(sed1376Ram, address, chips[DBVZ_CHIP_B0_SED].mask);
   else
      return sed1376GetRegister(address & chips[DBVZ_CHIP_B0_SED].mask);
}
static uint32_t sed1376Read32(uint32_t address){
#if !defined(EMU_NO_SAFETY)
   if(sed1376PowerSaveEnabled())
      return 0x00000000;
#endif
   if(address & SED1376_MR_BIT)
      return BUFFER_READ_32_BIG_ENDIAN(sed1376Ram, address, chips[DBVZ_CHIP_B0_SED].mask);
   else
      return sed1376GetRegister(address & chips[DBVZ_CHIP_B0_SED].mask);
}
static void sed1376Write8(uint32_t address, uint8_t value){
   if(address & SED1376_MR_BIT)
      BUFFER_WRITE_8_BIG_ENDIAN(sed1376Ram, address, chips[DBVZ_CHIP_B0_SED].mask, value);
   else
      sed1376SetRegister(address & chips[DBVZ_CHIP_B0_SED].mask, value);
}
static void sed1376Write16(uint32_t address, uint16_t value){
   if(address & SED1376_MR_BIT)
      BUFFER_WRITE_16_BIG_ENDIAN(sed1376Ram, address, chips[DBVZ_CHIP_B0_SED].mask, value);
   else
      sed1376SetRegister(address & chips[DBVZ_CHIP_B0_SED].mask, value);
}
static void sed1376Write32(uint32_t address, uint32_t value){
   if(address & SED1376_MR_BIT)
      BUFFER_WRITE_32_BIG_ENDIAN(sed1376Ram, address, chips[DBVZ_CHIP_B0_SED].mask, value);
   else
      sed1376SetRegister(address & chips[DBVZ_CHIP_B0_SED].mask, value);
}

static bool probeRead(uint8_t bank, uint32_t address){
   if(chips[bank].supervisorOnlyProtectedMemory){
      uint32_t index = address - chips[bank].start;
      if(index >= chips[bank].unprotectedSize && !flx68000IsSupervisor()){
         dbvzSetPrivilegeViolation(address, false);
         return false;
      }
   }
   return true;
}

static bool probeWrite(uint8_t bank, uint32_t address){
   if(chips[bank].readOnly){
      dbvzSetWriteProtectViolation(address);
      return false;
   }
   else if(chips[bank].supervisorOnlyProtectedMemory || chips[bank].readOnlyForProtectedMemory){
      uint32_t index = address - chips[bank].start;
      if(index >= chips[bank].unprotectedSize){
         if(chips[bank].supervisorOnlyProtectedMemory && !flx68000IsSupervisor()){
            dbvzSetPrivilegeViolation(address, true);
            return false;
         }
         if(chips[bank].readOnlyForProtectedMemory){
            dbvzSetWriteProtectViolation(address);
            return false;
         }
      }
   }
   return true;
}

uint8_t m68k_read_memory_8(uint32_t address){
   uint8_t addressType = dbvzBankType[DBVZ_START_BANK(address)];

#if !defined(EMU_NO_SAFETY)
   if(!probeRead(addressType, address))
      return 0x00;
#endif

#if defined(EMU_DEBUG) && defined(EMU_SANDBOX) && defined(EMU_SANDBOX_LOG_MEMORY_ACCESSES)
   sandboxOnMemoryAccess(address, 8, false, 0);
#endif

   switch(addressType){
      case DBVZ_CHIP_A0_ROM:
         return romRead8(address);

      case DBVZ_CHIP_A1_USB:
         return pdiUsbD12GetRegister(!!(address & chips[DBVZ_CHIP_A1_USB].mask));

      case DBVZ_CHIP_B0_SED:
         return sed1376Read8(address);

      case DBVZ_CHIP_DX_RAM:
         return ramRead8(address);

      case DBVZ_CHIP_00_EMU:
         return 0x00;

      case DBVZ_CHIP_REGISTERS:
         return dbvzGetRegister8(address);

      case DBVZ_CHIP_B1_NIL:
      case DBVZ_CHIP_NONE:
         dbvzSetBusErrorTimeOut(address, false);
         return 0x00;

      default:
         debugLog("Unknown bank type:%d\n", dbvzBankType[DBVZ_START_BANK(address)]);
         return 0x00;
   }
}

uint16_t m68k_read_memory_16(uint32_t address){
   uint8_t addressType = dbvzBankType[DBVZ_START_BANK(address)];

#if !defined(EMU_NO_SAFETY)
   if(!probeRead(addressType, address))
      return 0x0000;
#endif

#if defined(EMU_DEBUG) && defined(EMU_SANDBOX) && defined(EMU_SANDBOX_LOG_MEMORY_ACCESSES)
   sandboxOnMemoryAccess(address, 16, false, 0);
#endif

   switch(addressType){
      case DBVZ_CHIP_A0_ROM:
         return romRead16(address);

      case DBVZ_CHIP_A1_USB:
         return pdiUsbD12GetRegister(!!(address & chips[DBVZ_CHIP_A1_USB].mask));

      case DBVZ_CHIP_B0_SED:
         return sed1376Read16(address);

      case DBVZ_CHIP_DX_RAM:
         return ramRead16(address);

      case DBVZ_CHIP_00_EMU:
         return 0x0000;

      case DBVZ_CHIP_REGISTERS:
         return dbvzGetRegister16(address);

      case DBVZ_CHIP_B1_NIL:
      case DBVZ_CHIP_NONE:
         dbvzSetBusErrorTimeOut(address, false);
         return 0x0000;

      default:
         debugLog("Unknown bank type:%d\n", dbvzBankType[DBVZ_START_BANK(address)]);
         return 0x0000;
   }
}

uint32_t m68k_read_memory_32(uint32_t address){
   uint8_t addressType = dbvzBankType[DBVZ_START_BANK(address)];

#if !defined(EMU_NO_SAFETY)
   if(!probeRead(addressType, address))
      return 0x00000000;
#endif

#if defined(EMU_DEBUG) && defined(EMU_SANDBOX) && defined(EMU_SANDBOX_LOG_MEMORY_ACCESSES)
   sandboxOnMemoryAccess(address, 32, false, 0);
#endif

   switch(addressType){
      case DBVZ_CHIP_A0_ROM:
         return romRead32(address);

      case DBVZ_CHIP_A1_USB:
         return pdiUsbD12GetRegister(!!(address & chips[DBVZ_CHIP_A1_USB].mask));

      case DBVZ_CHIP_B0_SED:
         return sed1376Read32(address);

      case DBVZ_CHIP_DX_RAM:
         return ramRead32(address);

      case DBVZ_CHIP_00_EMU:
         return expansionHardwareGetRegister(address);

      case DBVZ_CHIP_REGISTERS:
         return dbvzGetRegister32(address);

      case DBVZ_CHIP_B1_NIL:
      case DBVZ_CHIP_NONE:
         dbvzSetBusErrorTimeOut(address, false);
         return 0x00000000;

      default:
         debugLog("Unknown bank type:%d\n", dbvzBankType[DBVZ_START_BANK(address)]);
         return 0x00000000;
   }
}

void m68k_write_memory_8(uint32_t address, uint8_t value){
   uint8_t addressType = dbvzBankType[DBVZ_START_BANK(address)];

#if !defined(EMU_NO_SAFETY)
   if(!probeWrite(addressType, address))
      return;
#endif

#if defined(EMU_DEBUG) && defined(EMU_SANDBOX) && defined(EMU_SANDBOX_LOG_MEMORY_ACCESSES)
   sandboxOnMemoryAccess(address, 8, true, value);
#endif

   switch(addressType){
      case DBVZ_CHIP_A0_ROM:
         return;

      case DBVZ_CHIP_A1_USB:
         pdiUsbD12SetRegister(!!(address & chips[DBVZ_CHIP_A1_USB].mask), value);
         return;

      case DBVZ_CHIP_B0_SED:
         sed1376Write8(address, value);
         return;

      case DBVZ_CHIP_DX_RAM:
         ramWrite8(address, value);
         return;

      case DBVZ_CHIP_00_EMU:
         return;

      case DBVZ_CHIP_REGISTERS:
         dbvzSetRegister8(address, value);
         return;

      case DBVZ_CHIP_B1_NIL:
      case DBVZ_CHIP_NONE:
         dbvzSetBusErrorTimeOut(address, true);
         return;

      default:
         debugLog("Unknown bank type:%d\n", dbvzBankType[DBVZ_START_BANK(address)]);
         return;
   }
}

void m68k_write_memory_16(uint32_t address, uint16_t value){
   uint8_t addressType = dbvzBankType[DBVZ_START_BANK(address)];

#if !defined(EMU_NO_SAFETY)
   if(!probeWrite(addressType, address))
      return;
#endif

#if defined(EMU_DEBUG) && defined(EMU_SANDBOX) && defined(EMU_SANDBOX_LOG_MEMORY_ACCESSES)
   sandboxOnMemoryAccess(address, 16, true, value);
#endif

   switch(addressType){
      case DBVZ_CHIP_A0_ROM:
         return;

      case DBVZ_CHIP_A1_USB:
         pdiUsbD12SetRegister(!!(address & chips[DBVZ_CHIP_A1_USB].mask), value);
         return;

      case DBVZ_CHIP_B0_SED:
         sed1376Write16(address, value);
         return;

      case DBVZ_CHIP_DX_RAM:
         ramWrite16(address, value);
         return;

      case DBVZ_CHIP_00_EMU:
         return;

      case DBVZ_CHIP_REGISTERS:
         dbvzSetRegister16(address, value);
         return;

      case DBVZ_CHIP_B1_NIL:
      case DBVZ_CHIP_NONE:
         dbvzSetBusErrorTimeOut(address, true);
         return;

      default:
         debugLog("Unknown bank type:%d\n", dbvzBankType[DBVZ_START_BANK(address)]);
         return;
   }
}

void m68k_write_memory_32(uint32_t address, uint32_t value){
   uint8_t addressType = dbvzBankType[DBVZ_START_BANK(address)];

#if !defined(EMU_NO_SAFETY)
   if(!probeWrite(addressType, address))
      return;
#endif

#if defined(EMU_DEBUG) && defined(EMU_SANDBOX) && defined(EMU_SANDBOX_LOG_MEMORY_ACCESSES)
   sandboxOnMemoryAccess(address, 32, true, value);
#endif

   switch(addressType){
      case DBVZ_CHIP_A0_ROM:
         return;

      case DBVZ_CHIP_A1_USB:
         pdiUsbD12SetRegister(!!(address & chips[DBVZ_CHIP_A1_USB].mask), value);
         return;

      case DBVZ_CHIP_B0_SED:
         sed1376Write32(address, value);
         return;

      case DBVZ_CHIP_DX_RAM:
         ramWrite32(address, value);
         return;

      case DBVZ_CHIP_00_EMU:
         expansionHardwareSetRegister(address, value);
         return;

      case DBVZ_CHIP_REGISTERS:
         dbvzSetRegister32(address, value);
         return;

      case DBVZ_CHIP_B1_NIL:
      case DBVZ_CHIP_NONE:
         dbvzSetBusErrorTimeOut(address, true);
         return;

      default:
         debugLog("Unknown bank type:%d\n", dbvzBankType[DBVZ_START_BANK(address)]);
         return;
   }
}

void m68k_write_memory_32_pd(uint32_t address, uint32_t value){
   m68k_write_memory_32(address, value >> 16 | value << 16);
}

//memory access for the disassembler, unused but must be defined
uint8_t m68k_read_disassembler_8(uint32_t address){return m68k_read_memory_8(address);}
uint16_t m68k_read_disassembler_16(uint32_t address){return m68k_read_memory_16(address);}
uint32_t m68k_read_disassembler_32(uint32_t address){return m68k_read_memory_32(address);}


static uint8_t getProperBankType(uint32_t bank){
   //registers have first priority, they cover 0xFFFFF000(and 0xXXFFF000 when DMAP enabled in SCR) even if a chip select overlaps this area or DBVZ_CHIP_A0_ROM is in boot mode
   //EMUCS also cant be covered by normal chip selects
   if(DBVZ_BANK_IN_RANGE(bank, DBVZ_REG_START_ADDRESS, DBVZ_REG_SIZE) || ((bank & 0x00FF) == 0x00FF && dbvzAreRegistersXXFFMapped()))
      return DBVZ_CHIP_REGISTERS;
   else if(palmEmuFeatures.info != FEATURE_ACCURATE && DBVZ_BANK_IN_RANGE(bank, DBVZ_EMUCS_START_ADDRESS, DBVZ_EMUCS_SIZE))
      return DBVZ_CHIP_00_EMU;
   else if(chips[DBVZ_CHIP_A0_ROM].inBootMode || (chips[DBVZ_CHIP_A0_ROM].enable && DBVZ_BANK_IN_RANGE(bank, chips[DBVZ_CHIP_A0_ROM].start, chips[DBVZ_CHIP_A0_ROM].lineSize)))
      return DBVZ_CHIP_A0_ROM;
   else if(chips[DBVZ_CHIP_DX_RAM].enable && DBVZ_BANK_IN_RANGE(bank, chips[DBVZ_CHIP_DX_RAM].start, chips[DBVZ_CHIP_DX_RAM].lineSize * 2))
      return DBVZ_CHIP_DX_RAM;
   else if(chips[DBVZ_CHIP_B0_SED].enable && DBVZ_BANK_IN_RANGE(bank, chips[DBVZ_CHIP_B0_SED].start, chips[DBVZ_CHIP_B0_SED].lineSize) && sed1376ClockConnected())
      return DBVZ_CHIP_B0_SED;
   else if(chips[DBVZ_CHIP_A1_USB].enable && DBVZ_BANK_IN_RANGE(bank, chips[DBVZ_CHIP_A1_USB].start, chips[DBVZ_CHIP_A1_USB].lineSize))
      return DBVZ_CHIP_A1_USB;
   else if(chips[DBVZ_CHIP_B1_NIL].enable && DBVZ_BANK_IN_RANGE(bank, chips[DBVZ_CHIP_B1_NIL].start, chips[DBVZ_CHIP_B1_NIL].lineSize))
      return DBVZ_CHIP_B1_NIL;

   return DBVZ_CHIP_NONE;
}

void dbvzSetRegisterXXFFAccessMode(void){
   uint32_t topByte;

   MULTITHREAD_LOOP(topByte) for(topByte = 0; topByte < 0x100; topByte++)
      dbvzBankType[DBVZ_START_BANK(topByte << 24 | 0x00FFF000)] = DBVZ_CHIP_REGISTERS;
}

void dbvzSetRegisterFFFFAccessMode(void){
   uint32_t topByte;

   MULTITHREAD_LOOP(topByte) for(topByte = 0; topByte < 0x100; topByte++){
      uint32_t bank = DBVZ_START_BANK(topByte << 24 | 0x00FFF000);
      dbvzBankType[bank] = getProperBankType(bank);
   }
}

void m515SetSed1376Attached(bool attached){
   if(chips[DBVZ_CHIP_B0_SED].enable && dbvzBankType[DBVZ_START_BANK(chips[DBVZ_CHIP_B0_SED].start)] != (attached ? DBVZ_CHIP_B0_SED : DBVZ_CHIP_NONE))
      memset(&dbvzBankType[DBVZ_START_BANK(chips[DBVZ_CHIP_B0_SED].start)], attached ? DBVZ_CHIP_B0_SED : DBVZ_CHIP_NONE, DBVZ_END_BANK(chips[DBVZ_CHIP_B0_SED].start, chips[DBVZ_CHIP_B0_SED].lineSize) - DBVZ_START_BANK(chips[DBVZ_CHIP_B0_SED].start) + 1);
}

void dbvzResetAddressSpace(void){
   uint32_t bank;

   MULTITHREAD_LOOP(bank) for(bank = 0; bank < DBVZ_TOTAL_MEMORY_BANKS; bank++)
      dbvzBankType[bank] = getProperBankType(bank);
}
