#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <stdint.h>
#include "ugui.h"
#include "testSuiteConfig.h"
#include "testSuite.h"
#include "viewer.h"
#include "debug.h"
#include "tools.h"

/*dont include this anywhere else*/
#include "TstSuiteRsc.h"


var makeVar(uint8_t length, uint8_t type, uint64_t value){
   var newVar;
   newVar.type = (length & 0xF0) | (type & 0x0F);
   newVar.value = value;
   return newVar;
}

uint8_t readArbitraryMemory8(uint32_t address){
   return *((uint8_t*)address);
}
uint16_t readArbitraryMemory16(uint32_t address){
   return *((uint16_t*)address);
}
uint32_t readArbitraryMemory32(uint32_t address){
   return *((uint32_t*)address);
}
void writeArbitraryMemory8(uint32_t address, uint8_t value){
   *((uint8_t*)address) = value;
}
void writeArbitraryMemory16(uint32_t address, uint16_t value){
   *((uint16_t*)address) = value;
}
void writeArbitraryMemory32(uint32_t address, uint32_t value){
   *((uint32_t*)address) = value;
}


/*exports*/
uint16_t palmButtons;
uint16_t palmButtonsLastFrame;
Boolean  unsafeMode;
uint8_t* sharedDataBuffer;


/*video stuff*/
static UG_GUI      uguiStruct;
static BitmapType* offscreenBitmap;
static uint8_t*    framebuffer;

/*other*/
static activity_t parentSubprograms[MAX_SUBPROGRAMS];
static uint32_t   subprogramIndex;
static activity_t currentSubprogram;
static var        subprogramData[MAX_SUBPROGRAMS];
static var        subprogramArgs;/*optional arguments when one subprogram calls another*/
static var        lastSubprogramReturnValue;
static Boolean    subprogramArgsSet;
static Boolean    applicationRunning;


static var errorSubprogramStackOverflow(){
   static Boolean wipedScreen = false;
   if(!wipedScreen){
      debugSafeScreenClear(C_WHITE);
      UG_PutString(0, 0, "Subprogram stack overflow!\nYou must close the program.");
      wipedScreen = true;
   }
   if(getButtonPressed(buttonBack)){
      /*force kill when back pressed*/
      applicationRunning = false;
   }
   /*do nothing, this is a safe crash*/
}

var memoryAllocationError(){
   static Boolean wipedScreen = false;
   if(!wipedScreen){
      debugSafeScreenClear(C_WHITE);
      UG_PutString(0, 0, "Could not allocate memory!\nYou must close the program.");
      wipedScreen = true;
   }
   if(getButtonPressed(buttonBack)){
      /*force kill when back pressed*/
      applicationRunning = false;
   }
   /*do nothing, this is a safe crash*/
}


static void uguiDrawPixel(UG_S16 x, UG_S16 y, UG_COLOR color){
   /*using 1bit grayscale*/
   int pixel = x + y * SCREEN_WIDTH;
   int byte = pixel / 8;
   int bit = pixel % 8;
   
   /*ugui will call this function even if its over the screen bounds, dont let those writes through*/
   if(pixel > SCREEN_WIDTH * SCREEN_HEIGHT - 1)
      return;
   
   if(!color){
      /*1 is black not white*/
      framebuffer[byte] |= (1 << (7 - bit));
   }
   else{
      framebuffer[byte] &= ~(1 << (7 - bit));
   }
}

void forceFrameRedraw(){
   WinDrawBitmap(offscreenBitmap, 0, 0);
}

void callSubprogram(activity_t activity){
   if(subprogramIndex < MAX_SUBPROGRAMS - 1){
      subprogramIndex++;
      parentSubprograms[subprogramIndex] = activity;
      currentSubprogram = activity;
      if(!subprogramArgsSet)
         subprogramArgs = makeVar(LENGTH_0, TYPE_NULL, 0);
      subprogramArgsSet = false;/*clear to prevent next subprogram called from inheriting the args*/
      setDebugTag("Subprogram Called");
   }
   else{
      currentSubprogram = errorSubprogramStackOverflow;
      /*cant recover from this*/
   }
}

void exitSubprogram(){
   if(subprogramIndex > 0){
      subprogramIndex--;
      currentSubprogram = parentSubprograms[subprogramIndex];
      setDebugTag("Subprogram Exited");
   }
   else{
      /*last subprogram is complete, exit application*/
      applicationRunning = false;
      setDebugTag("Application Exiting");
   }
}

