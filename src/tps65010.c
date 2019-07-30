#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "pxa260/pxa260I2c.h"


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


static void tps65010WriteRegister(uint8_t address, uint8_t value){
   //TODO: add register writes
   debugLog("TPS65010 register writes are unimplemented, address:0x%02X, value:0x%02X\n", address, value);
}

void tps65010Reset(void){
   memset(tps65010Registers, 0x00, sizeof(tps65010Registers));
   tps65010Registers[MASK1] = 0xFF;
   tps65010Registers[MASK2] = 0xFF;
   tps65010Registers[CHGCONFIG] = 0x1B;
   tps65010Registers[VDCDC1] = 0x72;//TODO: 0x7(2/3) need to check DEFMAIN pin
   tps65010Registers[VDCDC2] = 0x68;//TODO: 0x(6/7)8 need to check DEFCORE pin
   tps65010Registers[VREGS1] = 0x88;

   tps65010CurrentI2cByte = 0x00;
   tps65010CurrentI2cByteBitsRemaining = 8;
   tps65010SelectedRegister = 0x00;
   tps65010State = I2C_WAIT_FOR_ADDR;
   tps65010SelectedRegisterAlreadySet = false;
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
      tps65010SelectedRegisterAlreadySet = false;
      return I2C_FLOATING_BUS;
   }
   else if(i2cBus == I2C_STOP){
      return I2C_FLOATING_BUS;
   }
   else if(tps65010State == I2C_NOT_SELECTED){
      return I2C_FLOATING_BUS;
   }

   tps65010CurrentI2cByte |= (i2cBus == I2C_1);
   if(tps65010CurrentI2cByteBitsRemaining > 0){
      if(tps65010State == I2C_SENDING){
         newI2cBus = (tps65010Registers[tps65010SelectedRegister] && 1 << (tps65010CurrentI2cByteBitsRemaining - 1)) ? I2C_1 : I2C_0;
      }
      tps65010CurrentI2cByteBitsRemaining--;
   }

   if(tps65010CurrentI2cByteBitsRemaining == 0){
      //process data from byte
      if(tps65010State == I2C_WAIT_FOR_ADDR){
         if((tps65010CurrentI2cByte & 0xFE) == 0x90){
            //the address is of this device
            tps65010State = (tps65010CurrentI2cByte & 0x01) ? I2C_SENDING : I2C_RECEIVING;
         }
         else{
            tps65010State = I2C_NOT_SELECTED;
         }
      }
      else if(tps65010State == I2C_RECEIVING){
         if(tps65010SelectedRegisterAlreadySet){
            tps65010WriteRegister(tps65010SelectedRegister, tps65010CurrentI2cByte);
         }
         else{
            tps65010SelectedRegister = tps65010CurrentI2cByte;
            tps65010SelectedRegisterAlreadySet = true;
         }
      }

      tps65010CurrentI2cByteBitsRemaining = 8;
   }

   return newI2cBus;
}
