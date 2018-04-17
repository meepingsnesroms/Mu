#include <stdint.h>

#include "emulator.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "m68k/m68k.h"
#include "sed1376.h"


static memory_access_t bankAccessors[TOTAL_MEMORY_BANKS];//these are not part of savestates because function pointers change with -fPIC
uint8_t                bankType[TOTAL_MEMORY_BANKS];//these go in savestates


//used for unmapped address space and writes to write protected chipselects, should trigger access exceptions if enabled
static unsigned int invalidRead(unsigned int address){return 0x00000000;}
static void invalidWrite(unsigned int address, unsigned int value){}

//RAM accesses
static unsigned int ramRead8(unsigned int address){return BUFFER_READ_8(palmRam, address, chips[CHIP_D_RAM].start, chips[CHIP_D_RAM].mask);}
static unsigned int ramRead16(unsigned int address){return BUFFER_READ_16(palmRam, address, chips[CHIP_D_RAM].start, chips[CHIP_D_RAM].mask);}
static unsigned int ramRead32(unsigned int address){return BUFFER_READ_32(palmRam, address, chips[CHIP_D_RAM].start, chips[CHIP_D_RAM].mask);}
static void ramWrite8(unsigned int address, unsigned int value){BUFFER_WRITE_8(palmRam, address, chips[CHIP_D_RAM].start, chips[CHIP_D_RAM].mask, value);}
static void ramWrite16(unsigned int address, unsigned int value){BUFFER_WRITE_16(palmRam, address, chips[CHIP_D_RAM].start, chips[CHIP_D_RAM].mask, value);}
static void ramWrite32(unsigned int address, unsigned int value){BUFFER_WRITE_32(palmRam, address, chips[CHIP_D_RAM].start, chips[CHIP_D_RAM].mask, value);}

//ROM accesses
static unsigned int romRead8(unsigned int address){return BUFFER_READ_8(palmRom, address, chips[CHIP_A_ROM].start, chips[CHIP_A_ROM].mask);}
static unsigned int romRead16(unsigned int address){return BUFFER_READ_16(palmRom, address, chips[CHIP_A_ROM].start, chips[CHIP_A_ROM].mask);}
static unsigned int romRead32(unsigned int address){return BUFFER_READ_32(palmRom, address, chips[CHIP_A_ROM].start, chips[CHIP_A_ROM].mask);}
//the validity of attempting to write to ROM is determined by the read only bit in CSA, either way the write will do nothing
static void romWrite(unsigned int address, unsigned int value){}

//SED1376 framebuffer
static unsigned int sed1376FramebufferRead8(unsigned int address){return BUFFER_READ_8(sed1376Framebuffer, address, chips[CHIP_B_SED].start + SED1376_REG_SIZE, chips[CHIP_B_SED].mask);}
static unsigned int sed1376FramebufferRead16(unsigned int address){return BUFFER_READ_16(sed1376Framebuffer, address, chips[CHIP_B_SED].start + SED1376_REG_SIZE, chips[CHIP_B_SED].mask);}
static unsigned int sed1376FramebufferRead32(unsigned int address){return BUFFER_READ_32(sed1376Framebuffer, address, chips[CHIP_B_SED].start + SED1376_REG_SIZE, chips[CHIP_B_SED].mask);}
static void sed1376FramebufferWrite8(unsigned int address, unsigned int value){BUFFER_WRITE_8(sed1376Framebuffer, address, chips[CHIP_B_SED].start + SED1376_REG_SIZE, chips[CHIP_B_SED].mask, value);}
static void sed1376FramebufferWrite16(unsigned int address, unsigned int value){BUFFER_WRITE_16(sed1376Framebuffer, address, chips[CHIP_B_SED].start + SED1376_REG_SIZE, chips[CHIP_B_SED].mask, value);}
static void sed1376FramebufferWrite32(unsigned int address, unsigned int value){BUFFER_WRITE_32(sed1376Framebuffer, address, chips[CHIP_B_SED].start + SED1376_REG_SIZE, chips[CHIP_B_SED].mask, value);}


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
   //normal 68k has 16 bit bus but Dragonball VZ has 32 bit bus, so just write all at once(unverified, may not be accurate)
   bankAccessors[address >> 16].write32(address, value >> 16 | value << 16);
}

