#include <PalmOS.h>
#include <stdint.h>

#include "testSuite.h"
#include "specs/hardwareRegisterNames.h"
#include "specs/emuFeatureRegistersSpec.h"


enum{
   EMU_STATE_UNTESTED = 0,
   EMU_STATE_FALSE,
   EMU_STATE_TRUE
};


static const uint16_t* skipBusError = {
   0x5C8F,/*addq.l #6,sp    ; remove error info from stack*/
   0x54AF,/*addq.l #2,2(sp) ; skip over the invalid access, what ever was in the variable before the read should still be there*/
   0x0002,/*                ; displacement for above*/
   0x4E73/*rte              ; return*/
};

Boolean isEmulator(){
   static uint8_t emuStatus = EMU_STATE_UNTESTED;
   
   /*test if first call, else return cached value since testing is intensive*/
   if(emuStatus == EMU_STATE_UNTESTED){
      uint32_t oldBusErrorHandler = readArbitraryMemory32(2 * sizeof(uint32_t));/*EXCEPTION_BUS_ERROR vector*/
      uint8_t oldScr = readArbitraryMemory8(HW_REG_ADDR(SCR));
      uint32_t palmSpecialFeatures;
      
      writeArbitraryMemory8(HW_REG_ADDR(SCR), 0xF0);/*enable bus error timeout, clear current invalid accesses*/
      writeArbitraryMemory32(2 * sizeof(uint32_t), skipBusError);/*set EXCEPTION_BUS_ERROR vector*/
      
      /*set fallback value, will be the returned value if bus error timeout occurs*/
      palmSpecialFeatures = FEATURE_INVALID;
      
      /*everything is setup, now try to read invalid memory, success == its an emu*/
      palmSpecialFeatures = readArbitraryMemory32(EMU_REG_ADDR(EMU_INFO));
      
      if(palmSpecialFeatures == FEATURE_INVALID)
         emuStatus = EMU_STATE_FALSE;
      else
         emuStatus = EMU_STATE_TRUE;
   }
   
   return emuStatus == EMU_STATE_TRUE;
}

Boolean isEmulatorFeatureEnabled(uint32_t feature){
   if(isEmulator())
      return (readArbitraryMemory32(EMU_REG_ADDR(EMU_INFO)) & feature) != 0;
   return false;
}
