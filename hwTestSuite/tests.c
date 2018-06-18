#include <PalmOS.h>
#include <stdint.h>
#include <stdio.h>

#include "testSuite.h"
#include "testSuiteConfig.h"
#include "specs/hardwareRegisterNames.h"
#include "specs/sed1376RegisterNames.h"
#include "debug.h"
#include "tools.h"
#include "cpu.h"
#include "ugui.h"


var testButtonInput(){
   static Boolean  firstRun = true;
   static Boolean  polaritySwap;
   static uint8_t frameCount;
   static uint8_t  portDOriginalPolarity;
   uint16_t y = 0;
   
   if(firstRun){
      debugSafeScreenClear(C_WHITE);
      polaritySwap = false;
      frameCount = 0;
      portDOriginalPolarity = readArbitraryMemory8(HW_REG_ADDR(PDPOL));
      firstRun = false;
   }
   
   frameCount++;
   if(frameCount >= 30){
      writeArbitraryMemory8(HW_REG_ADDR(PDPOL), ~readArbitraryMemory8(HW_REG_ADDR(PDPOL)) & 0x07);
      frameCount = 0;
   }
   
   if(getButton(buttonLeft) && getButton(buttonRight) && getButton(buttonUp) && getButton(buttonBack) && !getButton(buttonDown) && !getButton(buttonSelect)){
      firstRun = true;
      writeArbitraryMemory8(HW_REG_ADDR(PDPOL), portDOriginalPolarity);
      exitSubprogram();
   }

   
   UG_PutString(0, y, "Press Left,Right,Up and Back to exit this test.");
   y += (FONT_HEIGHT + 1) * 2;
   
   UG_PutString(0, y, "This requirement is to allow button testing.");
   y += (FONT_HEIGHT + 1) * 2;
   
   StrPrintF(sharedDataBuffer, "PDPOL:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PDPOL)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PDDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PDDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PDKBEN:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PDKBEN)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var listDataRegisters(){
   static Boolean firstRun = true;
   uint16_t y = 0;
   
   if(firstRun){
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   StrPrintF(sharedDataBuffer, "PADATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PADATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PBDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PBDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PCDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PCDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PDDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PDDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PEDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PEDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PFDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PFDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PGDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PGDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PJDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PJDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PKDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PKDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PMDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PMDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var interrogateSpi2(){
   static Boolean firstRun = true;
   static float spiclk2PercentAs1;
   uint16_t y = 0;
   
   if(firstRun){
      firstRun = false;
      spiclk2PercentAs1 = percentageOfTimeAs1(HW_REG_ADDR(PEDATA), 8, 2, 20000, 3);
      debugSafeScreenClear(C_WHITE);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   StrPrintF(sharedDataBuffer, "PADATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PADATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PBDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PBDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PCDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PCDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   /*PDDATA is buttons, not relevent to the SPI*/
   StrPrintF(sharedDataBuffer, "PEDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PEDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PFDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PFDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PGDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PGDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PJDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PJDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PKDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PKDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PMDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PMDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PESEL:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PESEL)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PEDIR:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PEDIR)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "SPICONT2:0x%04X", readArbitraryMemory16(HW_REG_ADDR(SPICONT2)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "SPIDATA2:0x%04X", readArbitraryMemory16(HW_REG_ADDR(SPIDATA2)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "SPICLK2:%s", floatToString(spiclk2PercentAs1));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var tstat1GetSemaphoreLockOrder(){
   static Boolean firstRun = true;
   uint16_t testWriteValue = 0xF0F1;
   uint16_t y = 0;
   
   if(firstRun){
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   StrPrintF(sharedDataBuffer, "TSTAT1:0x%04X", readArbitraryMemory16(HW_REG_ADDR(TSTAT1)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   writeArbitraryMemory16(HW_REG_ADDR(TSTAT1), testWriteValue);
   StrPrintF(sharedDataBuffer, "Write TSTAT1:0x%04X", testWriteValue);
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "New TSTAT1:0x%04X", readArbitraryMemory16(HW_REG_ADDR(TSTAT1)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   writeArbitraryMemory16(HW_REG_ADDR(TSTAT1), 0xFFFF);
   StrPrintF(sharedDataBuffer, "Clear Semaphore");
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "New TSTAT1:0x%04X", readArbitraryMemory16(HW_REG_ADDR(TSTAT1)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var ads7846Read(){
   static Boolean firstRun = true;
   static Boolean referenceMode;
   static Boolean mode8Bit;
   uint8_t ads7846Channel;
   uint16_t y = 0;
   
   if(firstRun){
      firstRun = false;
      referenceMode = false;
      mode8Bit = false;
      debugSafeScreenClear(C_WHITE);
   }
   
   if(getButtonPressed(buttonSelect)){
      if(!referenceMode && !mode8Bit){
         referenceMode = true;
      }
      else if(referenceMode && !mode8Bit){
         referenceMode = false;
         mode8Bit = true;
      }
      else if(!referenceMode && mode8Bit){
         referenceMode = true;
      }
      else{
         referenceMode = false;
         mode8Bit = false;
      }
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   StrPrintF(sharedDataBuffer, "Ref Mode:%d, 8bit:%d", referenceMode, mode8Bit);
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   for(ads7846Channel = 0; ads7846Channel < 8; ads7846Channel++){
      StrPrintF(sharedDataBuffer, "Ch:%d Value:0x%04X", ads7846Channel, ads7846GetValue(ads7846Channel, referenceMode, mode8Bit));
      UG_PutString(0, y, sharedDataBuffer);
      y += FONT_HEIGHT + 1;
   }
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var selfProbeSpi2(){
   static Boolean firstRun = true;
   uint16_t testWriteValue = 0xF0F1;
   uint16_t y = 0;
   uint8_t scootTest = 0x0F;
   
   if(firstRun){
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   writeArbitraryMemory16(HW_REG_ADDR(SPIDATA2), 0x0001);
   
   StrPrintF(sharedDataBuffer, "SPIDATA2:0x%04X", readArbitraryMemory16(HW_REG_ADDR(SPIDATA2)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   writeArbitraryMemory16(HW_REG_ADDR(SPICONT2), 0x0200);/*enable SPI2*/
   writeArbitraryMemory16(HW_REG_ADDR(SPICONT2), 0x0300);/*shift in 1 bit*/
   StrPrintF(sharedDataBuffer, "Shift left 1");
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "New SPIDATA2:0x%04X", readArbitraryMemory16(HW_REG_ADDR(SPIDATA2)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "Scoot Test:0x%04X", scootTest << 8);
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "Scoot Test 2:0x%04X", (uint16_t)scootTest << 8);
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var getClk32Frequency(){
   static Boolean firstRun = true;
   
   if(firstRun){
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
      StrPrintF(sharedDataBuffer, "CLK32 Freq:%s", (readArbitraryMemory16(HW_REG_ADDR(RTCCTL)) & 0x0020) ? "38.4khz" : "32.7khz");
      UG_PutString(0, 0, sharedDataBuffer);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var getDeviceId(){
   static Boolean firstRun = true;
   
   if(firstRun){
      char deviceId[5];
      
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
      FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, deviceId);
      deviceId[4] = '\0';/*Palm OS sprintf doesnt support %.*s string length modifyers*/
      StrPrintF(sharedDataBuffer, "Device ID:%s", &deviceId);
      UG_PutString(0, 0, sharedDataBuffer);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var getCpuInfo(){
   static Boolean firstRun = true;
   
   if(firstRun){
      uint16_t y = 0;
      
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
      StrPrintF(sharedDataBuffer, "CPU Type:%s", getCpuString());
      UG_PutString(0, y, sharedDataBuffer);
      y += (FONT_HEIGHT + 1) * 5;
      StrPrintF(sharedDataBuffer, "SCR:0x%02X", readArbitraryMemory8(HW_REG_ADDR(SCR)));
      UG_PutString(0, y, sharedDataBuffer);
      y += FONT_HEIGHT + 1;
      StrPrintF(sharedDataBuffer, "CPU ID(IDR):0x%08lX", readArbitraryMemory32(HW_REG_ADDR(IDR)));
      UG_PutString(0, y, sharedDataBuffer);
      y += FONT_HEIGHT + 1;
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var toggleBacklight(){
   static Boolean firstRun = true;
   uint16_t y = 0;
   
   if(firstRun){
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
      StrPrintF(sharedDataBuffer, "Left = SED1376");
      UG_PutString(0, y, sharedDataBuffer);
      y += FONT_HEIGHT + 1;
      StrPrintF(sharedDataBuffer, "Right = Port G");
      UG_PutString(0, y, sharedDataBuffer);
      y += FONT_HEIGHT + 1;
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   if(getButtonPressed(buttonLeft)){
      writeArbitraryMemory8(0x1FF80000 + GPIO_CONT_0, readArbitraryMemory8(0x1FF80000 + GPIO_CONT_0) ^ 0x10);
   }
   
   if(getButtonPressed(buttonRight)){
      writeArbitraryMemory8(HW_REG_ADDR(PGDATA), readArbitraryMemory8(HW_REG_ADDR(PGDATA)) ^ 0x02);
   }
   
   y = (FONT_HEIGHT + 1) * 2;
   StrPrintF(sharedDataBuffer, "PGDATA:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PGDATA)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PGSEL:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PGSEL)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   StrPrintF(sharedDataBuffer, "PGPUEN:0x%02X", readArbitraryMemory8(HW_REG_ADDR(PGPUEN)));
   UG_PutString(0, y, sharedDataBuffer);
   y += FONT_HEIGHT + 1;
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var toggleMotor(){
   static Boolean firstRun = true;
   
   if(firstRun){
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
      StrPrintF(sharedDataBuffer, "Select = Toggle Motor");
      UG_PutString(0, 0, sharedDataBuffer);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   if(getButtonPressed(buttonSelect)){
      writeArbitraryMemory8(HW_REG_ADDR(PKDATA), readArbitraryMemory8(HW_REG_ADDR(PKDATA)) ^ 0x10);
   }
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var watchPenIrq(){
   static Boolean firstRun = true;
   
   if(firstRun){
      firstRun = false;
      writeArbitraryMemory8(HW_REG_ADDR(PFDIR), readArbitraryMemory8(HW_REG_ADDR(PFDIR)) & 0xFD);
      writeArbitraryMemory8(HW_REG_ADDR(PFSEL), readArbitraryMemory8(HW_REG_ADDR(PFSEL)) | 0x02);
      debugSafeScreenClear(C_WHITE);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      writeArbitraryMemory8(HW_REG_ADDR(PFSEL), readArbitraryMemory8(HW_REG_ADDR(PFSEL)) & 0xFD);
      exitSubprogram();
   }
   
   StrPrintF(sharedDataBuffer, "PENIRQ State:%s", (readArbitraryMemory8(HW_REG_ADDR(PFDATA)) & 0x02) ? "true " : "false");/*"true " needs the space to clear the e from "false"*/
   UG_PutString(0, 0, sharedDataBuffer);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var watchIcr(){
   static Boolean firstRun = true;
   
   if(firstRun){
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      writeArbitraryMemory8(HW_REG_ADDR(PFSEL), readArbitraryMemory8(HW_REG_ADDR(PFSEL)) & 0xFD);
      exitSubprogram();
   }

   StrPrintF(sharedDataBuffer, "ICR:0x%02X", readArbitraryMemory16(HW_REG_ADDR(ICR)));
   UG_PutString(0, 0, sharedDataBuffer);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

