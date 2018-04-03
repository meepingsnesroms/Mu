#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <stdint.h>
#include "ugui.h"
#include "testSuiteConfig.h"
#include "testSuite.h"
#include "viewer.h"

/*dont include this anywhere else*/
#include "TstSuiteRsc.h"


#define PalmOS35   sysMakeROMVersion(3,5,0,sysROMStageRelease,0)


/*functions that should be macros but are screwed up by c89*/
uint8_t getVarType(var thisVar){
   return thisVar.type & 0x0F;
}
uint8_t getVarLength(var thisVar){
   return thisVar.type & 0xF0;
}
uint32_t getVarDataLength(var thisVar){
   return thisVar.value >> 32;
}
void* getVarPointer(var thisVar){
   return (void*)(uint32_t)(thisVar.value & 0xFFFFFFFF);
}
uint32_t getVarPointerSize(var thisVar){
   return thisVar.value >> 32;
}
var_value getVarValue(var thisVar){
   return thisVar.value;
}
var makeVar(uint8_t length, uint8_t type, uint64_t value){
   var newVar;
   newVar.type = (length & 0xF0) | (type & 0x0F);
   newVar.value = value;
}
Boolean varsEqual(var var1, var var2){
   if(var1.type == var2.type && var1.value == var2.value)
      return true;
   return false;
}

Boolean getButton(uint16_t button){
   return (palmButtons & button) != 0;
}
Boolean getButtonLastFrame(uint16_t button){
   return (palmButtonsLastFrame & button) != 0;
}
Boolean getButtonChanged(uint16_t button){
   return (palmButtons & button) != (palmButtonsLastFrame & button);
}
Boolean getButtonPressed(uint16_t button){
   return (palmButtonsLastFrame & button) && !(palmButtonsLastFrame & button);
}
Boolean getButtonReleased(uint16_t button){
   return !(palmButtonsLastFrame & button) && (palmButtonsLastFrame & button);
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


/*video stuff*/
static UG_GUI   uguiStruct;
static uint8_t* framebuffer;

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
      UG_FillScreen(C_WHITE);
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
      UG_FillScreen(C_WHITE);
      UG_PutString(0, 0, "Could not allocate memory!\nYou must close the program.");
      wipedScreen = true;
   }
   if(getButtonPressed(buttonBack)){
      /*force kill when back pressed*/
      applicationRunning = false;
   }
   /*do nothing, this is a safe crash*/
}


void uguiDrawPixel(UG_S16 x, UG_S16 y, UG_COLOR color){
   /*using 1bit grayscale*/
   int pixel = x + y * SCREEN_WIDTH;
   int byte = pixel / 8;
   int bit = pixel % 8;
   if(color){
      framebuffer[byte] |= 1 << (7 - bit);
   }
   else{
      framebuffer[byte] &= ~(1 << (7 - bit));
   }
}

void callSubprogram(activity_t activity){
   if(subprogramIndex < MAX_SUBPROGRAMS - 1){
      subprogramIndex++;
      parentSubprograms[subprogramIndex] = activity;
      currentSubprogram = activity;
      if(!subprogramArgsSet)
         subprogramArgs = makeVar(LENGTH_0, TYPE_NULL, 0);
      subprogramArgsSet = false;/*clear to prevent next subprogram called from inheriting the args*/
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
   }
   else{
      /*last subprogram is complete, exit application*/
      applicationRunning = false;
   }
}

void execSubprogram(activity_t activity){
   if(!subprogramArgsSet)
      subprogramArgs = makeVar(LENGTH_0, TYPE_NULL, 0);
   subprogramArgsSet = false;/*clear to prevent next subprogram called from inheriting the args*/
   parentSubprograms[subprogramIndex] = activity;
   currentSubprogram = activity;
}

var getSubprogramReturnValue(){
   return lastSubprogramReturnValue;
}

var getSubprogramArgs(){
   return subprogramArgs;
}

void setSubprogramArgs(var args){
   subprogramArgs = args;
}

var subprogramGetData(){
   return subprogramData[subprogramIndex];
}

void subprogramSetData(var data){
   subprogramData[subprogramIndex] = data;
}

static Boolean testerInit(){
   long     osVer;
   Err      setFramebufferFormatError;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   Boolean  memoryAllocSuccess;
   
   FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osVer);
   
   if (osVer < PalmOS35) {
      FrmCustomAlert(alt_err, "TestSuite requires at least PalmOS 3.5", 0, 0);
      return false;
   }
   
   KeySetMask(~(keyBitPageUp | keyBitPageDown |
                keyBitHard1  | keyBitHard2 |
                keyBitHard3  | keyBitHard4 ));
   
   width = 160;
   height = 160;
   depth = 1;
   setFramebufferFormatError = ScrDisplayMode(scrDisplayModeSet, &width, &height, &depth, NULL);
   
   if(setFramebufferFormatError){
      FrmCustomAlert(alt_err, "Framebuffer unusable", 0, 0);
      return false;
   }
   
   framebuffer = BmpGetBits(WinGetBitmap(WinGetDrawWindow()));
   
   UG_Init(&uguiStruct, uguiDrawPixel, 160, 160);
   UG_FontSelect(&FONT_4X6);
   UG_SetBackcolor(C_WHITE);
   UG_SetForecolor(C_BLACK);
   UG_ConsoleSetBackcolor(C_WHITE);
   UG_ConsoleSetForecolor(C_BLACK);
   
   unsafeMode = false;
   
   /*make test list*/
   memoryAllocSuccess = initTestList();
   if(!memoryAllocSuccess){
      FrmCustomAlert(alt_err, "Could not allocate UG_WINDOW", 0, 0);
      return false;
   }
   
   /*load first subprogram*/
   subprogramIndex = 0;
   subprogramArgsSet = false;
   lastSubprogramReturnValue = makeVar(LENGTH_0, TYPE_NULL, 0);
   subprogramArgs = makeVar(LENGTH_0, TYPE_NULL, 0);
   currentSubprogram = testPicker;
   
   return true;
}

static void testerExit(){
   /*nothing right now*/
}

static void testerFrameLoop(){
   palmButtons = KeyCurrentState();
   
   if(!unsafeMode){
      /*allow exiting the app normally*/
      EventType event;
      EvtGetEvent(&event, 1);
      SysHandleEvent(&event);
      if(event.eType == appStopEvent)
         applicationRunning = false;
   }
   
   lastSubprogramReturnValue = currentSubprogram();
   
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
   return(0);
}
