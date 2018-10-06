#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <stdint.h>

/*dont include this anywhere else*/
#include "MuExpDriverRsc.h"


void installTrapHandlers(){
   
}

void showGui(){
   
}

Boolean isDeviceCompatible(){
   
}

Boolean isEnabled(){
   /*create a 1 byte database or check for its existance*/
}

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags){
   if(cmd == sysAppLaunchCmdNormalLaunch)
      showGui();
   else if(cmd == sysAppLaunchCmdSystemReset)
      if(isDeviceCompatible())
         installTrapHandlers();
   
   return 0;
}
