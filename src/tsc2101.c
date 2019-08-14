#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"


uint16_t tsc2101CurrentWord;
uint8_t  tsc2101CurrentWordBitsRemaining;
uint8_t  tsc2101CurrentPage;
uint8_t  tsc2101CurrentRegister;
bool     tsc2101CommandFinished;
bool     tsc2101Read;


static uint16_t tsc2101RegisterRead(uint8_t page, uint8_t address){
   debugLog("Unimplemented TSC2101 register read, page:0x%01X, address:0x%02X\n", page, address);
}

static void tsc2101RegisterWrite(uint8_t page, uint8_t address, uint16_t value){
   debugLog("Unimplemented TSC2101 register write, page:0x%01X, address:0x%02X, value:0x%04X\n", page, address, value);
}

void tsc2101Reset(void){
   tsc2101CurrentWord = 0x0000;
   tsc2101CurrentWordBitsRemaining = 16;
   tsc2101CurrentPage = 0;
   tsc2101CurrentRegister = 0;
   tsc2101CommandFinished = false;
   tsc2101Read = false;
}

uint32_t tsc2101StateSize(void){

}

void tsc2101SaveState(uint8_t* data){

}

void tsc2101LoadState(uint8_t* data){

}

void tsc2101SetChipSelect(bool value){
   tsc2101CurrentWordBitsRemaining = 16;
   tsc2101Read = false;
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
