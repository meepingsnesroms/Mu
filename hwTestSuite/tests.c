#include "testSuite.h"
#include "viewer.h"
#include "ugui.h"

var hexRamBrowser(){
   static Boolean  clearScreen = true;
   static uint32_t column = 0;/*what column to change with up/down keys*/
   static uint32_t pointerValue = 0x00000000;
   static char     hexString[100];
   
   
   if(getButtonPressed(buttonUp)){
      pointerValue += 0x1 << (4 * (7 - column));
   }
   
   if(getButtonPressed(buttonDown)){
      pointerValue -= 0x1 << (4 * (7 - column));
   }
   
   if(getButtonPressed(buttonLeft)){
      if(column > 0)
         column--;
   }
   
   if(getButtonPressed(buttonRight)){
      if(column < 7)
         column++;
   }
   
   if(getButtonPressed(buttonSelect)){
      /*open hex viewer*/
      clearScreen = true;
      setSubprogramArgs(makeVar(LENGTH_ANY, TYPE_PTR, pointerValue));/*length doesnt matter*/
      execSubprogram(hexViewer);
   }
   
   if(getButtonPressed(buttonBack)){
      clearScreen = true;
      exitSubprogram();
   }
   
   StrPrintF(hexString, "Open Hex Viewer At:\n0x%08X", pointerValue);
   
   if(clearScreen){
      UG_FillScreen(C_WHITE);
      clearScreen = false;
   }
   
   UG_PutString(0, 0, hexString);
}
