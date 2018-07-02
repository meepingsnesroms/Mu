#include <PalmOS.h>
#include <stdint.h>

#include "testSuite.h"


uint16_t (*customCall_HwrADC)(uint16_t mode, void* returnData);


Boolean initUndocumentedApiHandlers(){
   /*only have Ksyms for the Palm m515*/
   if(!isM515)
      return false;
   
   /*this may not be valid for all m515 devices, but I have yet to make a memory scanning dynamic linker for Palm OS to find the symbols so its static linking for now*/
   customCall_HwrADC = (uint16_t (*)(uint16_t, void*))0x10081284;/*Palm m515 ROM HwrADC address*/
   
   return true;
}