/* Memory access for the disassembler */
unsigned int m68k_read_disassembler_8(unsigned int address){return m68k_read_memory_8(address);}
unsigned int m68k_read_disassembler_16(unsigned int address){return m68k_read_memory_16(address);}
unsigned int m68k_read_disassembler_32(unsigned int address){return m68k_read_memory_32(address);}


static uint8_t getProperBankType(uint16_t bank){
   //special conditions
   if((bank & 0x00FF) == 0x00FF && registersAreXXFFMapped()){
      //XXFF register mode
      return REG_BANK;
   }
   
   //normal banks
   else if(chips[CHIP_A_ROM].enable && BANK_IN_RANGE(bank, chips[CHIP_A_ROM].start, chips[CHIP_A_ROM].size)){
      return ROM_BANK;
   }
   if(chips[CHIP_D_RAM].enable && BANK_IN_RANGE(bank, chips[CHIP_D_RAM].start, chips[CHIP_D_RAM].size)){
      return RAM_BANK;
   }
   else if(chips[CHIP_B_SED].enable && BANK_IN_RANGE(bank, chips[CHIP_B_SED].start, chips[CHIP_B_SED].size) && sed1376ClockConnected()){
      if(bank - START_BANK(chips[CHIP_B_SED].start) < NUM_BANKS(SED1376_REG_SIZE))
         return SED1376_REG_BANK;
      return SED1376_FB_BANK;
   }
   else if(BANK_IN_RANGE(bank, REG_START_ADDRESS, REG_SIZE)){
      return REG_BANK;
   }
   
   return EMPTY_BANK;
}

static bool getBankProtection(uint16_t bank){
   //special conditions
   if((bank & 0x00FF) == 0x00FF && registersAreXXFFMapped()){
      //XXFF register mode
      return false;
   }

   //normal banks
   else if(chips[CHIP_A_ROM].enable && BANK_IN_RANGE(bank, chips[CHIP_A_ROM].start, chips[CHIP_A_ROM].size)){
      return chips[CHIP_A_ROM].readOnly;//CSA has no protected memory segment
   }
   if(chips[CHIP_D_RAM].enable && BANK_IN_RANGE(bank, chips[CHIP_D_RAM].start, chips[CHIP_D_RAM].size)){
      return BANK_READ_ONLY(bank, CHIP_D_RAM);
   }
   else if(chips[CHIP_B_SED].enable && BANK_IN_RANGE(bank, chips[CHIP_B_SED].start, chips[CHIP_B_SED].size) && sed1376ClockConnected()){
      return BANK_READ_ONLY(bank, CHIP_B_SED);
   }
   else if(BANK_IN_RANGE(bank, REG_START_ADDRESS, REG_SIZE)){
      return false;
   }

   return false;
}

