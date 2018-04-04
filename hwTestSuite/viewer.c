#include "testSuiteConfig.h"
#include "testSuite.h"
#include "tests.h"
#include "debug.h"
#include "ugui.h"


#define MAX_OBJECTS 50

#define FONT_WIDTH   4
#define FONT_HEIGHT  6
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
static uint32_t lastIndex;
static uint32_t lastPage;
static uint32_t listLength;
static uint32_t selectedEntry;
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
   
   for(textBoxes = 0; textBoxes < ITEM_LIST_ENTRYS; textBoxes++){
      if(textBoxes == index){
         /*render list cursor inverted*/
         UG_SetBackcolor(C_BLACK);
         UG_SetForecolor(C_WHITE);
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
      uint32_t hexViewOffset = bufferAddress + selectedEntry;
      
      setDebugTag("Hex Handler Refresh");
      
      /*fill strings*/
      for(i = 0; i < ITEM_LIST_ENTRYS; i++){
         StrPrintF(itemStrings[i], "0x%08X:0x%02X", hexViewOffset - bufferAddress, readArbitraryMemory8(hexViewOffset));
         hexViewOffset++;
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
            StrNCopy(itemStrings[i], "Test List Overflow", ITEM_STRING_SIZE);
         }
      }
   }
   else if(command == LIST_ITEM_SELECTED){
      setDebugTag("Test Picker Test Selected");
      callSubprogram(hwTests[selectedEntry].testFunction);
   }
}

static void resetListHandler(){
   int i;
   
   page = 0;
   index = 0;
   lastIndex = 0;
   lastPage  = 0;
   listLength = 0;
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
         selectedEntry = listLength - ITEM_LIST_ENTRYS - 1;
   }
   
   page  = selectedEntry / ITEM_LIST_ENTRYS;
   index = selectedEntry % ITEM_LIST_ENTRYS;
   
   if((page != lastPage || index != lastIndex || forceListRefresh) && listHandler){
      listHandler(LIST_REFRESH);
   }
   
   if(getButtonPressed(buttonSelect)){
      /*select*/
      listHandler(LIST_ITEM_SELECTED);
   }
   
   if(getButtonPressed(buttonBack)){
      /*go back*/
      exitSubprogram();
   }
   
   
   if(index != lastIndex || forceListRefresh){
      /*update item colors*/
      renderListFrame();
   }
   
   lastIndex = index;
   lastPage  = page;
   
   /*UG_PutString(0, 50, "subprogram exec worked");*/
   
   if(forceListRefresh)
      forceListRefresh = false;
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var hexViewer(){
   var where = getSubprogramArgs();
   
   resetListHandler();
   listLength = getVarPointerSize(where);
   if(listLength){
      /*displaying a result buffer*/
      selectedEntry = 0;
      bufferAddress = (uint32_t)getVarPointer(where);
   }
   else{
      /*free roam ram access*/
      selectedEntry = (uint32_t)getVarPointer(where);
      bufferAddress = 0;
      listLength = UINT32_C(0xFFFFFFFF);
   }
   listHandler = hexHandler;
   execSubprogram(listModeFrame);
   
   setDebugTag("Hex Viewer Starting");
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var testPicker(){
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
   
   StrNCopy(hwTests[0].name, "Ram Browser", TEST_NAME_LENGTH);
   hwTests[0].isSimpleTest = false;
   hwTests[0].expectedResult = nullVar;
   hwTests[0].testFunction = hexRamBrowser;
   totalHwTests++;
}
