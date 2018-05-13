#include <PalmOS.h>
#include <string.h>
#include "testSuite.h"
#include "specs/irdaCommands.h"
#include "debug.h"
#include "ugui.h"


/*since modern OSs dont support any way to communicate with Palm OS I am making my own with an IR LED and receiver*/
/*this is mainly because running any sort of test takes 3 minutes to compile it, get it on a memory card, load it then find out it doesnt work*/


static uint32_t irdaCommandBufferLength;
static irda_command_t irdaCommandIn;
static irda_command_t irdaCommandOut;


static void irdaSendCommand(irda_command_t command){
   
}

static irda_command_t irdaGetCommand(){
   
}

static void irdaHandleCommands(){
   
}

static void irdaReset(){
   irdaCommandBufferLength = 0;
   irdaCommandIn.command = 0x00000000;
   irdaCommandIn.data = 0x00000000;
   irdaCommandOut.command = 0x00000000;
   irdaCommandOut.data = 0x00000000;
}

var irdaCommandLoop(){
   static Boolean firstRun = true;
   
   if(firstRun){
      firstRun = false;
      debugSafeScreenClear(C_WHITE);
      StrPrintF(sharedDataBuffer, "IRDA remote control\n mode running...");
      UG_PutString(0, 0, sharedDataBuffer);
      irdaReset();
   }
   
   if(getButtonPressed(buttonBack)){
      firstRun = true;
      exitSubprogram();
   }
   
   irdaHandleCommands();
   
   return makeVar(LENGTH_0, TYPE_NULL, 0);
}
