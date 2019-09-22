#include <stdint.h>

#include "tsc2101Driver.h"


uint32_t readAllTsc2101AdcValues(uint32_t* args){
   //uint16_t tsc2101OldStatus = tsc2101Read(TOUCH_CONTROL_TSC_ADC);
   
   //TODO: may need to scan TSC2101 first
   
   args[TOUCH_DATA_X] = tsc2101Read(TOUCH_DATA_X);
   args[TOUCH_DATA_Y] = tsc2101Read(TOUCH_DATA_Y);
   args[TOUCH_DATA_Z1] = tsc2101Read(TOUCH_DATA_Z1);
   args[TOUCH_DATA_Z2] = tsc2101Read(TOUCH_DATA_Z2);
   args[TOUCH_DATA_RESERVED_0] = 0x0000;
   args[TOUCH_DATA_BAT] = tsc2101Read(TOUCH_DATA_BAT);
   args[TOUCH_DATA_RESERVED_1] = 0x0000;
   args[TOUCH_DATA_AUX1] = tsc2101Read(TOUCH_DATA_AUX1);
   args[TOUCH_DATA_AUX2] = tsc2101Read(TOUCH_DATA_AUX2);
   args[TOUCH_DATA_TEMP1] = tsc2101Read(TOUCH_DATA_TEMP1);
   args[TOUCH_DATA_TEMP2] = tsc2101Read(TOUCH_DATA_TEMP2);
   
   //tsc2101Write(TOUCH_CONTROL_TSC_ADC, tsc2101OldStatus);
   
   return 0x0B;
}
