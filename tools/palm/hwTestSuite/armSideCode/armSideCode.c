#include <stdint.h>

#include "armSideCode.h"
#include "armDefines.h"
#include "armControl.h"
#include "armTests.h"


USED unsigned long runTest(const void* emulStateP, void* userData68KP, /*Call68KFuncType*/void* call68KFuncP){
   uint32_t oldInts;
   uint32_t* args = (uint32_t*)userData68KP;
   uint32_t test;
   uint32_t returnArgCount;
   
   test = args[0];
   args++;

   oldInts = disableInts();
   
   switch(test){
      case ARM_TEST_TSC2101_READ_ADC_VALUES:
         returnArgCount = readAllTsc2101AdcValues(args);
         break;

      default:
         returnArgCount = 0;
         break;
   }
   
   enableInts(oldInts);
   return returnArgCount;
}

ENTRYPOINT("runTest")
