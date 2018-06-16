#include <PalmOS.h>
#include <stdint.h>

#include "testSuiteConfig.h"
#include "testSuite.h"
#include "tools.h"
#include "tests.h"
#include "debug.h"
#include "cpu.h"
#include "irda.h"
#include "ugui.h"


#define FONT_SPACING 0

#define RESERVED_PIXELS_Y (SCREEN_HEIGHT / 4)/*save pixels at the bottom of the screen for other information on the current item*/

#define TEXTBOX_PIXEL_MARGIN       1 /*add 1 pixel border around text*/
#define TEXTBOX_PIXEL_WIDTH        SCREEN_WIDTH
#define TEXTBOX_PIXEL_HEIGHT       (FONT_HEIGHT + (TEXTBOX_PIXEL_MARGIN * 2))

#define ITEM_LIST_ENTRYS ((SCREEN_HEIGHT - RESERVED_PIXELS_Y) / TEXTBOX_PIXEL_HEIGHT - 1)
#define ITEM_STRING_SIZE (TEXTBOX_PIXEL_WIDTH / (FONT_WIDTH + FONT_SPACING))

#define LIST_REFRESH       0
#define LIST_ITEM_SELECTED 1


/*tests*/
static test_t   hwTests[TESTS_AVAILABLE];
static uint32_t totalHwTests;

/*list handler variables*/
static int64_t  listLength;
static int64_t  selectedEntry;
static int64_t  lastSelectedEntry;
static char     itemStrings[ITEM_LIST_ENTRYS][ITEM_STRING_SIZE];
static Boolean  forceListRefresh;
static void     (*listHandler)(uint32_t command);

#define PAGE (selectedEntry / ITEM_LIST_ENTRYS)
#define INDEX (selectedEntry % ITEM_LIST_ENTRYS)
#define LAST_PAGE (lastSelectedEntry / ITEM_LIST_ENTRYS)
#define LAST_INDEX (lastSelectedEntry % ITEM_LIST_ENTRYS)

/*specific handler variables*/
static uint32_t bufferAddress;/*used to put the hex viewer in buffer mode instead of free roam*/


/*functions*/
CODE_SECTION("viewer") static void renderListFrame(){
   uint16_t textBoxes;
   uint16_t y = 0;
   
   setDebugTag("Rendering List Frame");
   
   if(forceListRefresh || PAGE != LAST_PAGE)
      debugSafeScreenClear(C_WHITE);
   
   for(textBoxes = 0; textBoxes < ITEM_LIST_ENTRYS; textBoxes++){
      if(textBoxes == INDEX){
         /*render list cursor inverted*/
         UG_SetBackcolor(C_BLACK);
         UG_SetForecolor(C_WHITE);
         UG_FillFrame(0, y, SCREEN_WIDTH - 1, y + TEXTBOX_PIXEL_HEIGHT - 1, C_BLACK);/*remove white lines between characters*/
         UG_PutString(0, y, itemStrings[textBoxes]);
         UG_SetBackcolor(C_WHITE);
         UG_SetForecolor(C_BLACK);
      }
      else if(textBoxes == LAST_INDEX){
         UG_FillFrame(0, y, SCREEN_WIDTH - 1, y + TEXTBOX_PIXEL_HEIGHT - 1, C_WHITE);/*remove black lines between characters*/
         UG_PutString(0, y, itemStrings[textBoxes]);
      }
      else if(forceListRefresh || PAGE != LAST_PAGE){
         /*render everything the first time or on page change, only render changes after that*/
         UG_PutString(0, y, itemStrings[textBoxes]);
      }
      
      y += TEXTBOX_PIXEL_HEIGHT;
   }
}

CODE_SECTION("viewer") static void hexHandler(uint32_t command){
   if(command == LIST_REFRESH){
      uint16_t i;
      uint32_t hexViewOffset = bufferAddress + PAGE * ITEM_LIST_ENTRYS;
      
      setDebugTag("Hex Handler Refresh");
      
      /*fill strings*/
      for(i = 0; i < ITEM_LIST_ENTRYS; i++){
         uint32_t displayAddress = hexViewOffset - bufferAddress;
         
         if(displayAddress + i < listLength){
            StrPrintF(itemStrings[i], "0x%08lX:0x%02X", displayAddress, readArbitraryMemory8(hexViewOffset));
            hexViewOffset++;
         }
         else{
            itemStrings[i][0] = '\0';
         }
      }
   }
}

