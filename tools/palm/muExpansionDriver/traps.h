#ifndef TRAPS_H
#define TRAPS_H

#include "sdkPatch/PalmOSPatched.h"

UInt32 emuPceNativeCall(NativeFuncType *nativeFuncP, void *userDataP);
UInt32 emuKeyCurrentState(void);
void emuErrDisplayFileLineMsg(const Char* const filename, UInt16 lineNo, const Char* const msg);
void emuSysUnimplemented(void);
Err emuHwrDisplayAttributes(Boolean set, UInt8 attribute, void* dataPtr);
void emuScrDrawChars(WinPtr pWindow, Int16 xLoc, Int16 yLoc, Int16 xExtent, Int16 yExtent, Int16 clipTop, Int16 clipLeft, Int16 clipBottom, Int16 clipRight, Char* chars, UInt16 len, FontPtr fontPtr);


#endif
