#include "testSuite.h"
#include "viewer.h"
#include "debug.h"
#include "ugui.h"


var hexRamBrowser(){
   static Boolean  clearScreen = true;
   static uint32_t nibble = 0x10000000;
   static uint32_t pointerValue = UINT32_C(0x77777777);/*in the middle of the address space*/
   static char     hexString[100];
   
   if(getButtonPressed(buttonUp)){
      pointerValue += nibble;
      setDebugTag("Nibble Up");
   }
   
   if(getButtonPressed(buttonDown)){
      pointerValue -= nibble;
      setDebugTag("Nibble Down");
   }
   
   if(getButtonPressed(buttonRight)){
      if(nibble > 0x00000001)
         nibble >>= 4;
   }
   
   if(getButtonPressed(buttonLeft)){
      if(nibble < 0x10000000)
         nibble <<= 4;
   }
   
   if(getButtonPressed(buttonSelect)){
      /*open hex viewer*/
      nibble = 0x10000000;
      clearScreen = true;
      setSubprogramArgs(makeVar(LENGTH_ANY, TYPE_PTR, (uint64_t)pointerValue));/*length doesnt matter*/
      execSubprogram(hexViewer);
   }
   
   if(getButtonPressed(buttonBack)){
      nibble = 0x10000000;
      clearScreen = true;
      exitSubprogram();
   }
   
   if(clearScreen){
      debugSafeScreenClear(C_WHITE);
      clearScreen = false;
   }
   
   setDebugTag("Hex Ram Browser Address Select");
   
   /*Palm OS sprintf only supports 16 bit ints*/
   StrPrintF(hexString, "Open Hex Viewer At:\n0x%04X%04X", (uint16_t)(pointerValue >> 16), (uint16_t)pointerValue);
   UG_PutString(0, 0, hexString);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
