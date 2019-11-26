#include <stdint.h>
#include <string.h>

#include "emulator.h"
#include "portability.h"
#include "dbvz.h"
#include "sed1376.h"
#include "flx68000.h"//for flx68000GetPc()


//the SED1376 has only 16 address lines(17 if you count the line that switches between registers and framebuffer) and 16 data lines, the most you can read at once is 16 bits, registers are 8 bits

//the actions described below are just my best guesses after reading the datasheet, I have not tested with actual hardware
//you read and write the register on the address lines set
//8 bit register access works normal
//16 bit register reads will result in you getting (0x00 << 8 | register)(this is unverified)
//16 bit register writes will result in you writing the lower 8 bits(this is unverified)
//32 bit register reads will result in doing 2 16 bit reads
//32 bit register writes will result in doing 2 16 bit writes

//The LCD power-on sequence is activated by programming the Power Save Mode Enable bit (REG[A0h] bit 0) to 0.
//The LCD power-off sequence is activated by programming the Power Save Mode Enable bit (REG[A0h] bit 0) to 1.


#include "sed1376RegisterNames.c.h"


uint16_t* sed1376Framebuffer;
uint16_t  sed1376FramebufferWidth;
uint16_t  sed1376FramebufferHeight;
uint8_t   sed1376Ram[0x20000];

static uint8_t  sed1376Registers[0xB4];
static uint8_t  sed1376RLut[0x100];
static uint8_t  sed1376GLut[0x100];
static uint8_t  sed1376BLut[0x100];
static uint16_t sed1376OutputLut[0x100];//used to speed up pixel conversion
static uint32_t sed1376ScreenStartAddress;
static uint16_t sed1376LineSize;
static uint16_t (*sed1376RenderPixel)(uint16_t x, uint16_t y);


#include "sed1376Accessors.c.h"

static uint32_t sed1376GetBufferStartAddress(void){
   uint32_t sed1376ScreenStartAddress = sed1376Registers[DISP_ADDR_2] << 16 | sed1376Registers[DISP_ADDR_1] << 8 | sed1376Registers[DISP_ADDR_0];
   switch((sed1376Registers[SPECIAL_EFFECT] & 0x03) * 90){
      case 0:
         //desired byte address / 4.
         sed1376ScreenStartAddress *= 4;
         break;

      case 90:
         //((desired byte address + (panel height * bpp / 8)) / 4) - 1.
         sed1376ScreenStartAddress += 1;
         sed1376ScreenStartAddress *= 4;
         //sed1376ScreenStartAddress - (panelHeight * bpp / 8);
         break;

      case 180:
         //((desired byte address + (panel width * panel height * bpp / 8)) / 4) - 1.
         sed1376ScreenStartAddress += 1;
         sed1376ScreenStartAddress *= 4;
         //sed1376ScreenStartAddress - (panelWidth * panelHeight * bpp / 8);
         break;

      case 270:
         //(desired byte address + ((panel width - 1) * panel height * bpp / 8)) / 4.
         sed1376ScreenStartAddress *= 4;
         //sed1376ScreenStartAddress -= ((panelWidth - 1) * panelHeight * bpp / 8);
         break;
   }

   return sed1376ScreenStartAddress;
}

static uint32_t sed1376GetPipStartAddress(void){
   uint32_t pipStartAddress = sed1376Registers[PIP_ADDR_2] << 16 | sed1376Registers[PIP_ADDR_1] << 8 | sed1376Registers[PIP_ADDR_0];
   switch((sed1376Registers[SPECIAL_EFFECT] & 0x03) * 90){
      case 0:
         //desired byte address / 4.
         pipStartAddress *= 4;
         break;

      case 90:
         //((desired byte address + (panel height * bpp / 8)) / 4) - 1.
         pipStartAddress += 1;
         pipStartAddress *= 4;
         //pipStartAddress - (panelHeight * bpp / 8);
         break;

      case 180:
         //((desired byte address + (panel width * panel height * bpp / 8)) / 4) - 1.
         pipStartAddress += 1;
         pipStartAddress *= 4;
         //pipStartAddress - (panelWidth * panelHeight * bpp / 8);
         break;

      case 270:
         //(desired byte address + ((panel width - 1) * panel height * bpp / 8)) / 4.
         pipStartAddress *= 4;
         //pipStartAddress -= ((panelWidth - 1) * panelHeight * bpp / 8);
         break;
   }

   return pipStartAddress;
}

