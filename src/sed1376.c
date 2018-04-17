#include <stdint.h>
#include <string.h>

#include "emulator.h"
#include "hardwareRegisters.h"


//the SED1376 has only 16 address lines(17 if you count the line that switches between registers and framebuffer) and 16 data lines, the most you can read is 16 bits, registers are 8 bits

//the actions described below are just my best guesses after reading the datasheet, I have not tested with actual hardware
//you read and write the register on the address lines set
//8 bit register access works normal
//16 bit register reads will result in you getting (0x00 << 8 | register).
//16 bit register writes will result in you writing the lower 8 bits.
//32 bit register reads will result in you getting (randomUint16 << 16 | 0x00 << 8 | register), upper 16 bits are floating because SED1376 only has 16 address lines.
//32 bit register writes will result in you writing the lower 8 bits.

//The LCD power-on sequence is activated by programming the Power Save Mode Enable bit (REG[A0h] bit 0) to 0.
//The LCD power-off sequence is activated by programming the Power Save Mode Enable bit (REG[A0h] bit 0) to 1.


uint8_t sed1376Registers[SED1376_REG_SIZE];
uint8_t sed1376Framebuffer[SED1376_FB_SIZE];


unsigned int sed1376GetRegister(unsigned int address){
   debugLog("SED1376 Register Read from 0x%02X.\n", address);
   return sed1376Registers[address];
}

void sed1376SetRegister(unsigned int address, unsigned int value){
   debugLog("SED1376 Register write 0x%02X to 0x%02X.\n", value, address);
   sed1376Registers[address] = value;
}


void sed1376Reset(){
   memset(sed1376Registers, 0x00, SED1376_REG_SIZE);
   memset(sed1376Framebuffer, 0x00, SED1376_FB_SIZE);
   
   sed1376Registers[0x00] = 0x28;//revision code
   sed1376Registers[0x01] = 0x14;//display buffer size
   
}

void sed1376Render(){
   if(palmMisc.lcdOn && palmMisc.backlightOn && cpuIsOn()){
      //only render if LCD on and backlight on, SED1376 clock is provided by the CPU, if its off so is the SED

   }
   else{
      //black screen
   }
}
