#include <PalmOS.h>
#include <stdint.h>

#include "testSuite.h"
#include "tools.h"


uint16_t (*customCall_HwrADC)(uint16_t mode, void* returnData);

Boolean initUndocumentedApiHandlers(){
   uint32_t romCrc;
   
   /*only have Ksyms for one version of Palm OS 4 for the Palm m515*/
   if(!isM515)
      return false;
   
   romCrc = calcCrc32((uint8_t*)0x10000000, 0x400000);
   
   switch(romCrc){
      case 0x6481A088:
         customCall_HwrADC = (uint16_t (*)(uint16_t, void*))0x10081284;/*Palm m515 ROM HwrADC address*/
         return true;
         break;
   }
   
   return false;
}
