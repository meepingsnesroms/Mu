#include "testSuiteConfig.h"
#include "testSuite.h"
#include "tools.h"
#include "tests.h"
#include "debug.h"
#include "ugui.h"


#define FONT_WIDTH   8
#define FONT_HEIGHT  8
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
static uint32_t page;
static uint32_t index;
static int64_t  listLength;
static int64_t  lastSelectedEntry;
static int64_t  selectedEntry;
static char     itemStrings[ITEM_LIST_ENTRYS][ITEM_STRING_SIZE];
static Boolean  forceListRefresh;
static void     (*listHandler)(uint32_t command);

/*specific handler variables*/
static uint32_t bufferAddress;/*used to put the hex viewer in buffer mode instead of free roam*/


/*functions*/
static void renderListFrame(){
   int textBoxes;
   int y = 0;
   
   setDebugTag("Rendering List Frame");
   
   debugSafeScreenClear(C_WHITE);
   for(textBoxes = 0; textBoxes < ITEM_LIST_ENTRYS; textBoxes++){
      if(textBoxes == index){
         /*render list cursor inverted*/
         UG_SetBackcolor(C_BLACK);
         UG_SetForecolor(C_WHITE);
         UG_FillFrame(0, y, SCREEN_WIDTH - 1, y + TEXTBOX_PIXEL_HEIGHT - 1, C_BLACK);/*remove white lines between characters*/
         UG_PutString(0, y, itemStrings[textBoxes]);
         UG_SetBackcolor(C_WHITE);
         UG_SetForecolor(C_BLACK);
      }
      else{
         /*just the text*/
         UG_PutString(0, y, itemStrings[textBoxes]);
      }
      
      y += TEXTBOX_PIXEL_HEIGHT;
   }
}

static void hexHandler(uint32_t command){
   if(command == LIST_REFRESH){
      int i;
      uint32_t hexViewOffset = bufferAddress + page * ITEM_LIST_ENTRYS;
      
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

static void testPickerHandler(uint32_t command){
   if(command == LIST_REFRESH){
      int i;
      
      setDebugTag("Test Picker Handler Refresh");
      
      /*fill strings*/
      for(i = 0; i < ITEM_LIST_ENTRYS; i++){
         if(i < totalHwTests){
            StrNCopy(itemStrings[i], hwTests[i].name, ITEM_STRING_SIZE);
         }
         else{
            itemStrings[i][0] = '\0';
         }
      }
   }
   else if(command == LIST_ITEM_SELECTED){
      setDebugTag("Test Picker Test Selected");
      execSubprogram(hwTests[selectedEntry].testFunction);
   }
}

static void resetListHandler(){
   int i;
   
   page = 0;
   index = 0;
   listLength = 0;
   lastSelectedEntry = 0;
   selectedEntry = 0;
   for(i = 0; i < ITEM_LIST_ENTRYS; i++)
      itemStrings[i][0] = '\0';
   forceListRefresh = true;
   debugSafeScreenClear(C_WHITE);
   
   setDebugTag("List Handler Reset");
}

static var listModeFrame(){
   
   setDebugTag("List Mode Running");
   
   if(getButtonPressed(buttonUp)){
      if(selectedEntry > 0)
         selectedEntry--;
   }
   
   if(getButtonPressed(buttonDown)){
      if(selectedEntry + 1 < listLength)
         selectedEntry++;
   }
   
   if(getButtonPressed(buttonLeft) && listLength > ITEM_LIST_ENTRYS){
      if(selectedEntry - ITEM_LIST_ENTRYS >= 0)
         selectedEntry -= ITEM_LIST_ENTRYS;/*flip the page*/
      else
         selectedEntry = 0;
   }
   
   if(getButtonPressed(buttonRight) && listLength > ITEM_LIST_ENTRYS){
      if(selectedEntry + ITEM_LIST_ENTRYS < listLength)
         selectedEntry += ITEM_LIST_ENTRYS;/*flip the page*/
      else
         selectedEntry = listLength - 1;
   }
   
   if(getButtonPressed(buttonSelect)){
      /*select*/
      listHandler(LIST_ITEM_SELECTED);
   }
   
   if(getButtonPressed(buttonBack)){
      /*go back*/
      exitSubprogram();
   }
   
   page  = selectedEntry / ITEM_LIST_ENTRYS;
   index = selectedEntry % ITEM_LIST_ENTRYS;
   
   if(selectedEntry != lastSelectedEntry || forceListRefresh){
      listHandler(LIST_REFRESH);
   }
   
   if(selectedEntry != lastSelectedEntry || forceListRefresh){
      /*update item colors*/
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
      /*go back*/
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
            
            /*
         case TYPE_FLOAT:
            StrPrintF(sharedDataBuffer, "float:%f", *(float*)(&varData + 4));
            UG_PutString(0, 0, sharedDataBuffer);
            break;
            */
            
         case TYPE_BOOL:
            if(varData)
               UG_PutString(0, 0, "Bool:true");
            else
               UG_PutString(0, 0, "Bool:false");
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
      listLength = UINT64_C(0x100000000);
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

void initViewer(){
   var nullVar = makeVar(LENGTH_0, TYPE_NULL, 0);
   
   totalHwTests = 0;
   
   StrNCopy(hwTests[totalHwTests].name, "Ram Browser", TEST_NAME_LENGTH);
   hwTests[totalHwTests].testFunction = hexRamBrowser;
   totalHwTests++;
   
   StrNCopy(hwTests[totalHwTests].name, "Get Trap Address", TEST_NAME_LENGTH);
   hwTests[totalHwTests].testFunction = getTrapAddress;
   totalHwTests++;
   
   StrNCopy(hwTests[totalHwTests].name, "Dump Bootloader", TEST_NAME_LENGTH);
   hwTests[totalHwTests].testFunction = dumpBootloaderToFile;
   totalHwTests++;
   
   StrNCopy(hwTests[totalHwTests].name, "File Access Test", TEST_NAME_LENGTH);
   hwTests[totalHwTests].testFunction = testFileAccessWorks;
   totalHwTests++;
}
