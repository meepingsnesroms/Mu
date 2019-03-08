#ifndef PALM_OS_PRIV_H
#define PALM_OS_PRIV_H

#include "palmGlobalDefines.h"

/*these are all the Palm OS varibles the SDK dosent expose that I have found while decompiling Palm OS*/
/*defines*/
#define memNewChunkFlagAllowLarge 0x1000

/*CPU*/
#define ResetStackPointer FIXED_ADDRESS_VAR(0x00000000, uint16_t*)
#define ResetVector FIXED_ADDRESS_VAR(0x00000004, void*)

/*OS*/
#define ScrStatePtr FIXED_ADDRESS_VAR(0x00000164, void*)

#endif
