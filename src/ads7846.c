#include <stdbool.h>

#include "emulator.h"
#include "portability.h"


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

   if(ads7846BitsToNextControl == 6){
      //control byte and busy cycle finished, get output value
      bool mode = ads7846ControlByte & 0x08;
      bool differentialMode = !(ads7846ControlByte & 0x04);
      uint8_t powerSave = ads7846ControlByte & 0x03;

      //check if ADC is on, if not do nothing, PENIRQ is handled in refreshInputState() in hardwareRegisters.c
      if(powerSave != 0x02){
         if(differentialMode){
            //differential mode, this is whats used for touchscreen positions
            switch(ads7846ControlByte & 0x70){
               case 0x00:
                  //temperature 0, wrong mode
                  ads7846OutputValue = 0x07F8;
                  break;

               case 0x10:
                  //touchscreen y
                  if(palmInput.touchscreenTouched)
                     ads7846OutputValue = ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x00C8, 0x03B8);
                  else
                     ads7846OutputValue = 0x07D8;//y is 0x07D8 when dorment
                  debugLog("Accessed ADS7846 Touchscreen Y.\n");
                  break;

               case 0x20:
                  //battery, unknown hasent gotten low enough to test yet
                  ads7846OutputValue = 0x07F8;
                  //ads7846OutputValue = ads7846RangeMap(0, 100, palmMisc.batteryLevel, 0x0000, 0x07F8);
                  debugLog("Accessed Batt, Diff Mode.\n");
                  break;

               case 0x30:
                  //3 seems to be a duplicate of touchscreen x with a smaller range
                  ads7846OutputValue = 0x07F8;
                  debugLog("Accessed ADS7846 Unknown Slot 3, Diff Mode.\n");
                  break;

               case 0x40:
                  //4 seems to be a duplicate of touchscreen y with a smaller range and higher start point
                  ads7846OutputValue = 0x07F8;
                  debugLog("Accessed ADS7846 Empty Slot 4, Diff Mode.\n");
                  break;

               case 0x50:
                  //touchscreen x
                  if(palmInput.touchscreenTouched)
                     ads7846OutputValue = ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x00C0, 0x04B0);
                  else
                     ads7846OutputValue = 0x0000;
                  debugLog("Accessed ADS7846 Touchscreen X.\n");
                  break;

               case 0x60:
                  //dock, wrong mode
                  ads7846OutputValue = 0x07F8;
                  break;

               case 0x70:
                  //temperature 1, wrong mode
                  ads7846OutputValue = 0x07F8;
                  break;
            }
         }
         else{
            //not differential mode, used for battery and dock
            if(palmInput.touchscreenTouched){
               //when screen touched all values are 0x07C1 in this mode
               ads7846OutputValue = 0x07C1;
            }
            else{
               switch(ads7846ControlByte & 0x70){
                  case 0x00:
                     //temperature 0, unemulated for now
                     ads7846OutputValue = 0x05C3;
                     //debugLog("Accessed Temp 0.\n");
                     break;

                  case 0x10:
                     //touchscreen y, wrong mode
                     ads7846OutputValue = 0x07C3;
                     break;

                  case 0x20:
                     //battery, unknown hasent gotten low enough to test yet
                     ads7846OutputValue = 0x07C3;
                     debugLog("Accessed Batt, Non Diff Mode.\n");
                     break;

                  case 0x30:
                     //empty slot
                     ads7846OutputValue = 0x05C0;
                     break;

                  case 0x40:
                     //empty slot
                     ads7846OutputValue = 0x07C3;
                     break;

                  case 0x50:
                     //touchscreen x, wrong mode
                     ads7846OutputValue = 0x0002;
                     break;

                  case 0x60:
                     //dock, unemulated for now, changes depending on the type of dock attached 0x07C3 in unplugged
                     ads7846OutputValue = 0x07C3;
                     //debugLog("Accessed Dock.\n");
                     break;

                  case 0x70:
                     //temperature 1, unemulated for now
                     ads7846OutputValue = 0x06C3;
                     debugLog("Accessed Temp 1.\n");
                     break;
               }
            }
         }


         ads7846OutputValue <<= 4;//move to output position

         //if 8 bit conversion, clear extra bits
         if(mode)
            ads7846OutputValue &= 0xFF00;
      }
      else{
         //nothing in register
         ads7846OutputValue = 0x0000;
      }

      ads7846PenIrqEnabled = powerSave != 0x01;
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
