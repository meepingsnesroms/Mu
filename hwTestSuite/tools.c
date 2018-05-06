#include <PalmOS.h>
#include "hardwareRegisterNames.h"
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
      if(bytesWritten != chunkSize || error){
         FileClose(file);
         return error;
      }
      
      count -= chunkSize;
   }
   
   error = FileClose(file);
   return error;
}

uint16_t ads7846GetValue(uint8_t channel, Boolean referenceMode, Boolean mode8bit){
   uint16_t spi2Control = readArbitraryMemory16(HW_REG_ADDR(SPICONT2)) & 0xE230;
   uint8_t config = 0x80;
   uint16_t value;
   
   if(mode8bit)
      config |= 0x08;
   
   if(referenceMode)
      config |= 0x04;
   
   config |= channel << 4 & 0x70;
   
   /*wait until SPI2 is free*/
   while(readArbitraryMemory16(HW_REG_ADDR(SPICONT2)) & 0x0100);
   
   /*set data to send*/
   writeArbitraryMemory16(HW_REG_ADDR(SPIDATA2), config << 8);
   
   /*enable SPI 2 if disabled*/
   if(!(spi2Control & 0x0200))
      writeArbitraryMemory16(HW_REG_ADDR(SPICONT2), spi2Control | 0x0200);
   
   /*send data*/
   writeArbitraryMemory16(HW_REG_ADDR(SPICONT2), spi2Control | 0x0007);
   while(readArbitraryMemory16(HW_REG_ADDR(SPICONT2)) & 0x0100);
   
   /*receive data, mode = 1(8 bits) or mode = 0(12 bits)*/
   writeArbitraryMemory16(HW_REG_ADDR(SPICONT2), spi2Control | (mode8bit ? 0x0007 : 0x000B));
   while(readArbitraryMemory16(HW_REG_ADDR(SPICONT2)) & 0x0100);
   
   value = readArbitraryMemory16(HW_REG_ADDR(SPIDATA2));
   
   /*may need to alter value some*/
   
   return value;
}

var hexRamBrowser(){
   static Boolean  firstRun = true;
   static uint32_t nibble;
   static uint32_t pointerValue;
   
   if(firstRun){
      debugSafeScreenClear(C_WHITE);
      nibble = 0x10000000;
      pointerValue = 0x77777777;/*in the middle of the address space*/
      firstRun = false;
   }
   
   if(getButtonPressed(buttonUp))
      pointerValue += nibble;
   
   if(getButtonPressed(buttonDown))
      pointerValue -= nibble;
   
   if(getButtonPressed(buttonRight))
      if(nibble > 0x00000001)
         nibble >>= 4;
   
   if(getButtonPressed(buttonLeft))
      if(nibble < 0x10000000)
         nibble <<= 4;
   
   if(getButtonPressed(buttonSelect)){
      /*open hex viewer*/
      firstRun = true;
      setSubprogramArgs(makeVar(LENGTH_ANY, TYPE_PTR, (uint64_t)pointerValue));/*length doesnt matter*/
      callSubprogram(hexViewer);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   StrPrintF(sharedDataBuffer, "Open Hex Viewer At:\n0x%08lX", pointerValue);
   UG_PutString(0, 0, sharedDataBuffer);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var getTrapAddress(){
   static Boolean firstRun = true;
   static uint16_t nibble;
   static uint16_t trapNum;
   
   if(firstRun){
      debugSafeScreenClear(C_WHITE);
      nibble = 0x100;
      trapNum = 0xA377;/*in the middle of the trap list*/
      firstRun = false;
   }
   
   if(getButtonPressed(buttonUp))
      trapNum = (trapNum + nibble & 0xFFF) | 0xA000;
   
   if(getButtonPressed(buttonDown))
      trapNum = (trapNum - nibble & 0xFFF) | 0xA000;
   
   if(getButtonPressed(buttonRight))
      if(nibble > 0x001)
         nibble >>= 4;
   
   if(getButtonPressed(buttonLeft))
      if(nibble < 0x100)
         nibble <<= 4;
   
   if(getButtonPressed(buttonSelect)){
      /*open hex viewer*/
      firstRun = true;
      setSubprogramArgs(makeVar(LENGTH_ANY, TYPE_PTR, (uint64_t)(uint32_t)SysGetTrapAddress(trapNum)));
      callSubprogram(valueViewer);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
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
   
   if(firstRun){
      nibble = 0x10000000;
      hexValue = 0x77777777;
      originalLssa = readArbitraryMemory32(HW_REG_ADDR(LSSA));
      customEnabled = false;
      debugSafeScreenClear(C_WHITE);
      firstRun = false;
   }
   
   if(getButtonPressed(buttonUp))
      hexValue += nibble;
   
   if(getButtonPressed(buttonDown))
      hexValue -= nibble;
   
   if(getButtonPressed(buttonRight))
      if(nibble > 0x00000001)
         nibble >>= 4;
   
   if(getButtonPressed(buttonLeft))
      if(nibble < 0x10000000)
         nibble <<= 4;
   
   if(getButtonPressed(buttonSelect))
      if(customEnabled){
         writeArbitraryMemory32(HW_REG_ADDR(LSSA), originalLssa);
         customEnabled = false;
      }
      else{
         writeArbitraryMemory32(HW_REG_ADDR(LSSA), hexValue);
         customEnabled = true;
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
   makeFile((uint8_t*)0xFFFFFE00, 0x200, "BOOTLOADER.BIN");
   exitSubprogram();
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
