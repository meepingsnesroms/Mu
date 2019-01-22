#ifndef _MISSINGFUNCTIONS_H_
#define _MISSINGFUNCTIONS_H_

// From Window.h:vvv
// Selectors for the PIN trap dispatcher 
#define pinPINSetInputAreaState           0
#define pinPINGetInputAreaState           1
#define pinPINSetInputTriggerState        2
#define pinPINGetInputTriggerState        3
#define pinPINAltInputSystemEnabled       4
#define pinPINGetCurrentPinletName        5
#define pinPINSwitchToPinlet              6
#define pinPINCountPinlets                7
#define pinPINGetPinletInfo               8
#define pinPINSetInputMode                9
#define pinPINGetInputMode                10
#define pinPINClearPinletState            11
#define pinPINShowReferenceDialog         12
#define pinWinSetConstraintsSize          13
#define pinFrmSetDIAPolicyAttr            14
#define pinFrmGetDIAPolicyAttr            15
#define pinStatHide                       16
#define pinStatShow                       17
#define pinStatGetAttribute               18

#define pinSysGetOrientation              19
#define pinSysSetOrientation              20
#define pinSysGetOrientationTriggerState  21
#define pinSysSetOrientationTriggerState  22

#define pinStatBarSetIcon                 23
#define pinStatBarGetIcon                 24

#define PINS_TRAP(selector) _SYSTEM_API(_CALL_WITH_SELECTOR)(_SYSTEM_TABLE, sysTrapPinsDispatch, selector)
// From Window.h:^^^


// From SystemMgr.h:vvv
#define sysFtrNumROMVersion   1                  // ROM Version
// 0xMMmfsbbb, where MM is major version, m is minor version
// f is bug fix, s is stage: 3-release,2-beta,1-alpha,0-development,
// bbb is build number for non-releases
// V1.12b3   would be: 0x01122003
// V2.00a2   would be: 0x02001002
// V1.01     would be: 0x01013000

#define sysFtrNumProcessorID   2                  // Product id
// 0xMMMMRRRR, where MMMM is the processor model and RRRR is the revision.
#define sysFtrNumProcessorMask      0xFFFF0000      // Mask to obtain processor model
#define sysFtrNumProcessor328       0x00010000      // Motorola 68328      (Dragonball)
#define sysFtrNumProcessorEZ        0x00020000      // Motorola 68EZ328   (Dragonball EZ)
#define sysFtrNumProcessorVZ        0x00030000      // Motorola 68VZ328   (Dragonball VZ)
#define sysFtrNumProcessorSuperVZ   0x00040000      // Motorola 68SZ328   (Dragonball SuperVZ)
#define sysFtrNumProcessorARM720T   0x00100000      // ARM 720T
#define sysFtrNumProcessorARM7TDMI  0x00110000      // ARM7TDMI
#define sysFtrNumProcessorARM920T   0x00120000      // ARM920T
#define sysFtrNumProcessorARM922T   0x00130000      // ARM922T
#define sysFtrNumProcessorARM925    0x00140000      // ARM925
#define sysFtrNumProcessorStrongARM 0x00150000      // StrongARM
#define sysFtrNumProcessorXscale    0x00160000      // Xscale
#define sysFtrNumProcessorARM710A   0x00170000      // ARM710A
#define sysFtrNumProcessorARM925T   0x00180000      // ARM925T
#define sysFtrNumProcessorARM926EJS 0x00190000      // ARM926EJ-S
#define sysFtrNumProcessorx86       0x01000000      // Intel CPU      (Palm Simulator)

// The following sysFtrNumProcessorIs68K(x) and sysFtrNumProcessorIsARM(x)
// macros are intended to be used to test the value returned from a call to
//    FtrGet(sysFtrCreator, sysFtrNumProcessorID, &value);
// in order to determine if the code being executed is running on a 68K or ARM processor.

#define sysFtrNumProcessor68KIfZero    0xFFF00000   // 68K if zero; not 68K if non-zero
#define sysFtrNumProcessorIs68K(x)     (((x&sysFtrNumProcessor68KIfZero)==0)? true : false)

#define sysFtrNumProcessorARMIfNotZero 0x00F00000   // ARM if non-zero
#define sysFtrNumProcessorIsARM(x)     (((x&sysFtrNumProcessorARMIfNotZero)!=0)? true : false)
// From SystemMgr.h:^^^

#endif // _MISSINGFUNCTIONS_H_
