#include <stdint.h>
#include <string.h>

#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "portability.h"
#include "flx68000.h"
#include "sed1376.h"
#include "pdiUsbD12.h"


uint8_t bankType[TOTAL_MEMORY_BANKS];


//RAM accesses
static inline uint8_t ramRead8(uint32_t address){return BUFFER_READ_8(palmRam, address, chips[CHIP_DX_RAM].mask);}
static inline uint16_t ramRead16(uint32_t address){return BUFFER_READ_16(palmRam, address, chips[CHIP_DX_RAM].mask);}
static inline uint32_t ramRead32(uint32_t address){return BUFFER_READ_32(palmRam, address, chips[CHIP_DX_RAM].mask);}
static inline void ramWrite8(uint32_t address, uint8_t value){BUFFER_WRITE_8(palmRam, address, chips[CHIP_DX_RAM].mask, value);}
static inline void ramWrite16(uint32_t address, uint16_t value){BUFFER_WRITE_16(palmRam, address, chips[CHIP_DX_RAM].mask, value);}
static inline void ramWrite32(uint32_t address, uint32_t value){BUFFER_WRITE_32(palmRam, address, chips[CHIP_DX_RAM].mask, value);}

//ROM accesses
static inline uint8_t romRead8(uint32_t address){return BUFFER_READ_8(palmRom, address, chips[CHIP_A0_ROM].mask);}
static inline uint16_t romRead16(uint32_t address){return BUFFER_READ_16(palmRom, address, chips[CHIP_A0_ROM].mask);}
static inline uint32_t romRead32(uint32_t address){return BUFFER_READ_32(palmRom, address, chips[CHIP_A0_ROM].mask);}

//SED1376 accesses
static inline uint8_t sed1376Read8(uint32_t address){
   if(sed1376PowerSaveEnabled())
      return 0x00;
   if(address & SED1376_MR_BIT)
      return BUFFER_READ_8(sed1376Framebuffer, address, chips[CHIP_B0_SED].mask);
   else
      return sed1376GetRegister(address & chips[CHIP_B0_SED].mask);
}
static inline uint16_t sed1376Read16(uint32_t address){
   if(sed1376PowerSaveEnabled())
      return 0x0000;
   if(address & SED1376_MR_BIT)
      return BUFFER_READ_16(sed1376Framebuffer, address, chips[CHIP_B0_SED].mask);
   else
      return sed1376GetRegister(address & chips[CHIP_B0_SED].mask);
}
static inline uint32_t sed1376Read32(uint32_t address){
   if(sed1376PowerSaveEnabled())
      return 0x00000000;
   if(address & SED1376_MR_BIT)
      return BUFFER_READ_32(sed1376Framebuffer, address, chips[CHIP_B0_SED].mask);
   else
      return sed1376GetRegister(address & chips[CHIP_B0_SED].mask);
}
static inline void sed1376Write8(uint32_t address, uint8_t value){
   if(address & SED1376_MR_BIT)
      BUFFER_WRITE_8(sed1376Framebuffer, address, chips[CHIP_B0_SED].mask, value);
   else
      sed1376SetRegister(address & chips[CHIP_B0_SED].mask, value);
}
static inline void sed1376Write16(uint32_t address, uint16_t value){
   if(address & SED1376_MR_BIT)
      BUFFER_WRITE_16(sed1376Framebuffer, address, chips[CHIP_B0_SED].mask, value);
   else
      sed1376SetRegister(address & chips[CHIP_B0_SED].mask, value);
}
static inline void sed1376Write32(uint32_t address, uint32_t value){
   if(address & SED1376_MR_BIT)
      BUFFER_WRITE_32(sed1376Framebuffer, address, chips[CHIP_B0_SED].mask, value);
   else
      sed1376SetRegister(address & chips[CHIP_B0_SED].mask, value);
}

static inline bool probeRead(uint8_t bank, uint32_t address){
   if(chips[bank].supervisorOnlyProtectedMemory){
      uint32_t index = address - chips[bank].start;
      if(index >= chips[bank].unprotectedSize && !flx68000IsSupervisor()){
         setPrivilegeViolation(address, false);
         return false;
      }
   }
   return true;
}

static inline bool probeWrite(uint8_t bank, uint32_t address){
   if(chips[bank].readOnly){
      setWriteProtectViolation(address);
      return false;
   }
   else if(chips[bank].supervisorOnlyProtectedMemory || chips[bank].readOnlyForProtectedMemory){
      uint32_t index = address - chips[bank].start;
      if(index >= chips[bank].unprotectedSize){
         if(chips[bank].supervisorOnlyProtectedMemory && !flx68000IsSupervisor()){
            setPrivilegeViolation(address, true);
            return false;
         }
         if(chips[bank].readOnlyForProtectedMemory){
            setWriteProtectViolation(address);
            return false;
         }
      }
   }
   return true;
}

