#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "pxa260/pxa260Timing.h"
#include "tsc2101.h"


//TODO: using GPIO1 as an interrupt dosent work
//TODO: self controlled scanning mode is unimplemented
//TODO: single shot/continuos modes
//TODO: HCTLM is wrong

#define TSC2101_SCAN_DURATION 10000//in PXA260 opcodes//TODO: this may need to be calculated based on the timing params in the registers
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
   TOUCH_CONTROL_PROGRAMMABLE_DELAY,
   //TOUCH_CONTROL_RESERVED_X...
   //TODO: add audio control registers
   BUFFER_START = TSC2101_REG_LOCATION(3, 0),
   BUFFER_END = TSC2101_REG_LOCATION(3, 63)
};


static uint16_t tsc2101Registers[0x100];
static uint8_t  tsc2101BufferReadPosition;
static uint8_t  tsc2101BufferWritePosition;
static uint16_t tsc2101HasNewData;
static uint16_t tsc2101CurrentWord;
static uint8_t  tsc2101CurrentWordBitsRemaining;
static uint8_t  tsc2101CurrentPage;
static uint8_t  tsc2101CurrentRegister;
static bool     tsc2101CommandFinished;
static bool     tsc2101Read;
static bool     tsc2101ChipSelect;


static uint8_t tsc2101BufferFifoEntrys(void){
   //check for wraparound
   if(tsc2101BufferWritePosition < tsc2101BufferReadPosition)
      return tsc2101BufferWritePosition + 17 - tsc2101BufferReadPosition;
   return tsc2101BufferWritePosition - tsc2101BufferReadPosition;
}

static uint16_t tsc2101BufferFifoRead(void){
   uint16_t value = 0x0000;

   //FUF flag
   if(tsc2101BufferFifoEntrys() == 64)
      value |= 0x8000;

   //get data
   if(tsc2101BufferFifoEntrys() > 0)
      tsc2101BufferReadPosition = (tsc2101BufferReadPosition + 1) % 65;

   value |= tsc2101Registers[BUFFER_START + tsc2101BufferReadPosition];

   //EMF flag
   if(tsc2101BufferFifoEntrys() == 0)
      value |= 0x4000;

   return value;
}

static void tsc2101BufferFifoWrite(uint8_t who){
   uint16_t value = tsc2101Registers[who];

   switch(who){
      case TOUCH_DATA_X:
      case TOUCH_DATA_Z1:
      case TOUCH_DATA_BAT:
      case TOUCH_DATA_AUX2:
         //nothing, bit 12 is 0
         break;

      case TOUCH_DATA_Y:
      case TOUCH_DATA_Z2:
      case TOUCH_DATA_AUX1:
         value |= 0x1000;
         break;

      default:
         debugLog("TSC2101 invalid data register written to buffer:%d\n", who);
         break;
   }

   if(tsc2101BufferFifoEntrys() < 64){
      tsc2101BufferWritePosition = (tsc2101BufferWritePosition + 1) % 65;
      tsc2101Registers[BUFFER_START + tsc2101BufferWritePosition] = value;
   }
   else{
      debugLog("TSC2101 buffer FIFO overflowed\n");
   }
}

static void tsc2101BufferFifoFlush(void){
   tsc2101BufferReadPosition = tsc2101BufferWritePosition;
}

static uint16_t tsc2101GetAnalogMask(void){
   const uint16_t masks[4] = {0xFFF, 0xFF0, 0xFFC, 0xFFF};

   return masks[tsc2101Registers[TOUCH_CONTROL_TSC_ADC] >> 8 & 0x0003];
}

static uint16_t tsc2101GetXValue(void){
   //TODO: get fake touch range values
   return (uint16_t)(palmInput.touchscreenX * 0xFFF) & tsc2101GetAnalogMask();
}

static uint16_t tsc2101GetYValue(void){
   //TODO: get fake touch range values
   return (uint16_t)(palmInput.touchscreenY * 0xFFF) & tsc2101GetAnalogMask();
}

static uint16_t tsc2101GetZ1Value(void){
   //TODO: get fake touch range values
   return 0x777 & tsc2101GetAnalogMask();
}

static uint16_t tsc2101GetZ2Value(void){
   //TODO: get fake touch range values
   return 0x777 & tsc2101GetAnalogMask();
}

static uint16_t tsc2101GetBatValue(void){
   return 0x777 & tsc2101GetAnalogMask();
}