void sed1376Reset(void){
   memset(sed1376Registers, 0x00, sizeof(sed1376Registers));
   memset(sed1376OutputLut, 0x00, sizeof(sed1376OutputLut));
   memset(sed1376RLut, 0x00, sizeof(sed1376RLut));
   memset(sed1376GLut, 0x00, sizeof(sed1376GLut));
   memset(sed1376BLut, 0x00, sizeof(sed1376BLut));
   memset(sed1376Ram, 0x00, sizeof(sed1376Ram));

   palmMisc.backlightLevel = 0;
   palmMisc.lcdOn = false;

   sed1376RenderPixel = NULL;

   sed1376Registers[REV_CODE] = 0x28;
   sed1376Registers[DISP_BUFF_SIZE] = 0x14;

   //timing hack
   sed1376Registers[PWR_SAVE_CFG] = 0x80;
}

uint32_t sed1376StateSize(void){
   uint32_t size = 0;

   size += sizeof(sed1376Registers);
   size += sizeof(sed1376RLut);
   size += sizeof(sed1376GLut);
   size += sizeof(sed1376BLut);
   size += sizeof(sed1376Ram);

   return size;
}

void sed1376SaveState(uint8_t* data){
   uint32_t offset = 0;

   memcpy(data + offset, sed1376Registers, sizeof(sed1376Registers));
   offset += sizeof(sed1376Registers);
   memcpy(data + offset, sed1376RLut, sizeof(sed1376RLut));
   offset += sizeof(sed1376RLut);
   memcpy(data + offset, sed1376GLut, sizeof(sed1376GLut));
   offset += sizeof(sed1376GLut);
   memcpy(data + offset, sed1376BLut, sizeof(sed1376BLut));
   offset += sizeof(sed1376BLut);
   memcpy(data + offset, sed1376Ram, sizeof(sed1376Ram));
   offset += sizeof(sed1376Ram);
}

void sed1376LoadState(uint8_t* data){
   uint32_t offset = 0;
   uint16_t index;

   memcpy(sed1376Registers, data + offset, sizeof(sed1376Registers));
   offset += sizeof(sed1376Registers);
   memcpy(sed1376RLut, data + offset, sizeof(sed1376RLut));
   offset += sizeof(sed1376RLut);
   memcpy(sed1376GLut, data + offset, sizeof(sed1376GLut));
   offset += sizeof(sed1376GLut);
   memcpy(sed1376BLut, data + offset, sizeof(sed1376BLut));
   offset += sizeof(sed1376BLut);
   memcpy(sed1376Ram, data + offset, sizeof(sed1376Ram));
   offset += sizeof(sed1376Ram);

   //refresh LUT
   MULTITHREAD_LOOP(index) for(index = 0; index < 0x100; index++)
      sed1376OutputLut[index] = makeRgb16FromSed666(sed1376RLut[index], sed1376GLut[index], sed1376BLut[index]);
}

bool sed1376PowerSaveEnabled(void){
   return sed1376Registers[PWR_SAVE_CFG] & 0x01;
}

uint8_t sed1376GetRegister(uint8_t address){
   //returning 0x00 on power save mode is done in the sed1376ReadXX functions
   switch(address){
      case LUT_READ_LOC:
      case LUT_WRITE_LOC:
      case LUT_B_WRITE:
      case LUT_G_WRITE:
      case LUT_R_WRITE:
         //write only
         return 0x00;

      case PWR_SAVE_CFG:
      case SPECIAL_EFFECT:
      case DISP_MODE:
      case LINE_SIZE_0:
      case LINE_SIZE_1:
      case PIP_ADDR_0:
      case PIP_ADDR_1:
      case PIP_ADDR_2:
      case SCRATCH_0:
      case SCRATCH_1:
      case GPIO_CONF_0:
      case GPIO_CONT_0:
      case GPIO_CONF_1:
      case GPIO_CONT_1:
      case MEM_CLK:
      case PIXEL_CLK:
         //simple read, no actions needed
         return sed1376Registers[address];

      default:
         debugLog("SED1376 unknown register read 0x%02X, PC 0x%08X.\n", address, flx68000GetPc());
         return 0x00;
   }
}

