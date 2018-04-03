#include <stdio.h>
#include <string.h>
#include "testSuiteConfig.h"
#include "testSuite.h"
#include "snprintf.h"
#include "tests.h"
#include "ugui.h"


#define MAX_OBJECTS 50

#define FONT_WIDTH   8
#define FONT_HEIGHT  8
#define FONT_SPACING 0

#define RESERVED_PIXELS_Y (SCREEN_HEIGHT / 4)/*save pixels at the bottom of the screen for other information on the current item*/

#define TEXTBOX_PIXEL_MARGIN       1 /*add 1 pixel border around text*/
#define TEXTBOX_PIXEL_WIDTH        SCREEN_WIDTH
#define TEXTBOX_PIXEL_HEIGHT       (FONT_HEIGHT + (TEXTBOX_PIXEL_MARGIN * 2))
#define TEXTBOX_DEFAULT_COLOR      C_WHITE
#define TEXTBOX_DEFAULT_TEXT_COLOR C_BLACK
#define TEXTBOX_CURSOR_COLOR       C_BLACK
#define TEXTBOX_CURSOR_TEXT_COLOR  C_WHITE

#define ITEM_LIST_ENTRYS ((SCREEN_HEIGHT - RESERVED_PIXELS_Y) / TEXTBOX_PIXEL_HEIGHT - 1)
#define ITEM_STRING_SIZE (TEXTBOX_PIXEL_WIDTH / (FONT_WIDTH + FONT_SPACING))

#define LIST_REFRESH       0
#define LIST_ITEM_SELECTED 1


/*tests*/
static test_t hwTests[TESTS_AVAILABLE];
static uint32_t      totalHwTests;

/*graphics*/
static UG_GUI     fbGui;
static UG_WINDOW  fbWindow;
static UG_OBJECT  fbObjects[MAX_OBJECTS];
static UG_TEXTBOX listEntrys[ITEM_LIST_ENTRYS];
static char       textboxString[ITEM_LIST_ENTRYS][ITEM_STRING_SIZE];

/*list handler variables*/
static uint32_t page = 0;
static uint32_t index = 0;
static uint32_t lastIndex = 0;
static uint32_t lastPage  = 0;
static uint32_t listLength = 0;
static uint32_t selectedEntry = 0;
static Boolean  forceListRefresh = true;
static void     (*listHandler)(uint32_t command);

/*specific handler variables*/
static uint32_t bufferAddress;/*used to put the hex viewer in buffer mode instead of free roam*/


/*functions*/
static void windowCallback(UG_MESSAGE* message){
   /*do nothing*/
}

static void forceTextEntryRefresh(uint32_t box){
   UG_TextboxSetText(&fbWindow, 0, textboxString[0]);
}

static void hexHandler(uint32_t command){
   if(command == LIST_REFRESH){
      int i;
      uint32_t hexViewOffset = bufferAddress + selectedEntry;
      
      /*fill strings*/
      for(i = 0; i < ITEM_LIST_ENTRYS; i++){
         snprintf(textboxString[i], ITEM_STRING_SIZE, "0x%08X:0x%02X", hexViewOffset - bufferAddress, readArbitraryMemory8(hexViewOffset));
         forceTextEntryRefresh(i);
         hexViewOffset++;
      }
   }
}

static void testPickerHandler(uint32_t command){
   if(command == LIST_REFRESH){
      int i;
      
      /*fill strings*/
      for(i = 0; i < ITEM_LIST_ENTRYS; i++){
         if(i < totalHwTests){
            strncpy(textboxString[i], hwTests[i].name, ITEM_STRING_SIZE);
         }
         else{
            strncpy(textboxString[i], "Test List Overflow", ITEM_STRING_SIZE);
         }
         forceTextEntryRefresh(i);
      }
   }
   else if(command == LIST_ITEM_SELECTED){
      callSubprogram(hwTests[selectedEntry].testFunction);
   }
}

static void resetListHandler(){
   page = 0;
   index = 0;
   lastIndex = 0;
   lastPage  = 0;
   listLength = 0;
   selectedEntry = 0;
   forceListRefresh = true;
}