CODE_SECTION("viewer") static void testPickerHandler(uint32_t command){
   if(command == LIST_REFRESH){
      uint16_t i;
      
      setDebugTag("Test Picker Handler Refresh");
      
      /*fill strings*/
      for(i = 0; i < ITEM_LIST_ENTRYS; i++){
         uint16_t item = i + PAGE * ITEM_LIST_ENTRYS;
         
         if(item < totalHwTests){
            StrNCopy(itemStrings[i], hwTests[item].name, ITEM_STRING_SIZE);
         }
         else{
            itemStrings[i][0] = '\0';
         }
      }
   }
   else if(command == LIST_ITEM_SELECTED){
      setDebugTag("Test Picker Test Selected");
      callSubprogram(hwTests[selectedEntry].testFunction);
   }
}

CODE_SECTION("viewer") static void resetListHandler(){
   uint16_t i;
   
   listLength = 0;
   selectedEntry = 0;
   lastSelectedEntry = 0;
   for(i = 0; i < ITEM_LIST_ENTRYS; i++)
      itemStrings[i][0] = '\0';
   forceListRefresh = true;
   
   setDebugTag("List Handler Reset");
}

CODE_SECTION("viewer") static var listModeFrame(){
   setDebugTag("List Mode Running");
   
   if(getButtonPressed(buttonUp))
      if(selectedEntry > 0)
         selectedEntry--;
   
   if(getButtonPressed(buttonDown))
      if(selectedEntry + 1 < listLength)
         selectedEntry++;
   
   if(getButtonPressed(buttonLeft) && listLength > ITEM_LIST_ENTRYS)
      if(selectedEntry - ITEM_LIST_ENTRYS >= 0)
         selectedEntry -= ITEM_LIST_ENTRYS;/*flip the page*/
      else
         selectedEntry = 0;
   
   if(getButtonPressed(buttonRight) && listLength > ITEM_LIST_ENTRYS)
      if(selectedEntry + ITEM_LIST_ENTRYS < listLength)
         selectedEntry += ITEM_LIST_ENTRYS;/*flip the page*/
      else
         selectedEntry = listLength - 1;
   
   if(getButtonPressed(buttonSelect))
      listHandler(LIST_ITEM_SELECTED);
   
   if(getButtonPressed(buttonBack))
      exitSubprogram();
   
   if(selectedEntry != lastSelectedEntry || forceListRefresh){
      listHandler(LIST_REFRESH);
      renderListFrame();
   }
   
   lastSelectedEntry = selectedEntry;
   
   if(forceListRefresh)
      forceListRefresh = false;
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var valueViewer(){
   static Boolean clearNeeded = true;
   var value = getSubprogramArgs();
   uint64_t varData = getVarValue(value);
   
   if(getButtonPressed(buttonBack)){
      clearNeeded = true;
      exitSubprogram();
   }
   
   if(clearNeeded){
      debugSafeScreenClear(C_WHITE);
      
      switch(getVarType(value)){
            
         case TYPE_UINT:
            StrPrintF(sharedDataBuffer, "uint32_t:0x%08lX", (uint32_t)varData);
            UG_PutString(0, 0, sharedDataBuffer);
            break;
            
         case TYPE_INT:
            StrPrintF(sharedDataBuffer, "int32_t:0x%08lX", (uint32_t)varData);
            UG_PutString(0, 0, sharedDataBuffer);
            break;
            
         case TYPE_PTR:
            StrPrintF(sharedDataBuffer, "pointer:0x%08lX", (uint32_t)varData);
            UG_PutString(0, 0, sharedDataBuffer);
            break;
            
         case TYPE_FLOAT:
            StrPrintF(sharedDataBuffer, "float:%f", *(float*)&varData);
            UG_PutString(0, 0, sharedDataBuffer);
            break;
            
         case TYPE_BOOL:
            if(varData)
               UG_PutString(0, 0, "Boolean:true");
            else
               UG_PutString(0, 0, "Boolean:false");
            break;
            
         default:
            UG_PutString(0, 0, "Unknown Var Type");
            break;
      }
      
      clearNeeded = false;
   }
}

var hexViewer(){
   var where = getSubprogramArgs();
   
   resetListHandler();
   listLength = getVarDataLength(where);
   if(listLength){
      /*displaying a result buffer*/
      selectedEntry = 0;
      bufferAddress = (uint32_t)getVarPointer(where);
   }
   else{
      /*free roam ram access*/
      selectedEntry = (uint32_t)getVarPointer(where);
      bufferAddress = 0;
      listLength = 0x100000000;
   }
   listHandler = hexHandler;
   execSubprogram(listModeFrame);
   
   setDebugTag("Hex Viewer Starting");
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var functionPicker(){
   resetListHandler();
   listLength = totalHwTests;
   listHandler = testPickerHandler;
   execSubprogram(listModeFrame);
   
   setDebugTag("Test Picker Starting");
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

void resetFunctionViewer(){
   var nullVar = makeVar(LENGTH_0, TYPE_NULL, 0);
   
   totalHwTests = 0;
   
   StrNCopy(hwTests[totalHwTests].name, "Ram Browser", TEST_NAME_LENGTH);
   hwTests[totalHwTests].testFunction = hexRamBrowser;
   totalHwTests++;
   
   StrNCopy(hwTests[totalHwTests].name, "Get Trap Address", TEST_NAME_LENGTH);
   hwTests[totalHwTests].testFunction = getTrapAddress;
   totalHwTests++;
   
   StrNCopy(hwTests[totalHwTests].name, "IrDa telnet/VNC", TEST_NAME_LENGTH);
   hwTests[totalHwTests].testFunction = irdaRun;
   totalHwTests++;
   
   if(getPhysicalCpuType() & CPU_M68K){
      /*68k only functions*/
      StrNCopy(hwTests[totalHwTests].name, "Dump Bootloader", TEST_NAME_LENGTH);
      hwTests[totalHwTests].testFunction = dumpBootloaderToFile;
      totalHwTests++;
      
      StrNCopy(hwTests[totalHwTests].name, "List P*DATA Registers", TEST_NAME_LENGTH);
      hwTests[totalHwTests].testFunction = listDataRegisters;
      totalHwTests++;
      
      StrNCopy(hwTests[totalHwTests].name, "Button Test", TEST_NAME_LENGTH);
      hwTests[totalHwTests].testFunction = testButtonInput;
      totalHwTests++;
      
      StrNCopy(hwTests[totalHwTests].name, "Get CLK32 Freq", TEST_NAME_LENGTH);
      hwTests[totalHwTests].testFunction = getClk32Frequency;
      totalHwTests++;
      
      StrNCopy(hwTests[totalHwTests].name, "Get Device ID", TEST_NAME_LENGTH);
      hwTests[totalHwTests].testFunction = getDeviceId;
      totalHwTests++;
      
      if(isM515){
         StrNCopy(hwTests[totalHwTests].name, "Toggle Backlight", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = toggleBacklight;
         totalHwTests++;
         
         StrNCopy(hwTests[totalHwTests].name, "Toggle Motor", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = toggleMotor;
         totalHwTests++;
      }
      
      if(unsafeMode){
         StrNCopy(hwTests[totalHwTests].name, "Manual LSSA", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = manualLssa;
         totalHwTests++;
         
         StrNCopy(hwTests[totalHwTests].name, "TSTAT1 Semaphore Info", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = tstat1GetSemaphoreLockOrder;
         totalHwTests++;
         
         StrNCopy(hwTests[totalHwTests].name, "ADS7846 Read", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = ads7846Read;
         totalHwTests++;
         
         StrNCopy(hwTests[totalHwTests].name, "Interrogate SPI2", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = interrogateSpi2;
         totalHwTests++;
         
         StrNCopy(hwTests[totalHwTests].name, "Self Probe SPI2", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = selfProbeSpi2;
         totalHwTests++;
         
         StrNCopy(hwTests[totalHwTests].name, "Exit Unsafe Mode", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = exitUnsafeMode;
         totalHwTests++;
      }
      else{
         StrNCopy(hwTests[totalHwTests].name, "Enter Unsafe Mode", TEST_NAME_LENGTH);
         hwTests[totalHwTests].testFunction = enterUnsafeMode;
         totalHwTests++;
      }
   }
}
