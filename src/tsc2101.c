#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"


#define TSC2101_REG_LOCATION(page, address) ((page) << 6 | (address))

enum{
   TOUCH_DATA_X = TSC2101_REG_LOCATION(0, 0x00),
   TOUCH_DATA_Y,
   TOUCH_DATA_Z1,
   TOUCH_DATA_Z2,
   TOUCH_DATA_RESERVED_0,
   TOUCH_DATA_BAT,
   TOUCH_DATA_RESERVED_1,
   TOUCH_DATA_AUX1,
   TOUCH_DATA_AUX2,
   TOUCH_DATA_TEMP1,
   TOUCH_DATA_TEMP2,
   //TOUCH_DATA_RESERVED_X...
   TOUCH_CONTROL_TSC_ADC = TSC2101_REG_LOCATION(1, 0x00),
   TOUCH_CONTROL_STATUS,
   TOUCH_CONTROL_BUFFER_MODE,
   TOUCH_CONTROL_REFERENCE,
   TOUCH_CONTROL_RESET_CONTROL_REGISTER,
   TOUCH_CONTROL_CONFIGURATION,
   TOUCH_CONTROL_TEMPERATURE_MAX,
   TOUCH_CONTROL_TEMPERATURE_MIN,
   TOUCH_CONTROL_AUX1_MAX,
   TOUCH_CONTROL_AUX1_MIN,
   TOUCH_CONTROL_AUX2_MAX,
   TOUCH_CONTROL_AUX2_MIN,
   TOUCH_CONTROL_MEASUREMENT_CONFIGURATION,
   TOUCH_CONTROL_PROGRAMMABLE_DELAY
   //TOUCH_CONTROL_RESERVED_X...
   //TODO: add audio control registers
};


static uint16_t tsc2101Registers[0x100];
static uint16_t tsc2101CurrentWord;
static uint8_t  tsc2101CurrentWordBitsRemaining;
static uint8_t  tsc2101CurrentPage;
static uint8_t  tsc2101CurrentRegister;
static bool     tsc2101CommandFinished;
static bool     tsc2101Read;
static bool     tsc2101ChipSelect;


static uint16_t tsc2101RegisterRead(uint8_t page, uint8_t address){
   uint8_t combinedRegisterNumber = TSC2101_REG_LOCATION(page, address);

   switch(combinedRegisterNumber){
      case TOUCH_CONTROL_STATUS:
         //simple read, no actions needed
         //return tsc2101Registers[combinedRegisterNumber];

      default:
         debugLog("Unimplemented TSC2101 register read, page:0x%01X, address:0x%02X\n", page, address);
         return 0x0000;//TODO: this may need to be 0xFFFF
   }
}

static void tsc2101RegisterWrite(uint8_t page, uint8_t address, uint16_t value){
   uint8_t combinedRegisterNumber = TSC2101_REG_LOCATION(page, address);

   switch(combinedRegisterNumber){

      default:
         debugLog("Unimplemented TSC2101 register write, page:0x%01X, address:0x%02X, value:0x%04X\n", page, address, value);
         return;
   }
}

void tsc2101Reset(void){
   memset(tsc2101Registers, 0x00, sizeof(tsc2101Registers));
   tsc2101CurrentWord = 0x0000;
   tsc2101CurrentWordBitsRemaining = 16;
   tsc2101CurrentPage = 0;
   tsc2101CurrentRegister = 0;
   tsc2101CommandFinished = false;
   tsc2101Read = false;
   tsc2101ChipSelect = true;

   //TODO: need to add all the registers here
   tsc2101Registers[TOUCH_CONTROL_STATUS] = 0x8000;
}

uint32_t tsc2101StateSize(void){

}

void tsc2101SaveState(uint8_t* data){

}

void tsc2101LoadState(uint8_t* data){

}

void tsc2101SetChipSelect(bool value){
   if(value && !tsc2101ChipSelect){
      tsc2101CurrentWordBitsRemaining = 16;
      tsc2101Read = false;
   }
   tsc2101ChipSelect = value;
}

bool tsc2101ExchangeBit(bool bit){
   bool output = true;//TODO: SPI return value is usualy true but this is unverified

   if(!tsc2101Read){
      tsc2101CurrentWord <<= 1;
      tsc2101CurrentWord |= bit;
   }
   else{
      output = !!(tsc2101CurrentWord & 0x8000);
      tsc2101CurrentWord <<= 1;
   }
   tsc2101CurrentWordBitsRemaining--;

   if(tsc2101CurrentWordBitsRemaining == 0){
      if(!tsc2101CommandFinished){
         //write command word
         tsc2101CurrentPage = tsc2101CurrentWord >> 11 & 0x000F;
         tsc2101CurrentRegister = tsc2101CurrentWord >> 5 & 0x003F;
         tsc2101Read = !!(tsc2101CurrentWord & 0x8000);
         if(tsc2101Read){
            //add first data word
            tsc2101CurrentWord = tsc2101RegisterRead(tsc2101CurrentPage, tsc2101CurrentRegister);
            tsc2101CurrentRegister++;
         }
      }
      else if(!tsc2101Read){
         //write data word
         if(tsc2101CurrentRegister < 0x40){
            tsc2101RegisterWrite(tsc2101CurrentPage, tsc2101CurrentRegister, tsc2101CurrentWord);
            tsc2101CurrentRegister++;
         }
      }
      else{
         //read data word
         if(tsc2101CurrentRegister < 0x40){
            tsc2101CurrentWord = tsc2101RegisterRead(tsc2101CurrentPage, tsc2101CurrentRegister);
            tsc2101CurrentRegister++;
         }
         else{
            tsc2101CurrentWord = 0xFFFF;
         }
      }

      tsc2101CurrentWordBitsRemaining = 16;
   }

   return output;
}
