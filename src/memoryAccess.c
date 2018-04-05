#include <stdint.h>
#include <stdio.h>

#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "m68k/m68k.h"
#include "sed1376.h"


static memory_access_t bankAccessors[TOTAL_MEMORY_BANKS];//these are not part of savestates because function pointers can change with -fPIC
uint8_t                bankType[TOTAL_MEMORY_BANKS];//these go in savestates

//used for unmapped address space and writes to rom
static unsigned int unmappedRead(unsigned int address){return 0x00000000;}
static void unmappedWrite(unsigned int address, unsigned int value){}

//ram accesses
static unsigned int ramRead8(unsigned int address){return BUFFER_READ_8(palmRam, address, RAM_START_ADDRESS);}
static unsigned int ramRead16(unsigned int address){return BUFFER_READ_16(palmRam, address, RAM_START_ADDRESS);}
static unsigned int ramRead32(unsigned int address){return BUFFER_READ_32(palmRam, address, RAM_START_ADDRESS);}
static void ramWrite8(unsigned int address, unsigned int value){BUFFER_WRITE_8(palmRam, address, RAM_START_ADDRESS, value);}
static void ramWrite16(unsigned int address, unsigned int value){BUFFER_WRITE_16(palmRam, address, RAM_START_ADDRESS, value);}
static void ramWrite32(unsigned int address, unsigned int value){BUFFER_WRITE_32(palmRam, address, RAM_START_ADDRESS, value);}

//rom accesses
static unsigned int romRead8(unsigned int address){return BUFFER_READ_8(palmRom, address, ROM_START_ADDRESS);}
static unsigned int romRead16(unsigned int address){return BUFFER_READ_16(palmRom, address, ROM_START_ADDRESS);}
static unsigned int romRead32(unsigned int address){return BUFFER_READ_32(palmRom, address, ROM_START_ADDRESS);}

//sed1376 framebuffer
static unsigned int sed1376FramebufferRead8(unsigned int address){return BUFFER_READ_8(sed1376Framebuffer, address, SED1376_FB_START_ADDRESS);}
static unsigned int sed1376FramebufferRead16(unsigned int address){return BUFFER_READ_16(sed1376Framebuffer, address, SED1376_FB_START_ADDRESS);}
static unsigned int sed1376FramebufferRead32(unsigned int address){return BUFFER_READ_32(sed1376Framebuffer, address, SED1376_FB_START_ADDRESS);}
static void sed1376FramebufferWrite8(unsigned int address, unsigned int value){BUFFER_WRITE_8(sed1376Framebuffer, address, SED1376_FB_START_ADDRESS, value);}
static void sed1376FramebufferWrite16(unsigned int address, unsigned int value){BUFFER_WRITE_16(sed1376Framebuffer, address, SED1376_FB_START_ADDRESS, value);}
static void sed1376FramebufferWrite32(unsigned int address, unsigned int value){BUFFER_WRITE_32(sed1376Framebuffer, address, SED1376_FB_START_ADDRESS, value);}


/* Read from anywhere */
unsigned int  m68k_read_memory_8(unsigned int address){return bankAccessors[address >> 16].read8(address);}
unsigned int  m68k_read_memory_16(unsigned int address){return bankAccessors[address >> 16].read16(address);}
unsigned int  m68k_read_memory_32(unsigned int address){return bankAccessors[address >> 16].read32(address);}

/* Write to anywhere */
void m68k_write_memory_8(unsigned int address, unsigned int value){bankAccessors[address >> 16].write8(address, value);}
void m68k_write_memory_16(unsigned int address, unsigned int value){bankAccessors[address >> 16].write16(address, value);}
void m68k_write_memory_32(unsigned int address, unsigned int value){bankAccessors[address >> 16].write32(address, value);}
void m68k_write_memory_32_pd(unsigned int address, unsigned int value){
   //musashi says to write 2 16 bit words, but for now I am just writing as 32bit long
   bankAccessors[address >> 16].write32(address, value >> 16 | value << 16);
}

/* Memory access for the disassembler */
unsigned int m68k_read_disassembler_8(unsigned int address){return m68k_read_memory_8(address);}
unsigned int m68k_read_disassembler_16(unsigned int address){return m68k_read_memory_16(address);}
unsigned int m68k_read_disassembler_32(unsigned int address){return m68k_read_memory_32(address);}


static uint8_t getProperBankType(uint16_t bank){
   //ram bank 0x0000 not correct, inaccurate but works fine
   //0xXXFF register mode is handled separately
   if(BANK_IN_RANGE(bank, RAM_START_ADDRESS, RAM_SIZE))
      return RAM_BANK;
   else if(BANK_IN_RANGE(bank, ROM_START_ADDRESS, ROM_SIZE))
      return ROM_BANK;
   else if(BANK_IN_RANGE(bank, REG_START_ADDRESS, REG_SIZE))
      return REG_BANK;
   else if(BANK_IN_RANGE(bank, SED1376_REG_START_ADDRESS, SED1376_REG_SIZE))
      return SED1376_REG_BANK;
   else if(BANK_IN_RANGE(bank, SED1376_FB_START_ADDRESS, SED1376_FB_SIZE))
      return SED1376_FB_BANK;
   
   return EMPTY_BANK;
}

