#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <stdint.h>
#include "ugui.h"
#include "testSuiteConfig.h"
#include "testSuite.h"
#include "viewer.h"


/* register for direct video hardware access on OS 3.1 */
#define LSSA  *((         void **)0xFFFFFA00)
#define VPW   *((unsigned char  *)0xFFFFFA05)
#define LYMAX *((unsigned short *)0xFFFFFA0A)
#define LCXP  *((unsigned short *)0xFFFFFA18)
#define LCYP  *((unsigned short *)0xFFFFFA1A)
#define LCWCH *((unsigned short *)0xFFFFFA1C)
#define LBLCK *((unsigned char  *)0xFFFFFA1F)
#define PICF  *((unsigned char  *)0xFFFFFA20)
#define LPXCD *((unsigned char  *)0xFFFFFA25)
#define CKCON *((unsigned char  *)0xFFFFFA27)
#define LLBAR *((unsigned char  *)0xFFFFFA29)
#define LPOSR *((unsigned char  *)0xFFFFFA2D)
#define FRCM  *((unsigned char  *)0xFFFFFA31)
#define LGPMR *((unsigned short *)0xFFFFFA32)


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
   return (void*)(thisVar.value & 0xFFFFFFFF);
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


/*exports*/
uint16_t palmButtons;
uint16_t palmButtonsLastFrame;


/*video stuff*/
static UG_GUI   uguiStruct;
static uint8_t* framebuffer;
static uint8_t* oldFramebuffer;
static uint8_t  oldLpxcd;
static uint8_t  oldPicf;
static uint8_t  oldVpw;
static uint8_t  oldLlbar;

/*other*/
static activity_t parentSubprograms[MAX_SUBPROGRAMS];
static uint32_t   subprogramIndex;
static activity_t currentSubprogram;
static var        subprogramData[MAX_SUBPROGRAMS];
static var        subprogramArgs;/*optional arguments when one subprogram calls another*/
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

void testerInit(){
   KeySetMask(~(keyBitPageUp | keyBitPageDown |
                keyBitHard1  | keyBitHard2 |
                keyBitHard3  | keyBitHard4 ));
   
   framebuffer = MemPtrNew(SCREEN_WIDTH * SCREEN_HEIGHT / 8);
   
   /* save old video regs */
   oldFramebuffer = LSSA;
   oldLpxcd = LPXCD;
   oldPicf = PICF;
   oldVpw = VPW;
   oldLlbar = LLBAR;
   
   /* set to full refresh */
   LPXCD = 0;
   
   /* display off */
   CKCON &= ~0x80;
   
   /* virtual page width now 20 bytes (160 greyscale pixels) */
   VPW    = 10;
   PICF  &= ~0x03;  /* switch to black and white mode */
   LLBAR  = 10;     /* line buffer now 20 bytes */
   
   /* register to control grayscale pixel oscillations */
   FRCM = 0xB9;//not listed in Dragonball VZ datasheet
   
   LSSA = framebuffer;
   
   /* let the LCD get to a 2 new frames (40ms delay) */
   SysTaskDelay(4);
   
   /* switch LCD back on */
   CKCON |= 0x80;
   
   UG_Init(&uguiStruct, uguiDrawPixel, 160, 160);
   UG_SetBackcolor(C_WHITE);
   UG_SetForecolor(C_BLACK);
   
   subprogramIndex = 0;
   subprogramArgsSet = false;
   currentSubprogram = testViewer;
}

void testerExit(){
   LSSA = oldFramebuffer;
   LPXCD = oldLpxcd;
   PICF = oldPicf;
   VPW= oldVpw;
   LLBAR = oldLlbar;
}

void testerFrameLoop(){
   palmButtons = KeyCurrentState();
   
   currentSubprogram();
   
   palmButtonsLastFrame = palmButtons;
}

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags){
   testerInit();
   
   applicationRunning = true;
   while(applicationRunning){
      testerFrameLoop();
      SysTaskDelay(4);/*30 fps*/
   }
   
   testerExit();
   return(0);
}