static void setBankType(uint16_t bank, uint8_t type, bool writeProtected){
   bankType[bank] = type;

   //read handlers
   switch(type){
         
      case EMPTY_BANK:
         bankAccessors[bank].read8 = invalidRead;
         bankAccessors[bank].read16 = invalidRead;
         bankAccessors[bank].read32 = invalidRead;
         break;
         
      case RAM_BANK:
         bankAccessors[bank].read8 = ramRead8;
         bankAccessors[bank].read16 = ramRead16;
         bankAccessors[bank].read32 = ramRead32;
         break;
         
      case ROM_BANK:
         bankAccessors[bank].read8 = romRead8;
         bankAccessors[bank].read16 = romRead16;
         bankAccessors[bank].read32 = romRead32;
         break;
         
      case REG_BANK:
         bankAccessors[bank].read8 = getHwRegister8;
         bankAccessors[bank].read16 = getHwRegister16;
         bankAccessors[bank].read32 = getHwRegister32;
         break;
         
      case SED1376_REG_BANK:
         bankAccessors[bank].read8 = sed1376GetRegister;
         bankAccessors[bank].read16 = sed1376GetRegister;
         bankAccessors[bank].read32 = sed1376GetRegister;
         break;
         
      case SED1376_FB_BANK:
         bankAccessors[bank].read8 = sed1376FramebufferRead8;
         bankAccessors[bank].read16 = sed1376FramebufferRead16;
         bankAccessors[bank].read32 = sed1376FramebufferRead32;
         break;
   }

   //write handlers
   if(!writeProtected){
      switch(type){

         case EMPTY_BANK:
            bankAccessors[bank].write8 = invalidWrite;
            bankAccessors[bank].write16 = invalidWrite;
            bankAccessors[bank].write32 = invalidWrite;
            break;

         case RAM_BANK:
            bankAccessors[bank].write8 = ramWrite8;
            bankAccessors[bank].write16 = ramWrite16;
            bankAccessors[bank].write32 = ramWrite32;
            break;

         case ROM_BANK:
            bankAccessors[bank].write8 = romWrite;
            bankAccessors[bank].write16 = romWrite;
            bankAccessors[bank].write32 = romWrite;
            break;

         case REG_BANK:
            bankAccessors[bank].write8 = setHwRegister8;
            bankAccessors[bank].write16 = setHwRegister16;
            bankAccessors[bank].write32 = setHwRegister32;
            break;

         case SED1376_REG_BANK:
            bankAccessors[bank].write8 = sed1376SetRegister;
            bankAccessors[bank].write16 = sed1376SetRegister;
            bankAccessors[bank].write32 = sed1376SetRegister;
            break;

         case SED1376_FB_BANK:
            bankAccessors[bank].write8 = sed1376FramebufferWrite8;
            bankAccessors[bank].write16 = sed1376FramebufferWrite16;
            bankAccessors[bank].write32 = sed1376FramebufferWrite32;
            break;
      }
   }
   else{
      bankAccessors[bank].write8 = invalidWrite;
      bankAccessors[bank].write16 = invalidWrite;
      bankAccessors[bank].write32 = invalidWrite;
   }
}

void setRegisterXXFFAccessMode(){
   for(uint16_t topByte = 0; topByte < 0x100; topByte++){
      uint32_t bank = topByte << 8 | 0xFF;
      setBankType(bank, REG_BANK, false);
   }
}

void setRegisterFFFFAccessMode(){
   for(uint16_t topByte = 0; topByte < 0x100; topByte++){
      uint32_t bank = topByte << 8 | 0xFF;
      setBankType(bank, getProperBankType(bank), getBankProtection(bank));
   }
}

void setSed1376Attached(bool attached){
   if(chips[CHIP_B_SED].enable){
      if(attached){
         for(uint32_t bank = START_BANK(chips[CHIP_B_SED].start); bank < END_BANK(chips[CHIP_B_SED].start, chips[CHIP_B_SED].size); bank++){
            if(bank - START_BANK(chips[CHIP_B_SED].start) < NUM_BANKS(SED1376_REG_SIZE))
               setBankType(bank, SED1376_REG_BANK, getBankProtection(bank));
            else
               setBankType(bank, SED1376_FB_BANK, getBankProtection(bank));
         }
      }
      else{
         for(uint32_t bank = START_BANK(chips[CHIP_B_SED].start); bank < END_BANK(chips[CHIP_B_SED].start, chips[CHIP_B_SED].size); bank++){
            setBankType(bank, EMPTY_BANK, getBankProtection(bank));
         }
      }
   }
}

void refreshBankHandlers(){
   for(uint32_t bank = 0; bank < TOTAL_MEMORY_BANKS; bank++)
      setBankType(bank, bankType[bank], getBankProtection(bank));
}

void resetAddressSpace(){
   for(uint32_t bank = 0; bank < TOTAL_MEMORY_BANKS; bank++)
      setBankType(bank, getProperBankType(bank), getBankProtection(bank));
}
