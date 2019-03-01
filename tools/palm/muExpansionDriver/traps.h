#ifndef TRAPS_H
#define TRAPS_H

#include "sdkPatch/PalmOSPatched.h"

UInt32 emuPceNativeCall(NativeFuncType *nativeFuncP, void *userDataP);
UInt32 emuKeyCurrentState(void);
void emuErrDisplayFileLineMsg(const Char* const filename, UInt16 lineNo, const Char* const msg);
Err emuHwrDisplayAttributes(Boolean set, UInt8 attribute, void* returnPtr);

#endif