uint8_t m68k_read_memory_8(uint32_t address){
   uint8_t addressType = bankType[START_BANK(address)];

   if(!probeRead(addressType, address))
      return 0x00;

   switch(addressType){
      case CHIP_A0_ROM:
         return romRead8(address);

      case CHIP_A1_USB:
         return pdiUsbD12GetRegister(address & chips[CHIP_A1_USB].mask);

      case CHIP_B0_SED:
         return sed1376Read8(address);

      case CHIP_DX_RAM:
         return ramRead8(address);

      case CHIP_00_EMU:
         return 0x00;

      case CHIP_REGISTERS:
         return getHwRegister8(address);

      case CHIP_NONE:
         setBusErrorTimeOut(address, false);
         return 0x00;

      default:
         debugLog("Unknown bank type:%d\n", bankType[START_BANK(address)]);
         break;
   }

   return 0x00;
}

uint16_t m68k_read_memory_16(uint32_t address){
   uint8_t addressType = bankType[START_BANK(address)];

   if(!probeRead(addressType, address))
      return 0x0000;

   switch(addressType){
      case CHIP_A0_ROM:
         return romRead16(address);

      case CHIP_A1_USB:
         return pdiUsbD12GetRegister(address & chips[CHIP_A1_USB].mask);

      case CHIP_B0_SED:
         return sed1376Read16(address);

      case CHIP_DX_RAM:
         return ramRead16(address);

      case CHIP_00_EMU:
         return 0x0000;

      case CHIP_REGISTERS:
         return getHwRegister16(address);

      case CHIP_NONE:
         setBusErrorTimeOut(address, false);
         return 0x0000;

      default:
         debugLog("Unknown bank type:%d\n", bankType[START_BANK(address)]);
         break;
   }

   return 0x0000;
}

uint32_t m68k_read_memory_32(uint32_t address){
   uint8_t addressType = bankType[START_BANK(address)];

   if(!probeRead(addressType, address))
      return 0x00000000;

   switch(addressType){
      case CHIP_A0_ROM:
         return romRead32(address);

      case CHIP_A1_USB:
         return pdiUsbD12GetRegister(address & chips[CHIP_A1_USB].mask);

      case CHIP_B0_SED:
         return sed1376Read32(address);

      case CHIP_DX_RAM:
         return ramRead32(address);

      case CHIP_00_EMU:
         return getEmuRegister(address);

      case CHIP_REGISTERS:
         return getHwRegister32(address);

      case CHIP_NONE:
         setBusErrorTimeOut(address, false);
         return 0x00000000;

      default:
         debugLog("Unknown bank type:%d\n", bankType[START_BANK(address)]);
         break;
   }

   return 0x00000000;
}

void m68k_write_memory_8(uint32_t address, uint8_t value){
   uint8_t addressType = bankType[START_BANK(address)];

   if(!probeWrite(addressType, address))
      return;

   switch(addressType){
      case CHIP_A0_ROM:
         break;

      case CHIP_A1_USB:
         pdiUsbD12SetRegister(address & chips[CHIP_A1_USB].mask, value);
         break;

      case CHIP_B0_SED:
         sed1376Write8(address, value);
         break;

      case CHIP_DX_RAM:
         ramWrite8(address, value);
         break;

      case CHIP_00_EMU:
         break;

      case CHIP_REGISTERS:
         setHwRegister8(address, value);
         break;

      case CHIP_NONE:
         setBusErrorTimeOut(address, true);
         break;

      default:
         debugLog("Unknown bank type:%d\n", bankType[START_BANK(address)]);
         break;
   }

   return;
}

void m68k_write_memory_16(uint32_t address, uint16_t value){
   uint8_t addressType = bankType[START_BANK(address)];

   if(!probeWrite(addressType, address))
      return;

   switch(addressType){
      case CHIP_A0_ROM:
         break;

      case CHIP_A1_USB:
         pdiUsbD12SetRegister(address & chips[CHIP_A1_USB].mask, value);
         break;

      case CHIP_B0_SED:
         sed1376Write16(address, value);
         break;

      case CHIP_DX_RAM:
         ramWrite16(address, value);
         break;

      case CHIP_00_EMU:
         break;

      case CHIP_REGISTERS:
         setHwRegister16(address, value);
         break;

      case CHIP_NONE:
         setBusErrorTimeOut(address, true);
         break;

      default:
         debugLog("Unknown bank type:%d\n", bankType[START_BANK(address)]);
         break;
   }

   return;
}

