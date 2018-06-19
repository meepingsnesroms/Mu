#include <stdbool.h>

#include "emulator.h"
#include "portability.h"


uint8_t  ads7846BitsToNextControl;//bits before starting a new control byte
uint8_t  ads7846ControlByte;
bool     ads7846PenIrqEnabled;
uint16_t ads7846OutputValue;


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
         ads7846BitsToNextControl = 16;
      }
      return ads7846GetAdcBit();
   }
   else if(ads7846BitsToNextControl < 8){
      ads7846ControlByte <<= 1;
      ads7846ControlByte |= bitIn;
   }

   if(ads7846BitsToNextControl == 7){
      //control byte finished, get output value
      double value;
      double rangeMax;
      bool mode = ads7846ControlByte & 0x08;
      uint8_t powerSave = ads7846ControlByte & 0x03;

      ads7846PenIrqEnabled = !(ads7846ControlByte & 0x01);

      switch(ads7846ControlByte & 0x70){
         case 0x00:
            //temperature 1, unemulated for now
            rangeMax = 1;
            value = 1;
            break;

         case 0x10:
            //touchscreen y
            rangeMax = 160;
            if(palmInput.touchscreenTouched)
               value = palmInput.touchscreenY;
            else
               value = 160;
            debugLog("Accessed ADS7846 Touchscreen y.\n");
            break;

         case 0x20:
            //battery
            rangeMax = 100;
            value = palmMisc.batteryLevel;
            break;

         case 0x30:
         case 0x40:
            //empty slots
            rangeMax = 1;
            value = 1;
            break;

         case 0x50:
            //touchscreen x
            rangeMax = 160;
            if(palmInput.touchscreenTouched)
               value = palmInput.touchscreenX;
            else
               value = 160;
            debugLog("Accessed ADS7846 Touchscreen x.\n");
            break;

         case 0x60:
            //dock, unemulated for now
            rangeMax = 1;
            value = 1;
            break;

         case 0x70:
            //temperature 2, unemulated for now
            rangeMax = 1;
            value = 1;
            break;
      }

      //check if ADC is on, if not do nothing, PENIRQ is handled in refreshInputState() in hardwareRegisters.c
      if(powerSave != 0x02){
         if(mode){
            //8 bit conversion
            ads7846OutputValue = value / rangeMax * 0x00FF;
            ads7846OutputValue <<= 8;//move to output position
         }
         else{
            //12 bit conversion
            ads7846OutputValue = value / rangeMax * 0x0FFF;
            ads7846OutputValue <<= 4;//move to output position
         }
      }
   }

   return ads7846GetAdcBit();
}

bool ads7846Busy(){
   if(ads7846BitsToNextControl == 8)
      return true;
   return false;
}

void ads7846Reset(){
   ads7846OutputValue = 0x0000;
   ads7846ControlByte = 0x00;
   ads7846PenIrqEnabled = true;
   ads7846BitsToNextControl = 0;
}