static uint16_t tsc2101GetAux1Value(void){
   if(tsc2101Registers[TOUCH_CONTROL_MEASUREMENT_CONFIGURATION] & 0x4000)
      return 0x777 & tsc2101GetAnalogMask();//resistance
   return 0x777 & tsc2101GetAnalogMask();//voltage
}

static uint16_t tsc2101GetAux2Value(void){
   if(tsc2101Registers[TOUCH_CONTROL_MEASUREMENT_CONFIGURATION] & 0x2000)
      return 0x777 & tsc2101GetAnalogMask();//resistance
   return 0x777 & tsc2101GetAnalogMask();//voltage
}

static uint16_t tsc2101GetTemp1Value(void){
   return 0x777 & tsc2101GetAnalogMask();
}

static uint16_t tsc2101GetTemp2Value(void){
   return 0x777 & tsc2101GetAnalogMask();
}

static void tsc2101ResetRegisters(void){
   tsc2101HasNewData = 0x0000;

   memset(tsc2101Registers, 0x00, sizeof(tsc2101Registers));

   //TODO: need to add all the registers here
   tsc2101Registers[TOUCH_CONTROL_STATUS] = 0x8000;
   tsc2101Registers[TOUCH_CONTROL_REFERENCE] = 0x0002;
   //currenly at register 0x5 of touch control

   tsc2101RefreshInterrupt();
}

static uint16_t tsc2101RegisterRead(uint8_t page, uint8_t address){
   uint8_t combinedRegisterNumber = TSC2101_REG_LOCATION(page, address);

   switch(combinedRegisterNumber){
      case TOUCH_CONTROL_TSC_ADC:
         return tsc2101Registers[TOUCH_CONTROL_TSC_ADC] & 0x3FFF | palmInput.touchscreenTouched << 15 | 1 << 14/*TODO: this states the ADC is never busy*/;

      case TOUCH_CONTROL_STATUS:{
            uint16_t value = tsc2101Registers[TOUCH_CONTROL_STATUS] & 0xF000;

            value |= !!(tsc2101HasNewData) << 10;
            //TODO: not returning individual status bits yet

            debugLog("TSC2101 read status register\n");
            return value;
         }

      case TOUCH_CONTROL_BUFFER_MODE:
         return tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] | (tsc2101BufferFifoEntrys() == 64) << 10 | (tsc2101BufferFifoEntrys() == 0) << 9;

      case TOUCH_CONTROL_RESET_CONTROL_REGISTER:
         return 0xFFFF;

      //TOUCH_DATA_* registers are scaned, the values get copyed to the register when the result is generated
      case TOUCH_DATA_X:
      case TOUCH_DATA_Y:
      case TOUCH_DATA_Z1:
      case TOUCH_DATA_Z2:
      case TOUCH_DATA_BAT:
      case TOUCH_DATA_AUX1:
      case TOUCH_DATA_AUX2:
      case TOUCH_DATA_TEMP1:
      case TOUCH_DATA_TEMP2:
         tsc2101HasNewData &= ~(1 << combinedRegisterNumber);
         return tsc2101Registers[combinedRegisterNumber];

      default:
         if(page == 3)
            return tsc2101BufferFifoRead();
         debugLog("Unimplemented TSC2101 register read, page:0x%01X, address:0x%02X\n", page, address);
         return 0x0000;//TODO: this may need to be 0xFFFF
   }
}

