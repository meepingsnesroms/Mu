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

#endif // _MISSINGFUNCTIONS_H_
