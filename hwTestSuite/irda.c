#include <PalmOS.h>
#include <string.h>
#include "testSuite.h"
#include "specs/irdaCommands.h"
#include "debug.h"
#include "ugui.h"


/*since modern OSs dont support any way to communicate with Palm OS I am making my own with an IR LED and receiver*/
/*this is mainly because running any sort of test takes 3 minutes to compile it, get it on a memory card, load it then find out it doesnt work*/


static bool irdaRunning;
static bool irdaError;
static bool irdaOutOfSync;
static uint16_t irdaPortId;
static uint32_t irdaCommandBufferLength;
static uint8_t irdaCommandInOffset;
static uint8_t irdaCommandInBytes[IRDA_COMMAND_SIZE];
static uint8_t irdaCommandOutBytes[IRDA_COMMAND_SIZE];


static void irdaSendCommand(uint64_t command){
   if(irdaRunning){
      uint32_t bytes;
      Err error;
      
      bytes = SrmSend(irdaPortId, &irdaCommandOut, IRDA_COMMAND_SIZE/*bytes*/, &error);
      if(error != errNone)
         irdaError = true;
      if(bytes != IRDA_COMMAND_SIZE)
         irdaOutOfSync = true;
   }
}

static uint64_t irdaGetCommand(){
   if(irdaRunning){
      uint32_t requestBytes;
      uint32_t bytes;
      Err error;
      
      error = SrmReceiveCheck(irdaPortId, &requestBytes);
      if(error != errNone)
         irdaError = true;
      
      requestBytes = min(requestBytes, IRDA_COMMAND_SIZE - irdaCommandInOffset + 1);
      
      bytes = SrmReceive(irdaPortId, ((uint8_t*)&irdaCommandInBytes)[irdaCommandInOffset], requestBytes, 0, &error);
      if(error != errNone)
         irdaError = true;
      irdaCommandInOffset += bytes;
      
      if(irdaCommandInOffset >= IRDA_COMMAND_SIZE){
         irdaCommandInOffset = 0;
         return IRDA_GET_COMMAND(irdaCommandInBytes);
      }
      
      return (uint64_t)IRDA_COMMAND_NONE << 32 | 0x00000000;
   }
}

static void irdaHandleCommands(){
   if(irdaRunning){
      
   }
}

static bool irdaStart(){
   if(!irdaRunning){
      uint32_t value;
      Err error;
      
      error = FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &value);
      if(error != errNone)
         return false;
      
      error = SrmOpen(serPortIrPort, 9600, &irdaPortId);
      if(error != errNone)
         return false;
      
      irdaRunning = true;
      
      return true;
   }

   return true;/*its already running, return success*/
}

static void irdaClose(){
   if(irdaRunning){
      SrmSendWait(irdaPortId);
      SrmClose(irdaPortId);
   }
}

var irdaCommandLoop(){
   static Boolean firstRun = true;
   
   if(firstRun){
      Boolean success;
      firstRun = false;
      success = irdaStart();
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
      irdaClose();
      exitSubprogram();
   }
   
   irdaHandleCommands();
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