static void tsc2101RegisterWrite(uint8_t page, uint8_t address, uint16_t value){
   uint8_t combinedRegisterNumber = TSC2101_REG_LOCATION(page, address);

   switch(combinedRegisterNumber){
      case TOUCH_CONTROL_TSC_ADC:
         tsc2101Registers[TOUCH_CONTROL_TSC_ADC] = value;
         pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_TSC2101_SCAN, TSC2101_SCAN_DURATION);
         return;

      case TOUCH_CONTROL_STATUS:
         tsc2101Registers[TOUCH_CONTROL_STATUS] = value & 0xC000;
         tsc2101RefreshInterrupt();//interrupt type might have changed
         return;

      case TOUCH_CONTROL_BUFFER_MODE:
         if((tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000) && !(value & 0x8000))
            tsc2101BufferFifoFlush();

         tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] = value & 0xF800;
         tsc2101RefreshInterrupt();//need to refresh innterupt because the trigger level may have changed
         return;

      case TOUCH_CONTROL_REFERENCE:
         //TODO: may need to divide the values by the referece voltage then multiply by 0xFFF
         tsc2101Registers[TOUCH_CONTROL_REFERENCE] = value & 0x001F;
         return;

      case TOUCH_CONTROL_RESET_CONTROL_REGISTER:
         if(value == 0xBB00)
            tsc2101ResetRegisters();
         return;

      case TOUCH_CONTROL_CONFIGURATION:
         //TODO: TOUCH_CONTROL_CONFIGURATION SWPDTD bit
         debugLog("TSC2101 config register writes not fully implemented\n");
         tsc2101Registers[TOUCH_CONTROL_CONFIGURATION] = value & 0x007F;
         tsc2101RefreshInterrupt();
         return;

      case TOUCH_CONTROL_TEMPERATURE_MAX:
      case TOUCH_CONTROL_TEMPERATURE_MIN:
      case TOUCH_CONTROL_AUX1_MAX:
      case TOUCH_CONTROL_AUX1_MIN:
      case TOUCH_CONTROL_AUX2_MAX:
      case TOUCH_CONTROL_AUX2_MIN:
         tsc2101Registers[combinedRegisterNumber] = value & 0x1FFF;
         tsc2101RefreshInterrupt();
         return;

      case TOUCH_CONTROL_MEASUREMENT_CONFIGURATION:
         tsc2101Registers[TOUCH_CONTROL_MEASUREMENT_CONFIGURATION] = value & 0xFE04;
         tsc2101RefreshInterrupt();
         return;

      case TOUCH_DATA_X:
      case TOUCH_DATA_Y:
      case TOUCH_DATA_Z1:
      case TOUCH_DATA_Z2:
      case TOUCH_DATA_BAT:
      case TOUCH_DATA_AUX1:
      case TOUCH_DATA_AUX2:
      case TOUCH_DATA_TEMP1:
      case TOUCH_DATA_TEMP2:
         //invalid write, ignore
         return;

      case TOUCH_CONTROL_PROGRAMMABLE_DELAY:
         //simple write, no actions needed
         tsc2101Registers[combinedRegisterNumber] = value;
         return;

      default:
         if(page == 3)
            debugLog("TSC2101 attempted to write buffer register:0x%02X, value:0x%04X\n", address, value);
         debugLog("Unimplemented TSC2101 register write, page:0x%01X, address:0x%02X, value:0x%04X\n", page, address, value);
         return;
   }
}

void tsc2101Reset(bool isBoot){
   tsc2101BufferReadPosition = 0;
   tsc2101BufferWritePosition = 0;
   tsc2101CurrentWord = 0x0000;
   tsc2101CurrentWordBitsRemaining = 16;
   tsc2101CurrentPage = 0;
   tsc2101CurrentRegister = 0;
   tsc2101CommandFinished = false;
   tsc2101Read = false;
   pxa260TimingCancelEvent(PXA260_TIMING_CALLBACK_TSC2101_SCAN);

   if(isBoot)
      tsc2101ChipSelect = true;

   tsc2101ResetRegisters();
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
      tsc2101CommandFinished = false;
      tsc2101Registers[TOUCH_CONTROL_TSC_ADC] &= 0xC3FF;//TODO: treating this as "stop bit", may be wrong
      pxa260TimingCancelEvent(PXA260_TIMING_CALLBACK_TSC2101_SCAN);
   }
   tsc2101ChipSelect = value;
}

