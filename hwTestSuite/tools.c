#include <PalmOS.h>
#include "testSuite.h"
#include "viewer.h"
#include "debug.h"
#include "ugui.h"


Err makeFile(uint8_t* data, uint32_t size, char* fileName){
   FileHand        file;
   Err             error;
   uint32_t        count;
   
   file = FileOpen(0 /*cardNo*/, fileName, (uint32_t)'DATA', (uint32_t)'GuiC', fileModeReadWrite, &error);
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
   
   error = FileClose(file);
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
      callSubprogram(hexViewer);
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

var getTrapAddress(){
   static Boolean  clearScreen = true;
   static uint16_t nibble = 0x100;
   static uint16_t trapNum = 0xA377;/*in the middle of the trap list*/
   
   if(getButtonPressed(buttonUp)){
      trapNum = (trapNum + nibble & 0xFFF) | 0xA000;
   }
   
   if(getButtonPressed(buttonDown)){
      trapNum = (trapNum - nibble & 0xFFF) | 0xA000;
   }
   
   if(getButtonPressed(buttonRight)){
      if(nibble > 0x001)
         nibble >>= 4;
   }
   
   if(getButtonPressed(buttonLeft)){
      if(nibble < 0x100)
         nibble <<= 4;
   }
   
   if(getButtonPressed(buttonSelect)){
      /*open hex viewer*/
      nibble = 0x100;
      clearScreen = true;
      setSubprogramArgs(makeVar(LENGTH_ANY, TYPE_PTR, (uint64_t)(uint32_t)SysGetTrapAddress(trapNum)));
      callSubprogram(valueViewer);
   }
   
   if(getButtonPressed(buttonBack)){
      nibble = 0x100;
      clearScreen = true;
      exitSubprogram();
   }
   
   if(clearScreen){
      debugSafeScreenClear(C_WHITE);
      clearScreen = false;
   }
   
   StrPrintF(sharedDataBuffer, "Trap Num:\n0x%04X", trapNum);
   UG_PutString(0, 0, sharedDataBuffer);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var manualLssa(){
   static uint32_t nibble;
   static uint32_t hexValue;
   static uint32_t originalLssa;
   static Boolean  customEnabled;
   static Boolean  firstRun = true;
   
   if(firstRun == true){
      nibble = UINT32_C(0x10000000);
      hexValue = UINT32_C(0x77777777);
      originalLssa = readArbitraryMemory32(0xFFFFFA00);
      customEnabled = false;
      debugSafeScreenClear(C_WHITE);
      firstRun = false;
   }
   
   if(getButtonPressed(buttonUp)){
      hexValue += nibble;
   }
   
   if(getButtonPressed(buttonDown)){
      hexValue -= nibble;
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
      if(customEnabled){
         writeArbitraryMemory32(0xFFFFFA00, originalLssa);
         customEnabled = false;
      }
      else{
         writeArbitraryMemory32(0xFFFFFA00, hexValue);
         customEnabled = true;
      }
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   StrPrintF(sharedDataBuffer, "Enter switchs between this address and the custom LSSA.\nNew LSSA:\n0x%08lX", hexValue);
   UG_PutString(0, 0, sharedDataBuffer);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var dumpBootloaderToFile(){
   makeFile((uint8_t*)UINT32_C(0xFFFFFE00), 0x200, "BOOTLOADER.BIN");
   exitSubprogram();
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