void sed1376SetRegister(uint8_t address, uint8_t value){
   switch(address){
      case PWR_SAVE_CFG:
         //bit 7 must always be set, timing hack
         sed1376Registers[address] = (value & 0x01) | 0x80;
         return;

      case DISP_MODE:
         sed1376Registers[address] = value & 0xF7;
         return;

      case PANEL_TYPE:
         sed1376Registers[address] = value & 0xFB;
         return;

      case SPECIAL_EFFECT:
         sed1376Registers[address] = value & 0xD3;
         return;

      case MOD_RATE:
         sed1376Registers[address] = value & 0x3F;
         return;

      case DISP_ADDR_2:
      case PIP_ADDR_2:
         sed1376Registers[address] = value & 0x01;
         return;

      case PWM_CONTROL:
         sed1376Registers[address] = value & 0x9B;
         return;

      case LINE_SIZE_1:
      case PIP_LINE_SZ_1:
      case PIP_X_START_1:
      case PIP_X_END_1:
      case PIP_Y_START_1:
      case PIP_Y_END_1:
      case HORIZ_START_1:
      case VERT_TOTAL_1:
      case VERT_PERIOD_1:
      case VERT_START_1:
      case FPLINE_START_1:
      case FPFRAME_START_1:
         sed1376Registers[address] = value & 0x03;
         return;

      case LUT_WRITE_LOC:
         sed1376BLut[value] = sed1376Registers[LUT_B_WRITE];
         sed1376GLut[value] = sed1376Registers[LUT_G_WRITE];
         sed1376RLut[value] = sed1376Registers[LUT_R_WRITE];
         //whether or not LUT_X_READ are changed on a write when LUT_WRITE_LOC == LUT_READ_LOC is yet to be tested, turn this off for now
         /*
         if(sed1376Registers[LUT_WRITE_LOC] == sed1376Registers[LUT_READ_LOC]){
            sed1376Registers[LUT_B_READ] = sed1376BLut[value];
            sed1376Registers[LUT_G_READ] = sed1376GLut[value];
            sed1376Registers[LUT_R_READ] = sed1376RLut[value];
         }
         */
         sed1376OutputLut[value] = makeRgb16FromSed666(sed1376RLut[value], sed1376GLut[value], sed1376BLut[value]);
         return;

      case LUT_READ_LOC:
         sed1376Registers[LUT_B_READ] = sed1376BLut[value];
         sed1376Registers[LUT_G_READ] = sed1376GLut[value];
         sed1376Registers[LUT_R_READ] = sed1376RLut[value];
         return;

      case GPIO_CONF_0:
      case GPIO_CONT_0:
         sed1376Registers[address] = value & 0x7F;
         sed1376UpdateLcdStatus();
         return;

      case GPIO_CONF_1:
      case GPIO_CONT_1:
         sed1376Registers[address] = value & 0x80;
         return;

      case MEM_CLK:
         sed1376Registers[address] = value & 0x30;
         return;

      case PIXEL_CLK:
         sed1376Registers[address] = value & 0x73;
         return;

      case LUT_B_WRITE:
      case LUT_G_WRITE:
      case LUT_R_WRITE:
         sed1376Registers[address] = value & 0xFC;
         return;

      case HORIZ_TOTAL:
      case HORIZ_PERIOD:
         sed1376Registers[address] = value & 0x7F;
         return;

      case FPFRAME_WIDTH:
         sed1376Registers[address] = value & 0x87;
         return;

      case DTFD_GCP_INDEX:
         sed1376Registers[address] = value & 0x1F;
         return;

      case SCRATCH_0:
      case SCRATCH_1:
      case DISP_ADDR_0:
      case DISP_ADDR_1:
      case PIP_ADDR_0:
      case PIP_ADDR_1:
      case LINE_SIZE_0:
      case PIP_LINE_SZ_0:
      case PIP_X_START_0:
      case PIP_X_END_0:
      case PIP_Y_START_0:
      case PIP_Y_END_0:
      case HORIZ_START_0:
      case VERT_TOTAL_0:
      case VERT_PERIOD_0:
      case VERT_START_0:
      case FPLINE_WIDTH:
      case FPLINE_START_0:
      case FPFRAME_START_0:
      case DTFD_GCP_DATA:
      case PWM_CONFIG:
      case PWM_LENGTH:
      case PWM_DUTY_CYCLE:
         //simple write, no actions needed
         sed1376Registers[address] = value;
         return;

      default:
         debugLog("SED1376 unknown register write, wrote 0x%02X to 0x%02X, PC 0x%08X.\n", value, address, flx68000GetPc());
         return;
   }
}

