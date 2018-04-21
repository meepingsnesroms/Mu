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
uint8_t  sed1376RLut[SED1376_LUT_SIZE];
uint8_t  sed1376GLut[SED1376_LUT_SIZE];
uint8_t  sed1376BLut[SED1376_LUT_SIZE];
uint8_t  sed1376Framebuffer[SED1376_FB_SIZE];

static uint16_t sed1376OutputLut[SED1376_LUT_SIZE];//used to speed up pixel conversion

#include "sed1376Accessors.ch"

/*
static inline uint32_t getBufferStartAddress(){
   uint32_t screenStartAddress = sed1376Registers[DISP_ADDR_2] << 16 | sed1376Registers[DISP_ADDR_1] << 8 | sed1376Registers[DISP_ADDR_0];
   switch(sed1376Registers[SPECIAL_EFFECT] & 0x03){

      case 0x00:
         //0 degrees
         //desired byte address / 4.
         screenStartAddress *= 4;
         break;

      case 0x01:
         //90 degrees
         //((desired byte address + (panel height * bpp / 8)) / 4) - 1.
         screenStartAddress += 1;
         screenStartAddress *= 4;
         //screenStartAddress - (panelHeight * bpp / 8);
         break;

      case 0x02:
         //180 degrees
         //((desired byte address + (panel width * panel height * bpp / 8)) / 4) - 1.
         screenStartAddress += 1;
         screenStartAddress *= 4;
         //screenStartAddress - (panelWidth * panelHeight * bpp / 8);
         break;

      case 0x03:
         //270 degrees
         //(desired byte address + ((panel width - 1) * panel height * bpp / 8)) / 4.
         screenStartAddress *= 4;
         //screenStartAddress -= ((panelWidth - 1) * panelHeight * bpp / 8);
         break;
   }

   return screenStartAddress;
}
*/


bool sed1376PowerSaveEnabled(){
   return CAST_TO_BOOL(sed1376Registers[PWR_SAVE_CFG] & 0x01);
}

unsigned int sed1376GetRegister(unsigned int address){
   //returning 0x00 on power save mode is done in the sed1376ReadXX functions
   address -= chips[CHIP_B_SED].start;
   address &= 0x000000FF;
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_ALL)
   debugLog("SED1376 register read from 0x%02X.\n", address);
#endif
   switch(address){

      default:
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_UNKNOWN) && !defined(EMU_LOG_REGISTER_ACCESS_ALL)
         debugLog("SED1376 register read from 0x%02X.\n", address);
#endif
         return 0x00;
   }
   return 0x00;//for compiler warnings
}

void sed1376SetRegister(unsigned int address, unsigned int value){
   address -= chips[CHIP_B_SED].start;
   address &= 0x000000FF;
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_ALL)
   debugLog("SED1376 register write 0x%02X to 0x%02X.\n", value, address);
#endif
   switch(address){

      case PWR_SAVE_CFG:
         sed1376Registers[address] = value & 0x8E;
         break;

      case DISP_MODE:
         sed1376Registers[address] = value & 0xF7;
         break;

      case SPECIAL_EFFECT:
         sed1376Registers[address] = value & 0xD3;
         break;

      case DISP_ADDR_2:
         sed1376Registers[address] = value & 0x01;
         break;

      case LINE_SIZE_1:
         sed1376Registers[address] = value & 0x03;
         break;

      case DISP_ADDR_0:
      case DISP_ADDR_1:
      case LINE_SIZE_0:
      case LUT_R_WRITE:
      case LUT_G_WRITE:
      case LUT_B_WRITE:
         //simple write, no actions needed
         sed1376Registers[address] = value;
         break;

      case LUT_WRITE_LOC:
         sed1376RLut[value] = sed1376Registers[LUT_R_WRITE];
         sed1376GLut[value] = sed1376Registers[LUT_G_WRITE];
         sed1376BLut[value] = sed1376Registers[LUT_B_WRITE];
         sed1376OutputLut[value] = makeRgb16FromRgb666(sed1376Registers[LUT_R_WRITE], sed1376Registers[LUT_G_WRITE], sed1376Registers[LUT_B_WRITE]);
         break;

      case LUT_READ_LOC:
         sed1376Registers[LUT_R_READ] = sed1376RLut[value];
         sed1376Registers[LUT_G_READ] = sed1376GLut[value];
         sed1376Registers[LUT_B_READ] = sed1376BLut[value];
         break;

      default:
#if defined(EMU_DEBUG) && defined(EMU_LOG_REGISTER_ACCESS_UNKNOWN) && !defined(EMU_LOG_REGISTER_ACCESS_ALL)
         debugLog("SED1376 register write 0x%02X to 0x%02X.\n", value, address);
#endif
         break;
   }
}

void sed1376Reset(){
   memset(sed1376Registers, 0x00, SED1376_REG_SIZE);
   memset(sed1376OutputLut, 0x00, SED1376_LUT_SIZE * sizeof(uint16_t));
   memset(sed1376RLut, 0x00, SED1376_LUT_SIZE);
   memset(sed1376GLut, 0x00, SED1376_LUT_SIZE);
   memset(sed1376BLut, 0x00, SED1376_LUT_SIZE);
   memset(sed1376Framebuffer, 0x00, SED1376_FB_SIZE);
   
   sed1376Registers[REV_CODE] = 0x28;
   sed1376Registers[DISP_BUFF_SIZE] = 0x14;
}

void sed1376RefreshLut(){
   for(uint16_t count = 0; count < SED1376_LUT_SIZE; count++)
      sed1376OutputLut[count] = makeRgb16FromRgb666(sed1376RLut[count], sed1376GLut[count], sed1376BLut[count]);
}

void sed1376Render(){
   if(palmMisc.lcdOn && palmMisc.backlightOn && cpuIsOn() && !sed1376PowerSaveEnabled() && !(sed1376Registers[DISP_MODE] & 0x80)){
      //only render if LCD on, backlight on, CPU on, power save off, and force blank off, SED1376 clock is provided by the CPU, if its off so is the SED

   }
   else{
      //black screen
      memset(palmFramebuffer, 0x00, 160 * 160 * sizeof(uint16_t));
   }
}
