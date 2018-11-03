#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "portability.h"


bool ads7846PenIrqEnabled;

static const uint16_t ads7846DockResistorValues[PORT_END] = {0xFFF/*none*/, 0x1EB/*USB cradle*/, 0/*serial cradle*/, 0/*USB peripheral*/, 0/*serial peripheral*/};
//static const uint8_t ads7846BatteryValues[100];//need to map battery level to voltage
static uint8_t  ads7846BitsToNextControl;//bits before starting a new control byte
static uint8_t  ads7846ControlByte;
static uint16_t ads7846OutputValue;
static bool     ads7846ChipSelect;


static double ads7846RangeMap(double oldMin, double oldMax, double value, double newMin, double newMax){
   return (value - oldMin) / (oldMax - oldMin) * (newMax - newMin) + newMin;
}

static bool ads7846GetAdcBit(){
   bool bit = ads7846OutputValue & 0x8000;
   ads7846OutputValue <<= 1;
   return bit;
}

void ads7846Reset(){
   ads7846BitsToNextControl = 0;
   ads7846ControlByte = 0x00;
   ads7846PenIrqEnabled = true;
   ads7846OutputValue = 0x0000;
   ads7846ChipSelect = false;
}

uint64_t ads7846StateSize(){
   uint64_t size = 0;

   size += sizeof(uint8_t) * 4;
   size += sizeof(uint16_t);

   return size;
}

void ads7846SaveState(uint8_t* data){
   uint64_t offset = 0;

   writeStateValueBool(data + offset, ads7846PenIrqEnabled);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, ads7846BitsToNextControl);
   offset += sizeof(uint8_t);
   writeStateValue8(data + offset, ads7846ControlByte);
   offset += sizeof(uint8_t);
   writeStateValue16(data + offset, ads7846OutputValue);
   offset += sizeof(uint16_t);
   writeStateValueBool(data + offset, ads7846ChipSelect);
   offset += sizeof(uint8_t);
}

void ads7846LoadState(uint8_t* data){
   uint64_t offset = 0;

   ads7846PenIrqEnabled = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
   ads7846BitsToNextControl = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   ads7846ControlByte = readStateValue8(data + offset);
   offset += sizeof(uint8_t);
   ads7846OutputValue = readStateValue16(data + offset);
   offset += sizeof(uint16_t);
   ads7846ChipSelect = readStateValueBool(data + offset);
   offset += sizeof(uint8_t);
}

void ads7846SetChipSelect(bool value){
   //reset the chip when disabled, chip is active when chip select is low
   if(value && !ads7846ChipSelect){
      ads7846BitsToNextControl = 0;
      ads7846ControlByte = 0x00;
      ads7846PenIrqEnabled = true;
      ads7846OutputValue = 0x0000;
   }
}

bool ads7846ExchangeBit(bool bitIn){
   //chip data out is high when off
   if(ads7846ChipSelect)
      return true;

   if(ads7846BitsToNextControl > 0)
      ads7846BitsToNextControl--;

   if(ads7846BitsToNextControl == 0){
      //check for control bit
      //a new control byte can be sent while receiving data
      //this is valid behavior as long as the start of the last control byte was 16 or more clock cycles ago
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
   else if(ads7846BitsToNextControl == 6){
      //control byte and busy cycle finished, get output value
      bool bitMode = ads7846ControlByte & 0x08;
      //bool differentialMode = !(ads7846ControlByte & 0x04);
      uint8_t channel = (ads7846ControlByte & 0x70) >> 4;
      uint8_t powerSave = ads7846ControlByte & 0x03;

      //debugLog("Accessed ADS7846 Ch:%d, %d bits, %s Mode, Power Save:%d, PC:0x%08X.\n", channel, bitMode ? 8 : 12, differentialMode ? "Diff" : "Normal", ads7846ControlByte & 0x03, flx68000GetPc());

      //check if ADC is on, PENIRQ is handled in refreshInputState() in hardwareRegisters.c
      switch(powerSave){
         case 0:
         case 1:
            //touchscreen data only
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
                  if(palmInput.touchscreenTouched)
                      ads7846OutputValue = ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x093, 0x600) + ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x000, 0x280);
                  else
                     ads7846OutputValue = 0x000;
                  break;

               case 4:
                  //touchscreen y relative to x
                  if(palmInput.touchscreenTouched)
                      ads7846OutputValue = ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x9AF, 0xF3F) + ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x000, 0x150);
                  else
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
            //all data
            if(!palmInput.touchscreenTouched){
               switch(channel){
                  case 0:
                     //temperature 0, room temperature
                     ads7846OutputValue = 0x3E2;
                     break;

                  case 1:
                     //touchscreen y
                     if(palmInput.touchscreenTouched)
                        ads7846OutputValue = ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x0EE, 0xEE4);
                     else
                        ads7846OutputValue = 0xFFF;//y is almost fully on when dorment
                     break;

                  case 2:
                     //battery, unknown hasent gotten low enough to test yet
                     //ads7846OutputValue = 0x600;//5%
                     //ads7846OutputValue = 0x61C;//30%
                     //ads7846OutputValue = 0x63C;//40%
                     //ads7846OutputValue = 0x65C;//60%
                     //ads7846OutputValue = 0x67C;//80%
                     //ads7846OutputValue = 0x68C;//100%
                     ads7846OutputValue = 0x69C;//100%
                     //ads7846OutputValue = ads7846RangeMap(0, 100, palmMisc.batteryLevel, 0x0000, 0x07F8);
                     break;

                  case 3:
                     //touchscreen x relative to y
                     if(palmInput.touchscreenTouched)
                        ads7846OutputValue = ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x093, 0x600) + ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x000, 0x280);
                     else
                        ads7846OutputValue = 0x000;
                     break;

                  case 4:
                     //touchscreen y relative to x
                     if(palmInput.touchscreenTouched)
                        ads7846OutputValue = ads7846RangeMap(0, 219, 219 - palmInput.touchscreenY, 0x9AF, 0xF3F) + ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x000, 0x150);
                     else
                        ads7846OutputValue = 0xFFF;
                     break;

                  case 5:
                     //touchscreen x
                     if(palmInput.touchscreenTouched)
                        ads7846OutputValue = ads7846RangeMap(0, 159, 159 - palmInput.touchscreenX, 0x0FD, 0xF47);
                     else
                        ads7846OutputValue = 0x3FB;
                     break;

                  case 6:
                     //dock
                     if(palmMisc.dataPort < PORT_END)
                        ads7846OutputValue = ads7846DockResistorValues[palmMisc.dataPort];
                     else
                        ads7846OutputValue = ads7846DockResistorValues[PORT_NONE];
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

      //if 8 bit conversion, clear extra bits and shorten conversion by 4 bits
      if(bitMode){
         ads7846OutputValue &= 0xFF00;
         ads7846BitsToNextControl -= 4;
      }

      ads7846PenIrqEnabled = !(powerSave & 0x01);
   }

   return ads7846GetAdcBit();
}

bool ads7846Busy(){
   if(ads7846BitsToNextControl == 7)
      return true;
   return false;
}
