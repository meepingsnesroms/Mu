#include <stdint.h>
#include <stdio.h>

#include "emulator.h"
#include "hardwareRegisters.h"
#include "m68k/m68k.h"


/* Read from anywhere */
unsigned int  m68k_read_memory_8(unsigned int address){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE)
      return palmRam[address - RAM_START_ADDRESS];
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE)
      return palmRom[address - ROM_START_ADDRESS];
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE)
      return getHwRegister8(address - REG_START_ADDRESS);
   else if(address >= SED1376_REG_START_ADDRESS && address < SED1376_REG_START_ADDRESS + SED1376_REG_SIZE)
      return sed1376GetRegister(address - SED1376_REG_START_ADDRESS);
   
   //printf("Invalid 8 bit read at 0x%08X.\n", address);
   
   return 0x00;
}

unsigned int  m68k_read_memory_16(unsigned int address){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE)
      return palmRam[address - RAM_START_ADDRESS] << 8 | palmRam[address - RAM_START_ADDRESS + 1];
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE)
      return palmRom[address - ROM_START_ADDRESS] << 8 | palmRom[address - ROM_START_ADDRESS + 1];
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE)
      return getHwRegister16(address - REG_START_ADDRESS);
   else if(address >= SED1376_REG_START_ADDRESS && address < SED1376_REG_START_ADDRESS + SED1376_REG_SIZE)
      return sed1376GetRegister(address - SED1376_REG_START_ADDRESS);
   
   //printf("Invalid 16 bit read at 0x%08X.\n", address);
   
   return 0x0000;
}

unsigned int  m68k_read_memory_32(unsigned int address){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE)
      return palmRam[address - RAM_START_ADDRESS] << 24 | palmRam[address - RAM_START_ADDRESS + 1] << 16 | palmRam[address - RAM_START_ADDRESS + 2] << 8 | palmRam[address - RAM_START_ADDRESS + 3];
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE)
      return palmRom[address - ROM_START_ADDRESS] << 24 | palmRom[address - ROM_START_ADDRESS + 1] << 16 | palmRom[address - ROM_START_ADDRESS + 2] << 8 | palmRom[address - ROM_START_ADDRESS + 3];
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE)
      return getHwRegister32(address - REG_START_ADDRESS);
   else if(address >= SED1376_REG_START_ADDRESS && address < SED1376_REG_START_ADDRESS + SED1376_REG_SIZE)
      return sed1376GetRegister(address - SED1376_REG_START_ADDRESS);
   
   //printf("Invalid 32 bit read at 0x%08X.\n", address);
   
   return 0x00000000;
}

/* Write to anywhere */
void m68k_write_memory_8(unsigned int address, unsigned int value){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      palmRam[address - RAM_START_ADDRESS] = value;
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      //cant write ROM
      printf("Tried to write 8 bits to ROM at 0x%08X with value of 0x%02X.\n", address, value);
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      setHwRegister8(address - REG_START_ADDRESS, value);
   }
   else if(address >= SED1376_REG_START_ADDRESS && address < SED1376_REG_START_ADDRESS + SED1376_REG_SIZE){
      sed1376SetRegister(address - SED1376_REG_START_ADDRESS, value);
   }
   else{
      //printf("Invalid 8 bit write at 0x%08X with value of 0x%02X.\n", address, value);
   }
}

void m68k_write_memory_16(unsigned int address, unsigned int value){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      palmRam[address - RAM_START_ADDRESS] = value >> 8;
      palmRam[address - RAM_START_ADDRESS + 1] = value & 0xFF;
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      //cant write ROM
      printf("Tried to write 16 bits to ROM at 0x%08X with value of 0x%04X.\n", address, value);
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      setHwRegister16(address - REG_START_ADDRESS, value);
   }
   else if(address >= SED1376_REG_START_ADDRESS && address < SED1376_REG_START_ADDRESS + SED1376_REG_SIZE){
      sed1376SetRegister(address - SED1376_REG_START_ADDRESS, value & 0xFF);
   }
   else{
      //printf("Invalid 16 bit write at 0x%08X with value of 0x%04X.\n", address, value);
   }
}

void m68k_write_memory_32(unsigned int address, unsigned int value){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      palmRam[address - RAM_START_ADDRESS] = value >> 24;
      palmRam[address - RAM_START_ADDRESS + 1] = (value >> 16) & 0xFF;
      palmRam[address - RAM_START_ADDRESS + 2] = (value >> 8) & 0xFF;
      palmRam[address - RAM_START_ADDRESS + 3] = value & 0xFF;
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      //cant write ROM
      printf("Tried to write 32 bits to ROM at 0x%08X with value of 0x%08X.\n", address, value);
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      setHwRegister32(address - REG_START_ADDRESS, value);
   }
   else if(address >= SED1376_REG_START_ADDRESS && address < SED1376_REG_START_ADDRESS + SED1376_REG_SIZE){
      sed1376SetRegister(address - SED1376_REG_START_ADDRESS, value & 0xFF);
   }
   else{
      //printf("Invalid 32 bit write at 0x%08X with value of 0x%08X.\n", address, value);
   }
}

void m68k_write_memory_32_pd(unsigned int address, unsigned int value){
   //swaps the upper and lower 16bits
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      printf("Wrote predecrement swapped 32 bits to RAM at 0x%08X with value of 0x%08X.\n", address, value >> 16 | value << 16);
      palmRam[address - RAM_START_ADDRESS + 2] = value >> 24;
      palmRam[address - RAM_START_ADDRESS + 3] = (value >> 16) & 0xFF;
      palmRam[address - RAM_START_ADDRESS] = (value >> 8) & 0xFF;
      palmRam[address - RAM_START_ADDRESS + 1] = value & 0xFF;
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      //cant write ROM
      printf("Tried to write predecrement swapped 32 bits to ROM at 0x%08X with value of 0x%08X.\n", address, value >> 16 | value << 16);
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      setHwRegister32(address - REG_START_ADDRESS, value >> 16 | value << 16);
      //I dont know the interaction between the hw registers and the malformed opcode
      printf("Possibly fatal error: Predecrement swaped 32bit write to hw register: 0x%08X, at PC: 0x%08X\n", address, m68k_get_reg(NULL, M68K_REG_PC));
   }
   else if(address >= SED1376_REG_START_ADDRESS && address < SED1376_REG_START_ADDRESS + SED1376_REG_SIZE){
      //I dont know the interaction between sed1376 registers and the malformed opcode
      printf("Predecrement swapped 32 bit write at sed1376 register 0x%08X with value of 0x%08X.\n", address - SED1376_REG_START_ADDRESS, value);
   }
   else{
      printf("Invalid predecrement swapped 32 bit write at 0x%08X with value of 0x%08X.\n", address, value);
   }
}


/* Memory access for the disassembler */
unsigned int m68k_read_disassembler_8(unsigned int address){
   return m68k_read_memory_8(address);
}
unsigned int m68k_read_disassembler_16(unsigned int address){
   return m68k_read_memory_16(address);
}
unsigned int m68k_read_disassembler_32(unsigned int address){
   return m68k_read_memory_32(address);
}
