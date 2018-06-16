#include <PalmOS.h>
#include <stdint.h>
#include <string.h>

#include "testSuite.h"
#include "specs/irdaCommands.h"
#include "debug.h"
#include "ugui.h"
#include "vnc.h"


/*since modern OSs dont support any way to communicate with Palm OS I am making my own with an IR LED and receiver*/
/*this is mainly because running any sort of test takes 3 minutes to compile it, get it on a memory card, load it then find out it doesnt work*/


#define IRDA_WAIT_FOREVER 0x7FFFFFFF


static Boolean irdaRunning = false;
static Err irdaError;
static Boolean irdaUseThreadedVnc;/*use Dragonball VZ timer 2 interrupt as a second thread for VNC*/
static uint16_t irdaPortId;


#include "irdaAccessors.c.h"

void irdaHandleCommands(){
   if(irdaRunning){
      uint8_t irdaCommand = irdaReceiveUint8();
      while(irdaCommand != IRDA_COMMAND_NONE){
         switch(irdaCommand){
            case IRDA_COMMAND_NONE:
               break;
               
            case IRDA_COMMAND_GET_BYTE:{
               uint32_t location = irdaReceiveUint32();
               irdaTransmitUint8(IRDA_COMMAND_RETURN);
               irdaTransmitUint8(readArbitraryMemory8(location));
               break;
            }
               
            case IRDA_COMMAND_SET_BYTE:{
               uint32_t location = irdaReceiveUint32();
               uint8_t data = irdaReceiveUint8();
               writeArbitraryMemory8(location, data);
               break;
            }
               
            default:
               /*todo: print error and terminate*/
               break;
         }
         
         irdaCommand = irdaReceiveUint8();
      }
   }
}

static Boolean irdaInit(Boolean useThreadedVnc){
   if(!irdaRunning){
      uint32_t value;
      
      irdaError = FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &value);
      if(irdaError != errNone || value == false)
         return false;
      
      irdaError = SrmOpen(serPortIrPort, 9600, &irdaPortId);
      if(irdaError != errNone)
         return false;
      
      if(useThreadedVnc){
         Boolean success;
         success = vncInit();
         if(success)
            irdaUseThreadedVnc = true;
         else
            irdaUseThreadedVnc = false;
      }
      else{
         irdaUseThreadedVnc = false;
      }
      
      irdaRunning = true;
      return true;
   }

   return true;/*its already running, return success*/
}

static void irdaExit(){
   if(irdaRunning){
      if(irdaUseThreadedVnc)
         vncExit();
      irdaTransmitUint8(IRDA_COMMAND_CLOSE_CONNECTION);
      SrmSendWait(irdaPortId);
      SrmReceiveFlush(irdaPortId, 1);
      SrmSendFlush(irdaPortId);
      SrmClose(irdaPortId);
   }
}

var irdaCommandLoop(){
   static Boolean firstRun = true;
   static Boolean running;
   
   if(firstRun){
      Boolean success;
      firstRun = false;
      running = false;
      success = irdaInit(varsEqual(getSubprogramArgs(), makeVar(LENGTH_1, TYPE_BOOL, true)));
      if(success){
         debugSafeScreenClear(C_WHITE);
         StrPrintF(sharedDataBuffer, "IRDA remote control\n mode running...");
         UG_PutString(0, 0, sharedDataBuffer);
      }
      else{
         debugSafeScreenClear(C_WHITE);
         StrPrintF(sharedDataBuffer, "IRDA startup failed!");
         UG_PutString(0, 0, sharedDataBuffer);
      }
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      irdaExit();
      exitSubprogram();
   }
   
   if(!irdaUseThreadedVnc)
      irdaHandleCommands();
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}

var irdaRun(){
   static Boolean firstRun = true;
   uint16_t y = 0;
   
   if(firstRun){
      firstRun = false;
      if(!unsafeMode){
         /*cant run VNC server anyway*/
         setSubprogramArgs(makeVar(LENGTH_1, TYPE_BOOL, false));
         execSubprogram(irdaCommandLoop);
         return makeVar(LENGTH_0, TYPE_NULL, 0);
      }
      
      debugSafeScreenClear(C_WHITE);
      StrPrintF(sharedDataBuffer, "Use Dragonball VZ timer 2 as VNC thread?");
      UG_PutString(0, y, sharedDataBuffer);
      y += (FONT_HEIGHT + 1) * 2;
      StrPrintF(sharedDataBuffer, "Left = No, Right = Yes");
      UG_PutString(0, y, sharedDataBuffer);
      y += FONT_HEIGHT + 1;
   }
   
   if(getButtonPressed(buttonLeft)){
      firstRun = true;
      setSubprogramArgs(makeVar(LENGTH_1, TYPE_BOOL, false));
      execSubprogram(irdaCommandLoop);
   }
   
   if(getButtonPressed(buttonRight)){
      firstRun = true;
      setSubprogramArgs(makeVar(LENGTH_1, TYPE_BOOL, true));
      execSubprogram(irdaCommandLoop);
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
