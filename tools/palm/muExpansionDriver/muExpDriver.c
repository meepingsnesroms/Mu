#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "config.h"
#include "gui.h"
#include "patcher.h"


UInt32 PilotMain(UInt16 cmd, MemPtr cmdBPB, UInt16 launchFlags){
   uint32_t configFile[CONFIG_FILE_ENTRIES];
   
   readConfigFile(configFile);
   
   if(cmd == sysAppLaunchCmdNormalLaunch)
      showGui(configFile);
   else if(cmd == sysAppLaunchCmdSystemReset)
      initBoot(configFile);
   
   writeConfigFile(configFile);
   
   return 0;
}