void sed1376Render(void){
   //render if LCD on, PLL on, power save off and force blank off, SED1376 clock is provided by the CPU, if its off so is the SED
   if(palmMisc.lcdOn && dbvzIsPllOn() && !sed1376PowerSaveEnabled() && !(sed1376Registers[DISP_MODE] & 0x80)){
      bool color = !!(sed1376Registers[PANEL_TYPE] & 0x40);
      bool pictureInPictureEnabled = !!(sed1376Registers[SPECIAL_EFFECT] & 0x10);
      uint8_t bitDepth = 1 << (sed1376Registers[DISP_MODE] & 0x07);
      uint16_t rotation = 90 * (sed1376Registers[SPECIAL_EFFECT] & 0x03);
      uint32_t index;

      sed1376ScreenStartAddress = sed1376GetBufferStartAddress();
      sed1376LineSize = (sed1376Registers[LINE_SIZE_1] << 8 | sed1376Registers[LINE_SIZE_0]) * 4;
      selectRenderer(color, bitDepth);

      if(sed1376RenderPixel){
         uint16_t pixelX;
         uint16_t pixelY;

         MULTITHREAD_DOUBLE_LOOP(pixelX, pixelY) for(pixelY = 0; pixelY < sed1376FramebufferHeight; pixelY++)
            for(pixelX = 0; pixelX < sed1376FramebufferWidth; pixelX++)
               sed1376Framebuffer[pixelY * sed1376FramebufferWidth + pixelX] = sed1376RenderPixel(pixelX, pixelY);

         //debugLog("Screen start address:0x%08X, buffer width:%d, swivel view:%d degrees\n", sed1376ScreenStartAddress, lineSize, rotation);
         //debugLog("Screen format, color:%s, BPP:%d\n", boolString(color), bitDepth);

         if(pictureInPictureEnabled){
            uint16_t pipStartX = sed1376Registers[PIP_X_START_1] << 8 | sed1376Registers[PIP_X_START_0];
            uint16_t pipStartY = sed1376Registers[PIP_Y_START_1] << 8 | sed1376Registers[PIP_Y_START_0];
            uint16_t pipEndX = (sed1376Registers[PIP_X_END_1] << 8 | sed1376Registers[PIP_X_END_0]) + 1;
            uint16_t pipEndY = (sed1376Registers[PIP_Y_END_1] << 8 | sed1376Registers[PIP_Y_END_0]) + 1;

            if(rotation == 0 || rotation == 180){
               pipStartX *= 32 / bitDepth;
               pipEndX *= 32 / bitDepth;
            }
            else{
               pipStartY *= 32 / bitDepth;
               pipEndY *= 32 / bitDepth;
            }
            //debugLog("PIP state, start x:%d, end x:%d, start y:%d, end y:%d\n", pipStartX, pipEndX, pipStartY, pipEndY);
            //render PIP only if PIP window is onscreen
            if(pipStartX < sed1376FramebufferWidth && pipStartY < sed1376FramebufferHeight){
               pipEndX = FAST_MIN(pipEndX, sed1376FramebufferWidth);
               pipEndY = FAST_MIN(pipEndY, sed1376FramebufferHeight);
               sed1376ScreenStartAddress = sed1376GetPipStartAddress();
               sed1376LineSize = (sed1376Registers[PIP_LINE_SZ_1] << 8 | sed1376Registers[PIP_LINE_SZ_0]) * 4;
               MULTITHREAD_DOUBLE_LOOP(pixelX, pixelY) for(pixelY = pipStartY; pixelY < pipEndY; pixelY++)
                  for(pixelX = pipStartX; pixelX < pipEndX; pixelX++)
                     sed1376Framebuffer[pixelY * sed1376FramebufferWidth + pixelX] = sed1376RenderPixel(pixelX, pixelY);
            }
         }

         //rotation
         //later, unemulated

         //display inversion
         if((sed1376Registers[DISP_MODE] & 0x30) == 0x10)
            MULTITHREAD_LOOP(index) for(index = 0; index < sed1376FramebufferWidth * sed1376FramebufferHeight; index++)
               sed1376Framebuffer[index] = ~sed1376Framebuffer[index];
      }
      else{
         debugLog("Invalid screen format, color:%s, BPP:%d, rotation:%d\n", color ? "true" : "false", bitDepth, rotation);
      }
   }
   else{
      //black screen
      memset(sed1376Framebuffer, 0x00, sed1376FramebufferWidth * sed1376FramebufferHeight * sizeof(uint16_t));
      debugLog("Cant draw screen, LCD on:%s, PLL on:%s, power save on:%s, forced blank on:%s\n", palmMisc.lcdOn ? "true" : "false", dbvzIsPllOn() ? "true" : "false", sed1376PowerSaveEnabled() ? "true" : "false", !!(sed1376Registers[DISP_MODE] & 0x80) ? "true" : "false");
   }
}

void sed1376UpdateLcdStatus(void){
   palmMisc.lcdOn = !!(sed1376Registers[GPIO_CONT_0] & sed1376Registers[GPIO_CONF_0] & 0x20);
   palmMisc.backlightLevel = !!(sed1376Registers[GPIO_CONT_0] & sed1376Registers[GPIO_CONF_0] & 0x10) ? (50 + m515BacklightAmplifierState() * 50) : 0;
}