bool tsc2101ExchangeBit(bool bit){
   bool output = true;//TODO: SPI return value is usualy true but this is unverified

   if(tsc2101ChipSelect)
      return true;

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
         tsc2101CommandFinished = true;
         if(tsc2101Read){
            //add first data word
            tsc2101CurrentWord = tsc2101RegisterRead(tsc2101CurrentPage, tsc2101CurrentRegister);
            tsc2101CurrentRegister++;
         }
      }
      else if(!tsc2101Read){
         //write data word
         //debugLog("TSC2101 should write now!\n");
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

void tsc2101RefreshInterrupt(void){
   debugLog("TSC2101 PINTDAV not fully implemented\n");

   //check if PINTDAV is data or pen and data interrupt
   if(tsc2101Registers[TOUCH_CONTROL_STATUS] >> 14 & 0x0003){
      if(tsc2101Registers[TOUCH_CONTROL_TEMPERATURE_MAX] & 0x1000 && ((tsc2101Registers[TOUCH_CONTROL_MEASUREMENT_CONFIGURATION] & 0x8000) ? tsc2101Registers[TOUCH_DATA_TEMP2] : tsc2101Registers[TOUCH_DATA_TEMP1]) >= (tsc2101Registers[TOUCH_CONTROL_TEMPERATURE_MAX] & 0xFFF))
         goto trigger;

      if(tsc2101Registers[TOUCH_CONTROL_TEMPERATURE_MIN] & 0x1000 && ((tsc2101Registers[TOUCH_CONTROL_MEASUREMENT_CONFIGURATION] & 0x8000) ? tsc2101Registers[TOUCH_DATA_TEMP2] : tsc2101Registers[TOUCH_DATA_TEMP1]) <= (tsc2101Registers[TOUCH_CONTROL_TEMPERATURE_MIN] & 0xFFF))
         goto trigger;

      if(tsc2101Registers[TOUCH_CONTROL_AUX1_MAX] & 0x1000 && tsc2101Registers[TOUCH_DATA_AUX1] >= (tsc2101Registers[TOUCH_CONTROL_AUX1_MAX] & 0xFFF))
         goto trigger;

      if(tsc2101Registers[TOUCH_CONTROL_AUX1_MIN] & 0x1000 && tsc2101Registers[TOUCH_DATA_AUX1] <= (tsc2101Registers[TOUCH_CONTROL_AUX1_MIN] & 0xFFF))
         goto trigger;

      if(tsc2101Registers[TOUCH_CONTROL_AUX2_MAX] & 0x1000 && tsc2101Registers[TOUCH_DATA_AUX2] >= (tsc2101Registers[TOUCH_CONTROL_AUX2_MAX] & 0xFFF))
         goto trigger;

      if(tsc2101Registers[TOUCH_CONTROL_AUX2_MIN] & 0x1000 && tsc2101Registers[TOUCH_DATA_AUX2] <= (tsc2101Registers[TOUCH_CONTROL_AUX2_MIN] & 0xFFF))
         goto trigger;

      if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000){
         //buffer mode, check if data is in buffer
         if(tsc2101BufferFifoEntrys() >= ((tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] >> 11 & 0x0007) + 1) * 8)
            goto trigger;
      }
      else{
         //register mode
         if(tsc2101HasNewData)
            goto trigger;
      }
   }

   //TODO: ignoring touch detect power down
   //check PINTDAV is pen or pen and data interrupt and pen is down
   if((tsc2101Registers[TOUCH_CONTROL_STATUS] >> 14 & 0x0003) != 0x0001)
      if(palmInput.touchscreenTouched)
         goto trigger;

   //TODO: set pin high
   return;

   trigger:
   //TODO: set pin low
   return;
}

