#include "testSuite.h"
#include "viewer.h"
#include "debug.h"
#include "ugui.h"


uint32_t makeFile(uint8_t* data, uint32_t size, char* fileName){
   FileHand file;
   Err      error;
   uint8_t  dataBuffer[100];//used to anonymize data source
   uint32_t count;
   
   file = FileOpen(0 /*cardNo*/, fileName, 'DATA', 'GuiC', fileModeReadWrite | fileModeAnyTypeCreator, &error);
   if(!file || error)
      return error;
   
   count = size;
   while(count != 0){
      uint32_t chunkSize = count > 100 ? 100 : count;
      int32_t  bytesWritten;
      uint32_t i;
      for(i = 0; i < chunkSize; i++){
         dataBuffer[i] = data[i + size - count];
      }
      bytesWritten = FileWrite(file, dataBuffer, 1, chunkSize, &error);
      if(bytesWritten != chunkSize || error)
         return error;
      count -= chunkSize;
   }
   
   error = FileClose(file);
   if(error)
      return error;
   
   return 0;//success
}

var hexRamBrowser(){
   static Boolean  clearScreen = true;
   static uint32_t nibble = UINT32_C(0x10000000);
   static uint32_t pointerValue = UINT32_C(0x77777777);/*in the middle of the address space*/
   static char     hexString[100];
   
   if(getButtonPressed(buttonUp)){
      pointerValue += nibble;
   }
   
   if(getButtonPressed(buttonDown)){
      pointerValue -= nibble;
   }
   
   if(getButtonPressed(buttonRight)){
      if(nibble > UINT32_C(0x00000001))
         nibble >>= 4;
   }
   
   if(getButtonPressed(buttonLeft)){
      if(nibble < UINT32_C(0x10000000))
         nibble <<= 4;
   }
   
   if(getButtonPressed(buttonSelect)){
      /*open hex viewer*/
      nibble = UINT32_C(0x10000000);
      clearScreen = true;
      setSubprogramArgs(makeVar(LENGTH_ANY, TYPE_PTR, (uint64_t)pointerValue));/*length doesnt matter*/
      execSubprogram(hexViewer);
   }
   
   if(getButtonPressed(buttonBack)){
      nibble = UINT32_C(0x10000000);
      clearScreen = true;
      exitSubprogram();
   }
   
   if(clearScreen){
      debugSafeScreenClear(C_WHITE);
      clearScreen = false;
   }
   
   /*Palm OS sprintf only supports 16 bit ints*/
   StrPrintF(hexString, "Open Hex Viewer At:\n0x%04X%04X", (uint16_t)(pointerValue >> 16), (uint16_t)pointerValue);
   UG_PutString(0, 0, hexString);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var dumpBootloaderToFile(){
   makeFile((uint8_t*)UINT32_C(0xFFFFFE00), 0x200, "BOOTLOAD.BIN");
   exitSubprogram();
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var testFileAccessWorks(){
   uint32_t testTook = UINT32_C(0xFFFFFE00);
   makeFile((uint8_t*)&testTook, 4, "FILEOUT.BIN");
   exitSubprogram();
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

