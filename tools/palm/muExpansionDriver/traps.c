#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "traps.h"
#include "debug.h"
#include "globals.h"
#include "palmOsPriv.h"
#include "palmGlobalDefines.h"


/*cant use normal global variables in this file!!!*/
/*functions in this file are called directly by the Palm OS trap dispatch, this means they can be called when the app isnt loaded and the globals are just a random data buffer*/


void emuErrDisplayFileLineMsg(const Char* const filename, UInt16 lineNo, const Char* const msg){
   debugLog("Error at:%s, Line:%d, Msg:%s\n", filename, lineNo, msg);
}

void emuSysUnimplemented(void){
   __return_stack_frame;
   uint32_t callLocation = getCallLocation();
   uint16_t apiNum = getCalledApiNum();
   
   debugLog("Unimplemented API:0x%04X, called from:0x%08lX\n", apiNum, callLocation);
}