void execSubprogram(activity_t activity){
   if(!subprogramArgsSet)
      subprogramArgs = makeVar(LENGTH_0, TYPE_NULL, 0);
   subprogramArgsSet = false;/*clear to prevent next subprogram called from inheriting the args*/
   parentSubprograms[subprogramIndex] = activity;
   currentSubprogram = activity;
   setDebugTag("Subprogram Swapped Out");
}

var getSubprogramReturnValue(){
   return lastSubprogramReturnValue;
}

var getSubprogramArgs(){
   return subprogramArgs;
}

void setSubprogramArgs(var args){
   subprogramArgs = args;
   subprogramArgsSet = true;
}

var subprogramGetData(){
   return subprogramData[subprogramIndex];
}

void subprogramSetData(var data){
   subprogramData[subprogramIndex] = data;
}

static Boolean testerInit(){
   long     osVer;
   Err      error;
   
   FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osVer);
   if (osVer < PalmOS35) {
      FrmCustomAlert(alt_err, "TestSuite requires at least PalmOS 3.5", 0, 0);
      return false;
   }
   
   sharedDataBuffer = MemPtrNew(SHARED_DATA_BUFFER_SIZE);
   if(sharedDataBuffer == NULL){
      FrmCustomAlert(alt_err, "Cant create memory buffer", 0, 0);
      return false;
   }
   
   offscreenBitmap = BmpCreate(SCREEN_WIDTH, SCREEN_HEIGHT, 1, NULL, &error);
   if(error){
      FrmCustomAlert(alt_err, "Cant create bitmap", 0, 0);
      return false;
   }
   
   KeySetMask(~(keyBitPageUp | keyBitPageDown | keyBitHard1  | keyBitHard2 | keyBitHard3  | keyBitHard4 ));
   
   framebuffer = BmpGetBits(offscreenBitmap);
   WinSetActiveWindow(WinGetDisplayWindow());
   
   UG_Init(&uguiStruct, uguiDrawPixel, SCREEN_WIDTH, SCREEN_HEIGHT);
   UG_FontSelect(&SELECTED_FONT);
   UG_SetBackcolor(C_WHITE);
   UG_SetForecolor(C_BLACK);
   UG_ConsoleSetBackcolor(C_WHITE);
   UG_ConsoleSetForecolor(C_BLACK);
   UG_FillScreen(C_WHITE);
   
   /*make function list*/
   resetFunctionViewer();
   
   /*load first subprogram*/
   unsafeMode = false;
   subprogramIndex = 0;
   subprogramArgsSet = false;
   lastSubprogramReturnValue = makeVar(LENGTH_0, TYPE_NULL, 0);
   subprogramArgs = makeVar(LENGTH_0, TYPE_NULL, 0);
   currentSubprogram = functionPicker;
   
   return true;
}

static void testerExit(){
   MemPtrFree(sharedDataBuffer);
}

static void testerFrameLoop(){
   palmButtons = KeyCurrentState();
   
   if(!unsafeMode){
      /*allow exiting the app normally and prevent filling up the event loop*/
      EventType event;
      do{
         EvtGetEvent(&event, 1);
         SysHandleEvent(&event);
         if(event.eType == appStopEvent){
            applicationRunning = false;
            break;
         }
      }
      while(event.eType != nilEvent);
   }
   
   lastSubprogramReturnValue = currentSubprogram();
   
   WinDrawBitmap(offscreenBitmap, 0, 0);
   
   palmButtonsLastFrame = palmButtons;
}

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags){
   if(cmd == sysAppLaunchCmdNormalLaunch){
      Boolean initSuccess;
      initSuccess = testerInit();
      
      if(!initSuccess){
         return(0);
      }
      
      applicationRunning = true;
      while(applicationRunning){
         testerFrameLoop();
         SysTaskDelay(4);/*30 fps*/
      }
      
      testerExit();
   }
   else if(cmd == sysAppLaunchCmdSystemReset){
 #ifdef DEBUG
      /*app crashed, to ease development just self delete from internal storage*/
      /*
      LocalID whatToDelete;
      whatToDelete = DmFindDatabase(0, "HWTests");
      DmDeleteDatabase(0, whatToDelete);
      SysReset();
      */
 #endif
   }
   return(0);
}

