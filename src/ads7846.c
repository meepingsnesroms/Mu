#include <stdbool.h>

#include "emulator.h"


uint8_t  ads7846InputBitsLeft;
uint8_t  ads7846ControlByte;
bool     ads7846PenIrqEnabled;
uint16_t ads7846OutputValue;


void ads7846SendBit(bool bit){
   //check for control bit
   if(bit && ads7846InputBitsLeft == 0){
      ads7846ControlByte = 0x01;
      ads7846InputBitsLeft = 7;
      return;
   }

   ads7846ControlByte <<= 1;
   ads7846ControlByte |= bit;
   ads7846InputBitsLeft--;

   if(ads7846InputBitsLeft == 0){
      //control byte finished, get output value
      double value;
      double rangeMax;
      bool mode = ads7846ControlByte & 0x08;
      uint8_t powerSave = ads7846ControlByte & 0x03;

      ads7846PenIrqEnabled = !(ads7846ControlByte & 0x01);

      switch(ads7846ControlByte & 0x70){

         case 0x00:
            //temperature 1, unemulated for now
            value = 1;
            rangeMax = 1;
            break;

         case 0x01:
            //touchscreen y
            value = palmInput.touchscreenY;
            rangeMax = 160;
            break;

         case 0x02:
            //battery
            //value = palmMisc.batteryLevel;
            value = 100;
            rangeMax = 100;
            break;

         case 0x03:
         case 0x04:
            //empty slots
            value = 1;
            rangeMax = 1;
            break;

         case 0x05:
            //touchscreen x
            value = palmInput.touchscreenX;
            rangeMax = 160;
            break;

         case 0x06:
            //dock, unemulated for now
            value = 1;
            rangeMax = 1;
            break;

         case 0x07:
            //temperature 2, unemulated for now
            value = 1;
            rangeMax = 1;
            break;
      }

      //check if ADC is on, if not just leave the old value in the register
      if(powerSave != 2)
         ads7846OutputValue = value / rangeMax * (mode ? 0x00FF : 0x0FFF);
   }
}

bool ads7846RecieveBit(){
   bool bit;

   //CPU still sending command, do nothing
   if(ads7846InputBitsLeft != 0)
      return false;

   bit = ads7846OutputValue & 1;
   ads7846OutputValue >>= 1;
   return bit;
}

void ads7846Reset(){
   ads7846OutputValue = 0x0000;
   ads7846ControlByte = 0x00;
   ads7846PenIrqEnabled = false;
   ads7846InputBitsLeft = 0;
}