void m68k_write_memory_32(uint32_t address, uint32_t value){
   uint8_t addressType = bankType[START_BANK(address)];

   if(!probeWrite(addressType, address))
      return;

   switch(addressType){
      case CHIP_A0_ROM:
         break;

      case CHIP_A1_USB:
         pdiUsbD12SetRegister(address & chips[CHIP_A1_USB].mask, value);
         break;

      case CHIP_B0_SED:
         sed1376Write32(address, value);
         break;

      case CHIP_DX_RAM:
         ramWrite32(address, value);
         break;

      case CHIP_00_EMU:
         setEmuRegister(address, value);
         break;

      case CHIP_REGISTERS:
         setHwRegister32(address, value);
         break;

      case CHIP_NONE:
         setBusErrorTimeOut(address, true);
         break;

      default:
         debugLog("Unknown bank type:%d\n", bankType[START_BANK(address)]);
         break;
   }

   return;
}

void m68k_write_memory_32_pd(uint32_t address, uint32_t value){
   //musashi says to write 2 16 bit words, but for now I am just writing a 32 bit long since RAM cycles are unemulated
   m68k_write_memory_32(address, value >> 16 | value << 16);
}

//memory access for the disassembler
uint8_t m68k_read_disassembler_8(uint32_t address){return m68k_read_memory_8(address);}
uint16_t m68k_read_disassembler_16(uint32_t address){return m68k_read_memory_16(address);}
uint32_t m68k_read_disassembler_32(uint32_t address){return m68k_read_memory_32(address);}


static uint8_t getProperBankType(uint32_t bank){
   //registers have first priority, they cover 0xFFFFF000(and 0xXXFFF000 when DMAP enabled in SCR) even if a chip select overlaps this area or CHIP_A0_ROM is in boot mode
   //EMUCS also cant be covered by normal chip selects
   if(BANK_IN_RANGE(bank, REG_START_ADDRESS, REG_SIZE) || ((bank & 0x00FF) == 0x00FF && registersAreXXFFMapped()))
      return CHIP_REGISTERS;
   else if(palmSpecialFeatures != FEATURE_ACCURATE && BANK_IN_RANGE(bank, EMUCS_START_ADDRESS, EMUCS_SIZE))
      return CHIP_00_EMU;
   else if(chips[CHIP_A0_ROM].inBootMode || (chips[CHIP_A0_ROM].enable && BANK_IN_RANGE(bank, chips[CHIP_A0_ROM].start, chips[CHIP_A0_ROM].lineSize)))
      return CHIP_A0_ROM;
   else if(chips[CHIP_A1_USB].enable && BANK_IN_RANGE(bank, chips[CHIP_A1_USB].start, chips[CHIP_A1_USB].lineSize))
      return CHIP_A1_USB;
   else if(chips[CHIP_B0_SED].enable && BANK_IN_RANGE(bank, chips[CHIP_B0_SED].start, chips[CHIP_B0_SED].lineSize) && sed1376ClockConnected())
      return CHIP_B0_SED;
   else if(chips[CHIP_DX_RAM].enable && BANK_IN_RANGE(bank, chips[CHIP_DX_RAM].start, chips[CHIP_DX_RAM].lineSize * 2))
      return CHIP_DX_RAM;

   return CHIP_NONE;
}

void setRegisterXXFFAccessMode(){
   MULTITHREAD_LOOP for(uint32_t topByte = 0; topByte < 0x100; topByte++)
      bankType[START_BANK(topByte << 24 | 0x00FFF000)] = CHIP_REGISTERS;
}

void setRegisterFFFFAccessMode(){
   MULTITHREAD_LOOP for(uint32_t topByte = 0; topByte < 0x100; topByte++){
      uint32_t bank = START_BANK(topByte << 24 | 0x00FFF000);
      bankType[bank] = getProperBankType(bank);
   }
}

void setSed1376Attached(bool attached){
   if(chips[CHIP_B0_SED].enable && bankType[START_BANK(chips[CHIP_B0_SED].start)] != (attached ? CHIP_B0_SED : CHIP_NONE))
      memset(&bankType[START_BANK(chips[CHIP_B0_SED].start)], attached ? CHIP_B0_SED : CHIP_NONE, END_BANK(chips[CHIP_B0_SED].start, chips[CHIP_B0_SED].lineSize) - START_BANK(chips[CHIP_B0_SED].start) + 1);
}

void resetAddressSpace(){
   MULTITHREAD_LOOP for(uint32_t bank = 0; bank < TOTAL_MEMORY_BANKS; bank++)
      bankType[bank] = getProperBankType(bank);
}