static void setBankType(uint16_t bank, uint8_t type){
   bankType[bank] = type;
   switch(type){
         
      case EMPTY_BANK:
         bankAccessors[bank].read8 = unmappedRead;
         bankAccessors[bank].read16 = unmappedRead;
         bankAccessors[bank].read32 = unmappedRead;
         bankAccessors[bank].write8 = unmappedWrite;
         bankAccessors[bank].write16 = unmappedWrite;
         bankAccessors[bank].write32 = unmappedWrite;
         break;
         
      case RAM_BANK:
         bankAccessors[bank].read8 = ramRead8;
         bankAccessors[bank].read16 = ramRead16;
         bankAccessors[bank].read32 = ramRead32;
         bankAccessors[bank].write8 = ramWrite8;
         bankAccessors[bank].write16 = ramWrite16;
         bankAccessors[bank].write32 = ramWrite32;
         break;
         
      case ROM_BANK:
         bankAccessors[bank].read8 = romRead8;
         bankAccessors[bank].read16 = romRead16;
         bankAccessors[bank].read32 = romRead32;
         bankAccessors[bank].write8 = unmappedWrite;
         bankAccessors[bank].write16 = unmappedWrite;
         bankAccessors[bank].write32 = unmappedWrite;
         break;
         
      case REG_BANK:
         bankAccessors[bank].read8 = getHwRegister8;
         bankAccessors[bank].read16 = getHwRegister16;
         bankAccessors[bank].read32 = getHwRegister32;
         bankAccessors[bank].write8 = setHwRegister8;
         bankAccessors[bank].write16 = setHwRegister16;
         bankAccessors[bank].write32 = setHwRegister32;
         break;
         
      case SED1376_REG_BANK:
         bankAccessors[bank].read8 = sed1376GetRegister;
         bankAccessors[bank].read16 = sed1376GetRegister;
         bankAccessors[bank].read32 = sed1376GetRegister;
         bankAccessors[bank].write8 = sed1376SetRegister;
         bankAccessors[bank].write16 = sed1376SetRegister;
         bankAccessors[bank].write32 = sed1376SetRegister;
         break;
         
      case SED1376_FB_BANK:
         bankAccessors[bank].read8 = sed1376FramebufferRead8;
         bankAccessors[bank].read16 = sed1376FramebufferRead16;
         bankAccessors[bank].read32 = sed1376FramebufferRead32;
         bankAccessors[bank].write8 = sed1376FramebufferWrite8;
         bankAccessors[bank].write16 = sed1376FramebufferWrite16;
         bankAccessors[bank].write32 = sed1376FramebufferWrite32;
         break;
   }
}

void setRegisterXXFFAccessMode(){
   for(uint16_t topByte = 0; topByte < 0x100; topByte++){
      uint32_t bank = topByte << 8 | 0xFF;
      setBankType(bank, REG_BANK);
   }
}

void setRegisterFFFFAccessMode(){
   for(uint16_t topByte = 0; topByte < 0x100; topByte++){
      uint32_t bank = topByte << 8 | 0xFF;
      setBankType(bank, getProperBankType(bank));
   }
}

void setSed1376Attached(bool attached){
   if(attached){
      for(uint32_t bank = START_BANK(SED1376_REG_START_ADDRESS); bank < END_BANK(SED1376_REG_START_ADDRESS, SED1376_REG_SIZE); bank++){
         setBankType(bank, SED1376_REG_BANK);
      }
      for(uint32_t bank = START_BANK(SED1376_FB_START_ADDRESS); bank < END_BANK(SED1376_FB_START_ADDRESS, SED1376_FB_SIZE); bank++){
         setBankType(bank, SED1376_FB_BANK);
      }
   }
   else{
      for(uint32_t bank = START_BANK(SED1376_REG_START_ADDRESS); bank < END_BANK(SED1376_REG_START_ADDRESS, SED1376_REG_SIZE); bank++){
         setBankType(bank, EMPTY_BANK);
      }
      for(uint32_t bank = START_BANK(SED1376_FB_START_ADDRESS); bank < END_BANK(SED1376_FB_START_ADDRESS, SED1376_FB_SIZE); bank++){
         setBankType(bank, EMPTY_BANK);
      }
   }
}

void refreshBankHandlers(){
   for(uint32_t bank = 0; bank < TOTAL_MEMORY_BANKS; bank++)
      setBankType(bank, bankType[bank]);
}

void resetAddressSpace(){
   for(uint32_t bank = 0; bank < TOTAL_MEMORY_BANKS; bank++)
      setBankType(bank, getProperBankType(bank));
   
   //PFSEL = 0x87, sed1376 needs bit 0x04 set to be on, sed1376 is on at boot
   
   setRegisterXXFFAccessMode();
}
