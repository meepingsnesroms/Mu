#include "testSuiteConfig.h"
#include "testSuite.h"
#include "ugui.h"


#define FONT_WIDTH   8
#define FONT_HEIGHT  8
#define FONT_SPACING 0

#define MAX_OBJECTS          50

#define TEXTBOX_PIXEL_MARGIN 1 //add 1 pixel border around text
#define TEXTBOX_PIXEL_WIDTH  (SCREEN_WIDTH / 4 * 3) //3/4 of the screen
#define TEXTBOX_PIXEL_HEIGHT (FONT_HEIGHT + (TEXTBOX_PIXEL_MARGIN * 2))
#define TEXTBOX_DEFAULT_COLOR C_WHITE
#define TEXTBOX_DEFAULT_TEXT_COLOR C_BLACK
#define TEXTBOX_CURSOR_COLOR C_BLACK
#define TEXTBOX_CURSOR_TEXT_COLOR C_WHITE

#define ITEM_listEntrys (SCREEN_HEIGHT / TEXTBOX_PIXEL_HEIGHT - 1)
#define ITEM_STRING_SIZE (TEXTBOX_PIXEL_WIDTH / (FONT_WIDTH + FONT_SPACING))


bool       viewerInitialized = false;
int32_t    listLength;
int32_t    selectedEntry;

UG_GUI     fbGui;
UG_WINDOW  fbWindow;
UG_OBJECT  fbObjects[MAX_OBJECTS];
UG_TEXTBOX listEntrys[ITEM_LIST_ENTRYS];
char       textboxString[ITEM_listEntrys][ITEM_STRING_SIZE];


static void listModeFrame(){
   int32_t page;
   int32_t index;
   static int32_t lastIndex = 0;
   static int32_t lastPage  = 0;
   
   if (getButtonPressed(buttonUp)){
      if(selectedEntry - 1 >= 0)
         selectedEntry--;
   }
   
   if (getButtonPressed(buttonDown)){
      if(selectedEntry + 1 < listLength)
         selectedEntry++;
   }
   
   if (getButtonPressed(buttonLeft) && listLength > ITEM_listEntrys){
      if(selectedEntry - ITEM_listEntrys >= 0)
         selectedEntry -= ITEM_listEntrys;//flip the page
      else
         selectedEntry = 0;
   }
   
   if (getButtonPressed(buttonRight) && listLength > ITEM_listEntrys){
      if(selectedEntry + ITEM_listEntrys < listLength)
         selectedEntry += ITEM_listEntrys;//flip the page
      else
         selectedEntry = listLength - 1;
   }
   
   page  = selectedEntry / ITEM_listEntrys;
   index = selectedEntry % ITEM_listEntrys;
   
   /*
   if (page != last_page || index != last_index && list_handler){
      listLength = list_handler(page * ITEM_listEntrys);
   }
   */
   
   if (getButtonPressed(buttonSelect)){
      //select
      //run_action(list_index_action[index]);
      //nothing yet
   }
   
   if (getButtonPressed(buttonBack)){
      //go back
      exitSubprogram();
   }
   
   
   if (index != lastIndex)
   {
      //update item colors
      UG_TextboxSetForeColor(&fbWindow, lastIndex, TEXTBOX_DEFAULT_TEXT_COLOR);
      UG_TextboxSetBackColor(&fbWindow, lastIndex, TEXTBOX_DEFAULT_COLOR);
      
      UG_TextboxSetForeColor(&fbWindow, index, TEXTBOX_CURSOR_TEXT_COLOR);
      UG_TextboxSetBackColor(&fbWindow, index, TEXTBOX_CURSOR_COLOR);
   }
   
   lastIndex = index;
   lastPage  = page;
   
   UG_Update();
}


var initViewer(){
   UG_RESULT passed = UG_WindowCreate(&fbWindow, fbObjects, MAX_OBJECTS, windowCallback);
   if (passed != UG_RESULT_OK)
      return {LENGTH_1 | TYPE_BOOL, false};
   
   UG_WindowSetStyle(&fbWindow, WND_STYLE_HIDE_TITLE | WND_STYLE_2D);
   UG_WindowSetForeColor(&fbWindow, TEXTBOX_DEFAULT_TEXT_COLOR);
   UG_WindowSetBackColor(&fbWindow, TEXTBOX_DEFAULT_COLOR);
   
   int newTextboxY = 0;
   for (int entry = 0; entry < ITEM_listEntrys; entry++)
   {
      UG_TextboxCreate(&fbWindow, &listEntrys[entry], entry, 0/*x start*/, newTextboxY, TEXTBOX_PIXEL_WIDTH - 1/*x end*/, newTextboxY + TEXTBOX_PIXEL_HEIGHT - 1);
      UG_TextboxSetAlignment(&fbWindow, entry, ALIGN_CENTER);
      UG_TextboxSetText(&fbWindow, entry, textboxString[entry]);
      UG_TextboxShow(&fbWindow, entry);
      newTextboxY += TEXTBOX_PIXEL_HEIGHT;
   }
   //set_main_window
   UG_WindowShow(&fbWindow);
   return {LENGTH_1 | TYPE_BOOL, true};
}


var hexViewer(){
   if(!viewerInitialized){
      var passFail = initViewer();
      if(passFail != makeVar(LENGTH_1, TYPE_BOOL, true)){
         callSubprogram(memoryAllocationError);
         return makeVar(LENGTH_1, TYPE_BOOL, false);
      }
      viewerInitialized = true;
   }
   
   var where = getSubprogramArgs();
   if(getVarType(where) == TYPE_POINTER){
      
   }
   
   //make hex value list
   
   listModeFrame();
   
   return makeVar(LENGTH_1, TYPE_BOOL, true);
}

var testViewer(){
   if(!viewerInitialized){
      var passFail = initViewer();
      if(passFail != makeVar(LENGTH_1, TYPE_BOOL, true)){
         callSubprogram(memoryAllocationError);
         return makeVar(LENGTH_1, TYPE_BOOL, false);
      }
      viewerInitialized = true;
   }
   
   //make test list
   
   listModeFrame();
   
   return makeVar(LENGTH_1, TYPE_BOOL, true);
}
