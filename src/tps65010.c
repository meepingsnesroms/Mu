#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "pxa260/pxa260_GPIO.h"
#include "pxa260/pxa260I2c.h"
#include "pxa260/pxa260.h"


//I2C address is 100100X(X set by IFLSB pin)(seems to be 0, first byte transmitted over I2C is 0x90(0b10010000))


enum{
   CHGSTATUS = 0x01,//yes, the data sheet actually says the first register is 0x01, not 0x00
   REGSTATUS,
   MASK1,
   MASK2,
   ACKINT1,
   ACKINT2,
   CHGCONFIG,
   LED1_ON,
   LED1_PER,
   LED2_ON,
   LED2_PER,
   VDCDC1,
   VDCDC2,
   VREGS1,
   MASK3,
   DEFGPIO
};


static uint8_t tps65010Registers[0x11];
static uint8_t tps65010CurrentI2cByte;
static uint8_t tps65010CurrentI2cByteBitsRemaining;
static uint8_t tps65010SelectedRegister;
static uint8_t tps65010State;
static bool    tps65010SelectedRegisterAlreadySet;


static uint8_t tps65010ReadGpio(void){
   uint8_t direction = tps65010Registers[DEFGPIO] >> 4;
   uint8_t inputVoltage = /*!palmSdCard.flashChipData << 2 | */!palmInput.buttonPower;//TODO: card power might be output//TODO: is power button low or high when pressed//TODO: GPIO4 BCM *UNKNOWN*

   debugLog("TPS65010 DEFGPIO read, PC:0x%08X\n", pxa260GetPc());

   return tps65010Registers[DEFGPIO] & (0xF0 | direction) | inputVoltage & ~direction;
}

static void tps65010WriteGpio(uint8_t value){
   uint8_t direction = value >> 4;
   uint8_t voltage = value & 0x0F;

   debugLog("TPS65010 DEFGPIO write: 0x%02X, PC:0x%08X\n", value, pxa260GetPc());

   //ignore writes to input pins
   tps65010Registers[DEFGPIO] = direction << 4 | voltage & direction | tps65010Registers[DEFGPIO] & 0x0F & ~direction;
}

static uint8_t tps65010ReadRegister(uint8_t address){
   switch(address){
      /*
      case ACKINT1:
      case ACKINT2:
         //TODO: dont know proper behavior, datasheet says the CPU shouldnt need to access this
         //should probably clear the ints
         return 0x00;
      */

      case DEFGPIO:
         return tps65010ReadGpio();

      case CHGSTATUS:
      case REGSTATUS:
      case MASK1:
      case MASK2:
      case CHGCONFIG:
      case LED1_ON:
      case LED1_PER:
      case LED2_ON:
      case LED2_PER:
      case VDCDC1:
      case VDCDC2:
      case VREGS1:
      case MASK3:
         //simple read, no actions needed
         return tps65010Registers[address];

      default:
         debugLog("Unimplemented TPS65010 register read, address:0x%02X, PC:0x%08X\n", address, pxa260GetPc());
         return 0x00;
   }
}

static void tps65010WriteRegister(uint8_t address, uint8_t value){
   switch(address){
      /*
      case MASK3:
         tps65010Registers[address] = value;
         debugLog("TPS65010 MASK3 write, value:0x%02X\n", value);
         return;

      case DEFGPIO:
         tps65010Registers[address] = value;
         debugLog("TPS65010 DEFGPIO write, value:0x%02X\n", value);
         return;
       */

      default:
         debugLog("Unimplemented TPS65010 register write, address:0x%02X, value:0x%02X, PC:0x%08X\n", address, value, pxa260GetPc());
         return;
   }
}

void tps65010Reset(void){
   memset(tps65010Registers, 0x00, sizeof(tps65010Registers));
   tps65010CurrentI2cByte = 0x00;
   tps65010CurrentI2cByteBitsRemaining = 8;
   tps65010SelectedRegister = 0x00;
   tps65010State = I2C_WAIT_FOR_ADDR;
   tps65010SelectedRegisterAlreadySet = false;

   tps65010Registers[MASK1] = 0xFF;
   tps65010Registers[MASK2] = 0xFF;
   tps65010Registers[CHGCONFIG] = 0x1B;
   tps65010Registers[VDCDC1] = 0x72;//TODO: 0x7(2/3) need to check DEFMAIN pin
   tps65010Registers[VDCDC2] = 0x68;//TODO: 0x(6/7)8 need to check DEFCORE pin
   tps65010Registers[VREGS1] = 0x88;
}

uint32_t tps65010StateSize(void){
   uint32_t size = 0;

   return size;
}

void tps65010SaveState(uint8_t* data){
   uint32_t offset = 0;

}

void tps65010LoadState(uint8_t* data){
   uint32_t offset = 0;

}

uint8_t tps65010I2cExchange(uint8_t i2cBus){
   bool newI2cBus = I2C_FLOATING_BUS;

   if(i2cBus == I2C_START){
      tps65010State = I2C_WAIT_FOR_ADDR;
      return I2C_FLOATING_BUS;
   }
   else if(i2cBus == I2C_STOP){
      return I2C_FLOATING_BUS;
   }
   else if(tps65010State == I2C_NOT_SELECTED){
      return I2C_FLOATING_BUS;
   }

   if(tps65010State != I2C_SENDING){
      tps65010CurrentI2cByte <<= 1;
      tps65010CurrentI2cByte |= (i2cBus == I2C_1);
   }
   else{
      newI2cBus = (tps65010CurrentI2cByte & 1 << (tps65010CurrentI2cByteBitsRemaining - 1)) ? I2C_1 : I2C_0;
   }
   tps65010CurrentI2cByteBitsRemaining--;

   if(tps65010CurrentI2cByteBitsRemaining == 0){
      //process data from byte
      switch(tps65010State){
         case I2C_WAIT_FOR_ADDR:
            if((tps65010CurrentI2cByte & 0xFE) == 0x90){
               //the address is of this device
               tps65010State = (tps65010CurrentI2cByte & 0x01) ? I2C_SENDING : I2C_RECEIVING;
               if(tps65010State == I2C_SENDING)
                  tps65010CurrentI2cByte = tps65010ReadRegister(tps65010SelectedRegister);
            }
            else{
               tps65010State = I2C_NOT_SELECTED;
            }
            break;

         case I2C_RECEIVING:
            if(tps65010SelectedRegisterAlreadySet){
               tps65010WriteRegister(tps65010SelectedRegister, tps65010CurrentI2cByte);
               tps65010SelectedRegisterAlreadySet = false;
            }
            else{
               tps65010SelectedRegister = tps65010CurrentI2cByte;
               tps65010SelectedRegisterAlreadySet = true;
            }
            break;

         case I2C_SENDING:
            tps65010SelectedRegisterAlreadySet = false;
            break;

         default:
            debugLog("TPS65010 dont know what to do with byte, cmd:%d\n", tps65010State);
            break;
      }

      tps65010CurrentI2cByteBitsRemaining = 8;
   }

   return newI2cBus;
}

void tps65010UpdateInterrupt(void){
   //TODO: implement this
   //debugLog("Unimplemented TPS65010 interrupt check\n");

   //if(???)
   //   goto trigger;

   pxa260gpioSetState(&pxa260Gpio, 14, true);
   return;

   trigger:
   pxa260gpioSetState(&pxa260Gpio, 14, false);
}