static var listModeFrame(){
   if (getButtonPressed(buttonUp)){
      if(selectedEntry > 0)
         selectedEntry--;
   }
   
   if (getButtonPressed(buttonDown)){
      if(selectedEntry + 1 < listLength)
         selectedEntry++;
   }
   
   if (getButtonPressed(buttonLeft) && listLength > ITEM_LIST_ENTRYS){
      if(selectedEntry - ITEM_LIST_ENTRYS >= 0)
         selectedEntry -= ITEM_LIST_ENTRYS;/*flip the page*/
      else
         selectedEntry = 0;
   }
   
   if (getButtonPressed(buttonRight) && listLength > ITEM_LIST_ENTRYS){
      if(selectedEntry + ITEM_LIST_ENTRYS < listLength)
         selectedEntry += ITEM_LIST_ENTRYS;/*flip the page*/
      else
         selectedEntry = listLength - ITEM_LIST_ENTRYS - 1;
   }
   
   page  = selectedEntry / ITEM_LIST_ENTRYS;
   index = selectedEntry % ITEM_LIST_ENTRYS;
   
   if ((page != lastPage || index != lastIndex || forceListRefresh) && listHandler){
      listHandler(LIST_REFRESH);
   }
   
   if (getButtonPressed(buttonSelect)){
      /*select*/
      listHandler(LIST_ITEM_SELECTED);
   }
   
   if (getButtonPressed(buttonBack)){
      /*go back*/
      exitSubprogram();
   }
   
   
   if (index != lastIndex)
   {
      /*update item colors*/
      UG_TextboxSetForeColor(&fbWindow, lastIndex, TEXTBOX_DEFAULT_TEXT_COLOR);
      UG_TextboxSetBackColor(&fbWindow, lastIndex, TEXTBOX_DEFAULT_COLOR);
      
      UG_TextboxSetForeColor(&fbWindow, index, TEXTBOX_CURSOR_TEXT_COLOR);
      UG_TextboxSetBackColor(&fbWindow, index, TEXTBOX_CURSOR_COLOR);
   }
   else if(forceListRefresh){
      UG_TextboxSetForeColor(&fbWindow, index, TEXTBOX_CURSOR_TEXT_COLOR);
      UG_TextboxSetBackColor(&fbWindow, index, TEXTBOX_CURSOR_COLOR);
   }
   
   lastIndex = index;
   lastPage  = page;
   
   UG_Update();
   
   if(forceListRefresh)
      forceListRefresh = false;
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

static Boolean initViewer(){
   int newTextboxY;
   int entry;
   UG_RESULT passed;
   
   passed = UG_WindowCreate(&fbWindow, fbObjects, MAX_OBJECTS, windowCallback);
   if (passed != UG_RESULT_OK)
      return false;
   
   UG_WindowSetStyle(&fbWindow, WND_STYLE_HIDE_TITLE | WND_STYLE_2D);
   UG_WindowSetForeColor(&fbWindow, TEXTBOX_DEFAULT_TEXT_COLOR);
   UG_WindowSetBackColor(&fbWindow, TEXTBOX_DEFAULT_COLOR);
   
   newTextboxY = 0;
   for (entry = 0; entry < ITEM_LIST_ENTRYS; entry++)
   {
      UG_TextboxCreate(&fbWindow, &listEntrys[entry], entry, 0/*x start*/, newTextboxY, TEXTBOX_PIXEL_WIDTH - 1/*x end*/, newTextboxY + TEXTBOX_PIXEL_HEIGHT - 1);
      UG_TextboxSetAlignment(&fbWindow, entry, ALIGN_CENTER);
      UG_TextboxSetText(&fbWindow, entry, textboxString[entry]);
      UG_TextboxShow(&fbWindow, entry);
      newTextboxY += TEXTBOX_PIXEL_HEIGHT;
   }
   
   /*set_main_window*/
   UG_WindowShow(&fbWindow);
   return true;
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
      listLength = 0xFFFFFFFF;
   }
   listHandler = hexHandler;
   execSubprogram(listModeFrame);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var testPicker(){
   resetListHandler();
   listLength = totalHwTests;
   listHandler = testPickerHandler;
   execSubprogram(listModeFrame);
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

Boolean initTestList(){
   var nullVar = makeVar(LENGTH_0, TYPE_NULL, 0);
   Boolean memoryAllocSuccess = initViewer();
   
   if(!memoryAllocSuccess){
      return false;
   }
   
   totalHwTests = 0;
   
   strncpy(hwTests[0].name, "Ram Browser", 32);
   hwTests[0].isSimpleTest = false;
   hwTests[0].expectedResult = nullVar;
   hwTests[0].testFunction = hexRamBrowser;
   totalHwTests++;
   
   return true;
}
