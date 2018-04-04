#include <PalmOS.h>
#include "testSuiteConfig.h"
#include "testSuite.h"
#include "ugui.h"

#ifdef DEBUG

#define DEBUG_AREA_SIZE   10
#define DEBUG_STRING_SIZE 100


static char debugTag[DEBUG_STRING_SIZE];


void setDebugTag(char* tag){
   StrNCopy(debugTag, tag, DEBUG_STRING_SIZE);
   UG_PutString(0, SCREEN_HEIGHT - DEBUG_AREA_SIZE, debugTag);
   forceFrameRedraw();/*without this any crashes during a frame wont get reported*/
}

void debugSafeScreenClear(UG_COLOR c){
   UG_FillScreen(c);
   UG_PutString(0, SCREEN_HEIGHT - DEBUG_AREA_SIZE, debugTag);
}
#endif
