#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>


uint32_t getGlobalVar(uint16_t id){
   uint32_t value;
   FtrGet('EMUG', id, &value);
   return value;
}

void setGlobalVar(uint16_t id, uint32_t value){
   FtrSet('EMUG', id, value);
}
