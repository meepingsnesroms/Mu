#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <stdint.h>

/*dont include this anywhere else*/
#include "MuExpDriverRsc.h"


void installTrapHandlers(){
   
}

void showGui(){
   
}

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags){
   if(cmd == sysAppLaunchCmdNormalLaunch)
      showGui();
   else if(cmd == sysAppLaunchCmdSystemReset)
      installTrapHandlers();
   
   return 0;
}
