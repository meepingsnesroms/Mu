#include <stdint.h>
#include <stdio.h>

#include "emulator.h"
#include "hardwareregisters.h"
#include "m68k/m68k.h"


/* Read from anywhere */
unsigned int  m68k_read_memory_8(unsigned int address){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      return palmRam[address];
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      return palmRom[address];
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      return getHwRegister8(address);
   }
   
   return 0xAA;
}

unsigned int  m68k_read_memory_16(unsigned int address){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      return palmRam[address] << 8 | palmRam[address + 1];
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      return palmRom[address] << 8 | palmRom[address + 1];
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      return getHwRegister16(address);
   }
   
   return 0xAAAA;
}

unsigned int  m68k_read_memory_32(unsigned int address){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      return palmRam[address] << 24 | palmRam[address + 1] << 16 | palmRam[address + 2] << 8 | palmRam[address + 3];
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      return palmRom[address] << 24 | palmRom[address + 1] << 16 | palmRom[address + 2] << 8 | palmRom[address + 3];
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      return getHwRegister32(address);
   }
   
   return 0xAAAAAAAA;
}

/* Write to anywhere */
void m68k_write_memory_8(unsigned int address, unsigned int value){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      palmRam[address] = value;
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      //cant write ROM
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      setHwRegister8(address, value);
   }
}

void m68k_write_memory_16(unsigned int address, unsigned int value){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      palmRam[address] = value >> 8;
      palmRam[address + 1] = value & 0xFF;
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      //cant write ROM
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      setHwRegister16(address, value);
   }
}

void m68k_write_memory_32(unsigned int address, unsigned int value){
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      palmRam[address] = value >> 24;
      palmRam[address + 1] = (value >> 16) & 0xFF;
      palmRam[address + 2] = (value >> 8) & 0xFF;
      palmRam[address + 3] = value & 0xFF;
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      //cant write ROM
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      setHwRegister32(address, value);
   }
}

/*
void m68k_write_memory_32_pd(unsigned int address, unsigned int value){
   //swaps the upper and lower 16bits
   if(address >= RAM_START_ADDRESS && address < RAM_START_ADDRESS + RAM_SIZE){
      palmRam[address + 2] = value >> 24;
      palmRam[address + 3] = (value >> 16) & 0xFF;
      palmRam[address] = (value >> 8) & 0xFF;
      palmRam[address + 1] = value & 0xFF;
   }
   else if(address >= ROM_START_ADDRESS && address < ROM_START_ADDRESS + ROM_SIZE){
      //cant write ROM
   }
   else if(address >= REG_START_ADDRESS && address < REG_START_ADDRESS + REG_SIZE){
      //setHwRegister32(address);
      //I dont know the interaction between the hw registers and the malformed opcode
      printf("Possibly fatal error: Swaped 32bit write to hw register: 0x%08X, at PC: 0x%08X\n", address, m68k_get_reg(NULL, M68K_REG_PC));
      
      
   }
}
*/



/* Memory access for the disassembler */
unsigned int m68k_read_disassembler_8  (unsigned int address){
   return m68k_read_memory_8(address);
}
unsigned int m68k_read_disassembler_16 (unsigned int address){
   return m68k_read_memory_16(address);
}
unsigned int m68k_read_disassembler_32 (unsigned int address){
   return m68k_read_memory_32(address);
}



#if 0

/* Read data immediately following the PC */
unsigned int  m68k_read_immediate_16(unsigned int address){
   return m68k_read_memory_16(address);
}
unsigned int  m68k_read_immediate_32(unsigned int address){
   return m68k_read_memory_32(address);
}

/* Read data relative to the PC */
unsigned int  m68k_read_pcrelative_8(unsigned int address){
   return m68k_read_memory_8(address);
}
unsigned int  m68k_read_pcrelative_16(unsigned int address){
   return m68k_read_memory_16(address);
}
unsigned int  m68k_read_pcrelative_32(unsigned int address){
   return m68k_read_memory_32(address);
}

#endif
