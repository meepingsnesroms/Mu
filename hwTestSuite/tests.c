#include "testSuite.h"
#include "testSuiteConfig.h"
#include "debug.h"
#include "tools.h"
#include "ugui.h"


var testFileAccessWorks(){
   uint32_t testTook = UINT32_C(0xFFFFFE00);
   makeFile((uint8_t*)&testTook, 4, "FILEOUT.BIN");
   exitSubprogram();
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var testButtonInput(){
   static Boolean  firstRun = true;
   static uint32_t frameCount;
   uint32_t y = 0;
   
   if(firstRun){
      debugSafeScreenClear(C_WHITE);
      frameCount = 0;
      firstRun = false;
   }
   
   if(getButtonPressed(buttonLeft) && getButtonPressed(buttonRight) && getButtonPressed(buttonUp) && getButtonPressed(buttonBack) && !getButtonPressed(buttonDown) && !getButtonPressed(buttonSelect)){
      firstRun = true;
      exitSubprogram();
   }

   
   UG_PutString(0, y, "Press Left,Right,Up and Back to exit this test.");
   y += FONT_HEIGHT + 1;
   
   UG_PutString(0, y, "This requirement is to allow button testing.");
   y += FONT_HEIGHT + 1;
   
   StrPrintF(sharedDataBuffer, "PDPOL:0x%02X", readArbitraryMemory8(0xFFFFF41C));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   StrPrintF(sharedDataBuffer, "PDDATA:0x%02X", readArbitraryMemory8(0xFFFFF419));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   StrPrintF(sharedDataBuffer, "PDKBEN:0x%02X", readArbitraryMemory8(0xFFFFF41E));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
