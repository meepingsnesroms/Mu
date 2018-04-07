#include <PalmOS.h>
#include "testSuite.h"
#include "viewer.h"
#include "debug.h"
#include "ugui.h"


Err makeFile(uint8_t* data, uint32_t size, char* fileName){
   FileHand        file;
   Err             error;
   uint32_t        count;
   
   file = FileOpen(0 /*cardNo*/, fileName, (uint32_t)'DATA', (uint32_t)'GuiC', fileModeReadWrite | fileModeAnyTypeCreator, &error);
   if(file == fileNullHandle || error)
      return error;
   
   /*the copy is used to anonymize the data source*/
   count = size;
   while(count != 0){
      uint32_t chunkSize = count > SHARED_DATA_BUFFER_SIZE ? SHARED_DATA_BUFFER_SIZE : count;
      int32_t  bytesWritten;
      uint32_t i;
      for(i = 0; i < chunkSize; i++){
         sharedDataBuffer[i] = data[i + size - count];
      }
      bytesWritten = FileWrite(file, sharedDataBuffer, 1, chunkSize, &error);
      if(bytesWritten != chunkSize || error)
         return error;
      count -= chunkSize;
   }
   
   UG_PutString(0, 0, "File close next");
   
   error = FileClose(file);//I have no idea why, but this function causes a fatal execption but i still get valid data in the file after reboot
   return error;
}

var hexRamBrowser(){
   static Boolean  clearScreen = true;
   static uint32_t nibble = UINT32_C(0x10000000);
   static uint32_t pointerValue = UINT32_C(0x77777777);/*in the middle of the address space*/
   
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
   
   StrPrintF(sharedDataBuffer, "Open Hex Viewer At:\n0x%08lX", pointerValue);
   UG_PutString(0, 0, sharedDataBuffer);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var dumpBootloaderToFile(){
   makeFile((uint8_t*)UINT32_C(0xFFFFFE00), 0x200, "BOOTLOADER.BIN");
   exitSubprogram();
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
