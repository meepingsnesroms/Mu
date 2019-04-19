#ifndef TRAPS_H
#define TRAPS_H

#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>

#include "palmGlobalDefines.h"

typedef struct PACKED{
   uint32_t linkSp;
   uint32_t pc;
}__return_stack_frame_type;
#define __return_stack_frame register __return_stack_frame_type* __stack_frame asm("a6")

/*these functions only work if __return_stack_frame is declared at the top of the API*/
/*__stack_frame should not be touched outside of these functions, thats why it has "__"*/
#define getCallLocation() (__stack_frame->pc - 4)
#define getCalledApiNum() (readArbitraryMemory16(__stack_frame->pc - 2))

void emuErrDisplayFileLineMsg(const Char* const filename, UInt16 lineNo, const Char* const msg);
void emuSysUnimplemented(void);

#endif
