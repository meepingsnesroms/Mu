#include <stdbool.h>

#include "emulator.h"
#include "portability.h"
#include "m68k/m68k.h"//only used for debugLog


static uint16_t dockResistorValues[PORT_END] = {0xFFF/*none*/, 0x1EB/*USB cradle*/, 0/*serial cradle*/, 0/*USB peripheral*/, 0/*serial peripheral*/};

uint8_t  ads7846BitsToNextControl;//bits before starting a new control byte
uint8_t  ads7846ControlByte;
bool     ads7846PenIrqEnabled;
uint16_t ads7846OutputValue;


static inline double ads7846RangeMap(double oldMin, double oldMax, double value, double newMin, double newMax){
   return (value - oldMin) / (oldMax - oldMin) * (newMax - newMin) + newMin;
}

static inline bool ads7846GetAdcBit(){
   //a new control byte can be sent while receiving data
   //this is valid behavior as long as the start of the last control byte was 16 or more clock cycles ago
   bool bit = ads7846OutputValue & 0x8000;
   ads7846OutputValue <<= 1;
   return bit;
}

bool ads7846ExchangeBit(bool bitIn){
   if(ads7846BitsToNextControl > 0)
      ads7846BitsToNextControl--;

   if(ads7846BitsToNextControl == 0){
      //check for control bit
      if(bitIn){
         ads7846ControlByte = 0x01;
         ads7846BitsToNextControl = 15;
      }
      return ads7846GetAdcBit();
   }
   else if(ads7846BitsToNextControl >= 8){
      ads7846ControlByte <<= 1;
      ads7846ControlByte |= bitIn;
   }
   /*
   else{
      //test, allow starting new control byte early
      if(bitIn){
         debugLog("ADS7846 new control byte starting too early.");
         ads7846ControlByte = 0x01;
         ads7846BitsToNextControl = 15;
      }
      return ads7846GetAdcBit();
   }
   */

   if(ads7846BitsToNextControl == 6){
      //control byte and busy cycle finished, get output value
      bool bitMode = ads7846ControlByte & 0x08;
      bool differentialMode = !(ads7846ControlByte & 0x04);
      uint8_t channel = (ads7846ControlByte & 0x70) >> 4;
      uint8_t powerSave = ads7846ControlByte & 0x03;

      debugLog("Accessed ADS7846 Ch:%d, %s Mode, Power Save:%d, PC:0x%08X.\n", channel, differentialMode ? "Diff" : "Normal", ads7846ControlByte & 0x03, m68k_get_reg(NULL, M68K_REG_PPC));

      //check if ADC is on, PENIRQ is handled in refreshInputState() in hardwareRegisters.c
      switch(powerSave){
         case 0:
         case 1:
            //normal touchscreen operation
            switch(channel){
               case 0:
                  //temperature 0, wrong mode
                  ads7846OutputValue = 0xFFF;
                  break;

               case 1:
                  //touchscreen y
                  if(palmInput.touchscreenTouched)
                     ads7846OutputValue = ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x0EE, 0xEE4);
                  else
                     ads7846OutputValue = 0xFEF;//y is almost fully on when dorment
                  break;

               case 2:
                  //battery, wrong mode
                  ads7846OutputValue = 0xFFF;
                  break;

               case 3:
                  //touchscreen x relative to y
                  ads7846OutputValue = ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x093, 0x600) + ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x000, 0x280);
                  break;

               case 4:
                  //touchscreen y relative to x
                  ads7846OutputValue = ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x9AF, 0xF3F) + ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x000, 0x150);
                  //hack need to scale value of 2nd coord based on where the touchpoint is
                  if(ads7846OutputValue > 0xFFF)
                     ads7846OutputValue = 0xFFF;
                  break;

               case 5:
                  //touchscreen x
                  if(palmInput.touchscreenTouched)
                     ads7846OutputValue = ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x0FD, 0xF47);
                  else
                     ads7846OutputValue = 0x309;
                  break;

               case 6:
                  //dock, wrong mode
                  ads7846OutputValue = 0xFFF;
                  break;

               case 7:
                  //temperature 1, wrong mode, usualy 0xDFF/0xBFF, sometimes 0xFFF
                  ads7846OutputValue = 0xDFF;
                  break;
            }
            break;

         case 2:
            //ADC is off, return invalid data
            if((channel == 3 || channel == 5) && !palmInput.touchscreenTouched)
               ads7846OutputValue = 0x000;
            else
               ads7846OutputValue = 0xFFF;
            break;

         case 3:
            //non touchscreen data
            if(!palmInput.touchscreenTouched){
               switch(channel){
                  case 0:
                     //temperature 0, room temperature
                     ads7846OutputValue = 0x3E2;
                     break;

                  case 1:
                     //touchscreen y, wrong mode
                     ads7846OutputValue = 0xFFF;
                     break;

                  case 2:
                     //battery, unknown hasent gotten low enough to test yet
                     ads7846OutputValue = 0x65C;//~90%
                     //ads7846OutputValue = ads7846RangeMap(0, 100, palmMisc.batteryLevel, 0x0000, 0x07F8);
                     break;

                  case 3:
                     //touchscreen x relative to y, wrong mode
                     ads7846OutputValue = 0x000;
                     break;

                  case 4:
                     //touchscreen y relative to x, wrong mode
                     ads7846OutputValue = 0xFFF;
                     break;

                  case 5:
                     //touchscreen x, wrong mode
                     ads7846OutputValue = 0x000;
                     break;

                  case 6:
                     //dock
                     if(palmMisc.dataPort < PORT_END)
                        ads7846OutputValue = dockResistorValues[palmMisc.dataPort];
                     else
                        ads7846OutputValue = dockResistorValues[PORT_NONE];
                     break;

                  case 7:
                     //temperature 1, room temperature
                     ads7846OutputValue = 0x4A1;
                     break;
               }
            }
            else{
               //crosses lines with REF+(unverified)
               ads7846OutputValue = 0xF80;
            }
            break;
      }

      //move to output position
      ads7846OutputValue <<= 4;

      //if 8 bit conversion, clear extra bits
      if(bitMode)
         ads7846OutputValue &= 0xFF00;

      ads7846PenIrqEnabled = !(powerSave & 0x01);
   }

   return ads7846GetAdcBit();
}

bool ads7846Busy(){
   if(ads7846BitsToNextControl == 7)
      return true;
   return false;
}

void ads7846Reset(){
   ads7846OutputValue = 0x0000;
   ads7846ControlByte = 0x00;
   ads7846PenIrqEnabled = true;
   ads7846BitsToNextControl = 0;
}
