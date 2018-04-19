#include <stdint.h>
#include <string.h>

#include "emulator.h"
#include "portability.h"
#include "hardwareRegisters.h"
#include "memoryAccess.h"
#include "sed1376RegisterNames.h"


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


uint8_t  sed1376Registers[SED1376_REG_SIZE];
uint16_t sed1376Lut[SED1376_LUT_SIZE];
uint8_t  sed1376Framebuffer[SED1376_FB_SIZE];


static inline uint16_t makeRgb16FromRgb666(uint8_t r, uint8_t g, uint8_t b){
   uint16_t color = r << 10 & 0xF800;
   color |= g << 5 & 0x07E0;
   color |= b >> 1 & 0x008F;
   return color;
}

static inline void makeRgb666FromRgb16(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b){
   *r = color >> 10 & 0x3E;
   *g = color >> 5 & 0x3F;
   *b = color << 1 & 0x3E;
}


bool sed1376PowerSaveEnabled(){
   return CAST_TO_BOOL(sed1376Registers[PWR_SAVE_CFG] & 0x01);
}

unsigned int sed1376GetRegister(unsigned int address){
   //returning 0x00 on power save mode is done in the sed1376ReadXX functions
   address -= chips[CHIP_B_SED].start;
   address &= 0x000000FF;
   switch(address){

      default:
         debugLog("SED1376 Register Read from 0x%02X.\n", address);
         return 0x00;
   }
   return 0x00;//for compiler warnings
}

void sed1376SetRegister(unsigned int address, unsigned int value){
   address -= chips[CHIP_B_SED].start;
   address &= 0x000000FF;
   switch(address){

      case PWR_SAVE_CFG:
      case LUT_R_WRITE:
      case LUT_G_WRITE:
      case LUT_B_WRITE:
         //simple write, no actions needed
         sed1376Registers[address] = value;
         break;

      case LUT_WRITE_LOC:
         sed1376Lut[value] = makeRgb16FromRgb666(sed1376Registers[LUT_R_WRITE], sed1376Registers[LUT_G_WRITE], sed1376Registers[LUT_B_WRITE]);
         break;

      default:
         debugLog("SED1376 Register write 0x%02X to 0x%02X.\n", value, address);
         break;
   }
}


void sed1376Reset(){
   memset(sed1376Registers, 0x00, SED1376_REG_SIZE);
   memset(sed1376Lut, 0x00, 0x100);
   memset(sed1376Framebuffer, 0x00, SED1376_FB_SIZE);
   
   sed1376Registers[0x00] = 0x28;//revision code
   sed1376Registers[0x01] = 0x14;//display buffer size
   
}

void sed1376Render(){
   if(palmMisc.lcdOn && palmMisc.backlightOn && cpuIsOn() && !sed1376PowerSaveEnabled()){
      //only render if LCD on and backlight on, SED1376 clock is provided by the CPU, if its off so is the SED

   }
   else{
      //black screen
   }
}
