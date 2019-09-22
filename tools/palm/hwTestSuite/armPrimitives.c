#include <PalmOS.h>
#include "PceNativeCall.h"
#include "ByteOrderUtils.h"
#include <stdint.h>

#include "testSuite.h"


void callArmTests(uint32_t* args, uint8_t argCount){
   MemHandle armHandle;
   MemPtr armPointer;
   uint8_t index;
   
   argCount++;/*add the test number to the args too*/
   for(index = 0; index < argCount; index++)
      args[index] = EndianSwap32(args[index]);
   
   armHandle = DmGetResource(ARM_RESOURCE_TYPE, ARM_RESOURCE_ID);
   armPointer = MemHandleLock(armHandle);
   argCount = PceNativeCall(armPointer, args);
   MemHandleUnlock(armHandle);
   DmReleaseResource(armHandle);
   
   argCount++;/*add the test number to the args too*/
   for(index = 0; index < argCount; index++)
      args[index] = EndianSwap32(args[index]);
}