void tsc2101Scan(void){
   bool keepScanning = false;

   switch(tsc2101Registers[TOUCH_CONTROL_TSC_ADC] >> 10 & 0x000F){
      case 0x0:
         //none
         break;

      case 0x1:
         //touch x and y
         tsc2101Registers[TOUCH_DATA_X] = tsc2101GetXValue();
         tsc2101Registers[TOUCH_DATA_Y] = tsc2101GetYValue();
         tsc2101HasNewData |= 1 << TOUCH_DATA_X | 1 << TOUCH_DATA_Y;
         keepScanning = true;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000){
            tsc2101BufferFifoWrite(TOUCH_DATA_Y);
            tsc2101BufferFifoWrite(TOUCH_DATA_X);
         }
         break;

      case 0x2:
         //touch x, y, z1 and z2
         tsc2101Registers[TOUCH_DATA_X] = tsc2101GetXValue();
         tsc2101Registers[TOUCH_DATA_Y] = tsc2101GetYValue();
         tsc2101Registers[TOUCH_DATA_Z1] = tsc2101GetZ1Value();
         tsc2101Registers[TOUCH_DATA_Z2] = tsc2101GetZ2Value();
         tsc2101HasNewData |= 1 << TOUCH_DATA_X | 1 << TOUCH_DATA_Y | 1 << TOUCH_DATA_Z1 | 1 << TOUCH_DATA_Z2;
         keepScanning = true;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000){
            tsc2101BufferFifoWrite(TOUCH_DATA_Y);
            tsc2101BufferFifoWrite(TOUCH_DATA_X);
            tsc2101BufferFifoWrite(TOUCH_DATA_Z1);
            tsc2101BufferFifoWrite(TOUCH_DATA_Z2);
         }
         break;

      case 0x3:
         //touch x
         tsc2101Registers[TOUCH_DATA_X] = tsc2101GetXValue();
         tsc2101HasNewData |= 1 << TOUCH_DATA_X;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000)
            tsc2101BufferFifoWrite(TOUCH_DATA_X);
         break;

      case 0x4:
         //touch y
         tsc2101Registers[TOUCH_DATA_Y] = tsc2101GetYValue();
         tsc2101HasNewData |= 1 << TOUCH_DATA_Y;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000)
            tsc2101BufferFifoWrite(TOUCH_DATA_Y);
         break;

      case 0x5:
         //touch z1 and z2
         tsc2101Registers[TOUCH_DATA_Z1] = tsc2101GetZ1Value();
         tsc2101Registers[TOUCH_DATA_Z2] = tsc2101GetZ2Value();
         tsc2101HasNewData |= 1 << TOUCH_DATA_Z1 | 1 << TOUCH_DATA_Z2;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000){
            tsc2101BufferFifoWrite(TOUCH_DATA_Z1);
            tsc2101BufferFifoWrite(TOUCH_DATA_Z2);
         }
         break;

      case 0x6:
         //battery
         tsc2101Registers[TOUCH_DATA_BAT] = tsc2101GetBatValue();
         tsc2101HasNewData |= 1 << TOUCH_DATA_BAT;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000)
            tsc2101BufferFifoWrite(TOUCH_DATA_BAT);
         break;

      case 0x7:
         //AUX1
         tsc2101Registers[TOUCH_DATA_AUX1] = tsc2101GetAux1Value();
         tsc2101HasNewData |= 1 << TOUCH_DATA_AUX1;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000)
            tsc2101BufferFifoWrite(TOUCH_DATA_AUX1);
         break;

      case 0x8:
         //AUX2
         tsc2101Registers[TOUCH_DATA_AUX2] = tsc2101GetAux2Value();
         tsc2101HasNewData |= 1 << TOUCH_DATA_AUX2;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000)
            tsc2101BufferFifoWrite(TOUCH_DATA_AUX2);
         break;

      case 0x9:
         //auto scan
         //TODO: implement
         //keepScanning = true;
         debugLog("TSC2101 auto scan unimplemented\n");
         break;

      case 0xA:
         //TEMP1
         tsc2101Registers[TOUCH_DATA_TEMP1] = tsc2101GetTemp1Value();
         tsc2101HasNewData |= 1 << TOUCH_DATA_TEMP1;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000)
            tsc2101BufferFifoWrite(TOUCH_DATA_TEMP1);
         break;

      case 0xB:
         //BAT, AUX1 and AUX2
         tsc2101Registers[TOUCH_DATA_BAT] = tsc2101GetBatValue();
         tsc2101Registers[TOUCH_DATA_AUX1] = tsc2101GetAux1Value();
         tsc2101Registers[TOUCH_DATA_AUX2] = tsc2101GetAux2Value();
         tsc2101HasNewData |= 1 << TOUCH_DATA_BAT | 1 << TOUCH_DATA_AUX1 | 1 << TOUCH_DATA_AUX2;
         keepScanning = true;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000){
            tsc2101BufferFifoWrite(TOUCH_DATA_BAT);
            tsc2101BufferFifoWrite(TOUCH_DATA_AUX1);
            tsc2101BufferFifoWrite(TOUCH_DATA_AUX2);
         }
         break;

      case 0xC:
         //TEMP2
         tsc2101Registers[TOUCH_DATA_TEMP2] = tsc2101GetTemp2Value();
         tsc2101HasNewData |= 1 << TOUCH_DATA_TEMP2;

         if(tsc2101Registers[TOUCH_CONTROL_BUFFER_MODE] & 0x8000)
            tsc2101BufferFifoWrite(TOUCH_DATA_TEMP2);
         break;

      default:
         //just turns power lines on, do nothing
         break;
   }

   tsc2101RefreshInterrupt();

   if(keepScanning)
      pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_TSC2101_SCAN, TSC2101_SCAN_DURATION);
   else
      tsc2101Registers[TOUCH_CONTROL_TSC_ADC] &= 0xC3FF;
}

