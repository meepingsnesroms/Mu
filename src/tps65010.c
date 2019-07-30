#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"


void tps65010Reset(void){
   
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

void tps65010SetChipSelect(bool value){
   
}

uint8_t tps65010I2cExchange(uint8_t i2cBus){
   debugLog("Unimplemented T3 PMU exchange, bus:%d\n", i2cBus);
   return EMU_I2C_FLOATING_BUS;
}
