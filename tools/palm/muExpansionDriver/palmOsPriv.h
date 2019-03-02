#ifndef PALM_OS_PRIV_H
#define PALM_OS_PRIV_H

#define memNewChunkFlagAllowLarge 0x1000

#define FIXED_ADDRESS_VAR(a, t) (*((volatile t*)a))

/*these are all the Palm OS varibles the SDK dosent expose that I have found while decompiling Palm OS*/
#define ScrStatePtr FIXED_ADDRESS_VAR(0x00000164, void*)

#endif
