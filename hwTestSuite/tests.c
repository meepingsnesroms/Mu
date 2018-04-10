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
   static Boolean  polaritySwap;
   static uint32_t frameCount;
   static uint8_t  portDOriginalPolarity;
   uint32_t y = 0;
   
   if(firstRun){
      debugSafeScreenClear(C_WHITE);
      polaritySwap = false;
      frameCount = 0;
      portDOriginalPolarity = readArbitraryMemory8(0xFFFFF41C);
      firstRun = false;
   }
   
   frameCount++;
   if(frameCount >= 30){
      writeArbitraryMemory8(0xFFFFF41C, ~readArbitraryMemory8(0xFFFFF41C) & 0x07);
      frameCount = 0;
   }
   
   if(getButton(buttonLeft) && getButton(buttonRight) && getButton(buttonUp) && getButton(buttonBack) && !getButton(buttonDown) && !getButton(buttonSelect)){
      firstRun = true;
      writeArbitraryMemory8(0xFFFFF41C, portDOriginalPolarity);
      exitSubprogram();
   }

   
   UG_PutString(0, y, "Press Left,Right,Up and Back to exit this test.");
   y += (FONT_HEIGHT + 1) * 2;
   
   UG_PutString(0, y, "This requirement is to allow button testing.");
   y += (FONT_HEIGHT + 1) * 2;
   
   /*setDebugTag("PDPOL read");*/
   StrPrintF(sharedDataBuffer, "PDPOL:0x%02X", readArbitraryMemory8(0xFFFFF41C));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   /*setDebugTag("PDDATA read");*/
   StrPrintF(sharedDataBuffer, "PDDATA:0x%02X", readArbitraryMemory8(0xFFFFF419));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   /*setDebugTag("PDKBEN read");*/
   StrPrintF(sharedDataBuffer, "PDKBEN:0x%02X", readArbitraryMemory8(0xFFFFF41E));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
