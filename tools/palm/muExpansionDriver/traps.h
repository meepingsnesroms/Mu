#ifndef TRAPS_H
#define TRAPS_H

#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "palmGlobalDefines.h"

typedef const struct{
   uint16_t sr;
   uint32_t pc;
   uint32_t sp;
}__vector_stack_frame_type;
#define __dispatcher_stack_frame __vector_stack_frame_type __stack_frame

/*these functions only work if __dispatcher_stack_frame is the final variable of the API*/
#define getCallLocation() (__stack_frame.pc - 2)
#define getApiNum() (readArbitraryMemory16(__stack_frame.pc))

UInt32 emuPceNativeCall(NativeFuncType *nativeFuncP, void *userDataP);
UInt32 emuKeyCurrentState(void);
void emuErrDisplayFileLineMsg(const Char* const filename, UInt16 lineNo, const Char* const msg);
void emuSysUnimplemented(__dispatcher_stack_frame);
Err emuHwrDisplayAttributes(Boolean set, UInt8 attribute, void* dataPtr);
void emuScrDrawChars(WinPtr pWindow, Int16 xLoc, Int16 yLoc, Int16 xExtent, Int16 yExtent, Int16 clipTop, Int16 clipLeft, Int16 clipBottom, Int16 clipRight, Char* chars, UInt16 len, FontPtr fontPtr);


#endif
