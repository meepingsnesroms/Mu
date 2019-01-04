 #ifndef __CORETRAPS_PATCH_H_
 #define __CORETRAPS_PATCH_H_

#undef sysTrapLastTrapNumber
#undef sysNumTraps

// ======================================================================
// Palm OS 5.0 Traps
// No new traps were added for 4.1, though 4.1 SC (see below) added some.
// ======================================================================

// 11/16/01 bob
#define sysTrapPceNativeCall							0xA45A

// ======================================================================
// Palm OS 5.1 Traps
// ======================================================================

// 12/04/01 lrt
#define sysTrapSndStreamCreate							0xA45B
#define sysTrapSndStreamDelete							0xA45C
#define sysTrapSndStreamStart							0xA45D
#define sysTrapSndStreamPause							0xA45E
#define sysTrapSndStreamStop							0xA45F
#define sysTrapSndStreamSetVolume						0xA460
#define sysTrapSndStreamGetVolume						0xA461
#define sysTrapSndPlayResource							0xA462
#define sysTrapSndStreamSetPan							0xA463
#define sysTrapSndStreamGetPan							0xA464

// 04/12/02 jed
#define sysTrapMultimediaDispatch						0xA465

// TRAPS ABOVE THIS POINT CAN NOT CHANGE BECAUSE THEY HAVE
// BEEN RELEASED TO CUSTOMERS IN SHIPPING ROMS AND SDKS.
// (MOVE THIS COMMENT DOWN WHENEVER THE "NEXT" RELEASE OCCURS.)

// ======================================================================
// Palm OS 5.1.1 Traps
// ======================================================================

// 08/02/02 mne
#define sysTrapSndStreamCreateExtended					0xa466
#define sysTrapSndStreamDeviceControl					0xa467

// ======================================================================
// Palm OS 4.2SC (Simplified Chinese) Traps
// These were added to an older 68K-based version of Palm OS to support
// QVGA displays.
// ======================================================================

// 09/23/02 acs & bob
#define sysTrapBmpCreateVersion3						0xA468
#define sysTrapECFixedMul								0xA469
#define sysTrapECFixedDiv								0xA46A
#define sysTrapHALDrawGetSupportedDensity				0xA46B
#define sysTrapHALRedrawInputArea						0xA46C
#define sysTrapGrfBeginStroke							0xA46D
#define sysTrapBmpPrvConvertBitmap						0xA46E

// ======================================================================
// Palm OS 5.x Traps
// These were added for new features or extensions for 5.x
// ======================================================================
#define sysTrapNavSelector			 			        0xA46F


// 12/11/02 grant
#define sysTrapPinsDispatch								0xA470

// ======================================================================
// Palm OS 5.3 Traps
// These were added for new features or extensions for 5.2. Currently
// they aren't implemented by any version of Palm OS released by
// PalmSource, but are reserved for future implementation.
// ======================================================================
#define sysTrapSysReservedTrap1		 			0xA471
#define sysTrapSysReservedTrap2					0xA472
#define sysTrapSysReservedTrap3					0xA473
#define sysTrapSysReservedTrap4					0xA474


#define sysTrapDmSync                           0xA475
#define sysTrapDmSyncDatabase                   0xA476

#define sysTrapLastTrapNumber					0xA477

// WARNING!! LEAVE THIS AT THE END AND ALWAYS ADD NEW TRAPS TO
// THE END OF THE TRAP TABLE BUT RIGHT BEFORE THIS TRAP, AND THEN
// RENUMBER THIS ONE TO ONE MORE THAN THE ONE RIGHT BEFORE IT!!!!!!!!!

#define	sysNumTraps	 (sysTrapLastTrapNumber - sysTrapBase)



#endif  //__CORETRAPS_H_
