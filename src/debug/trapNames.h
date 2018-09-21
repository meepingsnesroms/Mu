#pragma once

// ======================================================================
// Palm OS 1.0 Traps
// ======================================================================

#define MemInit									0xA000
#define MemInitHeapTable							0xA001
#define MemStoreInit								0xA002
#define MemCardFormat							0xA003
#define MemCardInfo								0xA004
#define MemStoreInfo								0xA005
#define MemStoreSetInfo							0xA006
#define MemNumHeaps								0xA007
#define MemNumRAMHeaps							0xA008
#define MemHeapID								0xA009
#define MemHeapPtr								0xA00A
#define MemHeapFreeBytes							0xA00B
#define MemHeapSize								0xA00C
#define MemHeapFlags								0xA00D
#define MemHeapCompact							0xA00E
#define MemHeapInit								0xA00F
#define MemHeapFreeByOwnerID						0xA010
#define MemChunkNew								0xA011
#define MemChunkFree								0xA012
#define MemPtrNew								0xA013
#define MemPtrRecoverHandle						0xA014
#define MemPtrFlags								0xA015
#define MemPtrSize								0xA016
#define MemPtrOwner								0xA017
#define MemPtrHeapID								0xA018
#define MemPtrCardNo								0xA019
#define MemPtrToLocalID							0xA01A
#define MemPtrSetOwner							0xA01B
#define MemPtrResize								0xA01C
#define MemPtrResetLock							0xA01D
#define MemHandleNew								0xA01E
#define MemHandleLockCount						0xA01F
#define MemHandleToLocalID						0xA020
#define MemHandleLock							0xA021
#define MemHandleUnlock							0xA022
#define MemLocalIDToGlobal						0xA023
#define MemLocalIDKind							0xA024
#define MemLocalIDToPtr							0xA025
#define MemMove									0xA026
#define MemSet									0xA027
#define MemStoreSearch							0xA028
#define SysReserved10Trap1						0xA029  /* "Reserved" trap in Palm OS 1.0 and later (was MemPtrDataStorage) */

#define MemKernelInit							0xA02A
#define MemHandleFree							0xA02B
#define MemHandleFlags							0xA02C
#define MemHandleSize							0xA02D
#define MemHandleOwner							0xA02E
#define MemHandleHeapID							0xA02F
#define MemHandleDataStorage						0xA030
#define MemHandleCardNo							0xA031
#define MemHandleSetOwner						0xA032
#define MemHandleResize							0xA033
#define MemHandleResetLock						0xA034
#define MemPtrUnlock								0xA035
#define MemLocalIDToLockedPtr					0xA036
#define MemSetDebugMode							0xA037
#define MemHeapScramble							0xA038
#define MemHeapCheck								0xA039
#define MemNumCards								0xA03A
#define MemDebugMode								0xA03B
#define MemSemaphoreReserve						0xA03C
#define MemSemaphoreRelease						0xA03D
#define MemHeapDynamic							0xA03E
#define MemNVParams								0xA03F


#define DmInit									0xA040
#define DmCreateDatabase							0xA041
#define DmDeleteDatabase							0xA042
#define DmNumDatabases							0xA043
#define DmGetDatabase							0xA044
#define DmFindDatabase							0xA045
#define DmDatabaseInfo							0xA046
#define DmSetDatabaseInfo						0xA047
#define DmDatabaseSize							0xA048
#define DmOpenDatabase							0xA049
#define DmCloseDatabase							0xA04A
#define DmNextOpenDatabase						0xA04B
#define DmOpenDatabaseInfo						0xA04C
#define DmResetRecordStates						0xA04D
#define DmGetLastErr								0xA04E
#define DmNumRecords								0xA04F
#define DmRecordInfo								0xA050
#define DmSetRecordInfo							0xA051
#define DmAttachRecord							0xA052
#define DmDetachRecord							0xA053
#define DmMoveRecord								0xA054
#define DmNewRecord								0xA055
#define DmRemoveRecord							0xA056
#define DmDeleteRecord							0xA057
#define DmArchiveRecord							0xA058
#define DmNewHandle								0xA059
#define DmRemoveSecretRecords					0xA05A
#define DmQueryRecord							0xA05B
#define DmGetRecord								0xA05C
#define DmResizeRecord							0xA05D
#define DmReleaseRecord							0xA05E
#define DmGetResource							0xA05F
#define DmGet1Resource							0xA060
#define DmReleaseResource						0xA061
#define DmResizeResource							0xA062
#define DmNextOpenResDatabase					0xA063
#define DmFindResourceType						0xA064
#define DmFindResource							0xA065
#define DmSearchResource							0xA066
#define DmNumResources							0xA067
#define DmResourceInfo							0xA068
#define DmSetResourceInfo						0xA069
#define DmAttachResource							0xA06A
#define DmDetachResource							0xA06B
#define DmNewResource							0xA06C
#define DmRemoveResource							0xA06D
#define DmGetResourceIndex						0xA06E
#define DmQuickSort								0xA06F
#define DmQueryNextInCategory					0xA070
#define DmNumRecordsInCategory					0xA071
#define DmPositionInCategory						0xA072
#define DmSeekRecordInCategory					0xA073
#define DmMoveCategory							0xA074
#define DmOpenDatabaseByTypeCreator				0xA075
#define DmWrite									0xA076
#define DmStrCopy								0xA077
#define DmGetNextDatabaseByTypeCreator			0xA078
#define DmWriteCheck								0xA079
#define DmMoveOpenDBContext						0xA07A
#define DmFindRecordByID							0xA07B
#define DmGetAppInfoID							0xA07C
#define DmFindSortPositionV10					0xA07D
#define DmSet									0xA07E
#define DmCreateDatabaseFromImage				0xA07F

#define DbgSrcMessage							0xA080
#define DbgMessage								0xA081
#define DbgGetMessage							0xA082
#define DbgCommSettings							0xA083

#define ErrDisplayFileLineMsg					0xA084
#define ErrSetJump								0xA085
#define ErrLongJump								0xA086
#define ErrThrow									0xA087
#define ErrExceptionList							0xA088

#define SysBroadcastActionCode					0xA089
#define SysUnimplemented							0xA08A
#define SysColdBoot								0xA08B
#define SysReset									0xA08C
#define SysDoze									0xA08D
#define SysAppLaunch								0xA08E
#define SysAppStartup							0xA08F
#define SysAppExit								0xA090
#define SysSetA5									0xA091
#define SysSetTrapAddress						0xA092
#define SysGetTrapAddress						0xA093
#define SysTranslateKernelErr					0xA094
#define SysSemaphoreCreate						0xA095
#define SysSemaphoreDelete						0xA096
#define SysSemaphoreWait							0xA097
#define SysSemaphoreSignal						0xA098
#define SysTimerCreate							0xA099
#define SysTimerWrite							0xA09A
#define SysTaskCreate							0xA09B
#define SysTaskDelete							0xA09C
#define SysTaskTrigger							0xA09D
#define SysTaskID								0xA09E
#define SysTaskUserInfoPtr						0xA09F
#define SysTaskDelay								0xA0A0
#define SysTaskSetTermProc						0xA0A1
#define SysUILaunch								0xA0A2
#define SysNewOwnerID							0xA0A3
#define SysSemaphoreSet							0xA0A4
#define SysDisableInts							0xA0A5
#define SysRestoreStatus							0xA0A6
#define SysUIAppSwitch							0xA0A7
#define SysCurAppInfoPV20						0xA0A8
#define SysHandleEvent							0xA0A9
#define SysInit									0xA0AA
#define SysQSort									0xA0AB
#define SysCurAppDatabase						0xA0AC
#define SysFatalAlert							0xA0AD
#define SysResSemaphoreCreate					0xA0AE
#define SysResSemaphoreDelete					0xA0AF
#define SysResSemaphoreReserve					0xA0B0
#define SysResSemaphoreRelease					0xA0B1
#define SysSleep									0xA0B2
#define SysKeyboardDialogV10						0xA0B3
#define SysAppLauncherDialog						0xA0B4
#define SysSetPerformance						0xA0B5
#define SysBatteryInfoV20						0xA0B6
#define SysLibInstall							0xA0B7
#define SysLibRemove								0xA0B8
#define SysLibTblEntry							0xA0B9
#define SysLibFind								0xA0BA
#define SysBatteryDialog							0xA0BB
#define SysCopyStringResource					0xA0BC
#define SysKernelInfo							0xA0BD
#define SysLaunchConsole							0xA0BE
#define SysTimerDelete							0xA0BF
#define SysSetAutoOffTime						0xA0C0
#define SysFormPointerArrayToStrings				0xA0C1
#define SysRandom								0xA0C2
#define SysTaskSwitching							0xA0C3
#define SysTimerRead								0xA0C4

#define StrCopy									0xA0C5
#define StrCat									0xA0C6
#define StrLen									0xA0C7
#define StrCompare								0xA0C8
#define StrIToA									0xA0C9
#define StrCaselessCompare						0xA0CA
#define StrIToH									0xA0CB
#define StrChr									0xA0CC
#define StrStr									0xA0CD
#define StrAToI									0xA0CE
#define StrToLower								0xA0CF

#define SerReceiveISP							0xA0D0

#define SlkOpen									0xA0D1
#define SlkClose									0xA0D2
#define SlkOpenSocket							0xA0D3
#define SlkCloseSocket							0xA0D4
#define SlkSocketRefNum							0xA0D5
#define SlkSocketSetTimeout						0xA0D6
#define SlkFlushSocket							0xA0D7
#define SlkSetSocketListener						0xA0D8
#define SlkSendPacket							0xA0D9
#define SlkReceivePacket							0xA0DA
#define SlkSysPktDefaultResponse					0xA0DB
#define SlkProcessRPC							0xA0DC

#define ConPutS									0xA0DD
#define ConGetS									0xA0DE

#define FplInit									0xA0DF  /* Obsolete, here for compatibilty only! */
#define FplFree									0xA0E0  /* Obsolete, here for compatibilty only! */
#define FplFToA									0xA0E1  /* Obsolete, here for compatibilty only! */
#define FplAToF									0xA0E2  /* Obsolete, here for compatibilty only! */
#define FplBase10Info							0xA0E3  /* Obsolete, here for compatibilty only! */
#define FplLongToFloat							0xA0E4  /* Obsolete, here for compatibilty only! */
#define FplFloatToLong							0xA0E5  /* Obsolete, here for compatibilty only! */
#define FplFloatToULong							0xA0E6  /* Obsolete, here for compatibilty only! */
#define FplMul									0xA0E7  /* Obsolete, here for compatibilty only! */
#define FplAdd									0xA0E8  /* Obsolete, here for compatibilty only! */
#define FplSub									0xA0E9  /* Obsolete, here for compatibilty only! */
#define FplDiv									0xA0EA  /* Obsolete, here for compatibilty only! */

#define WinScreenInit							0xA0EB  /* was ScrInit */
#define ScrCopyRectangle							0xA0EC
#define ScrDrawChars								0xA0ED
#define ScrLineRoutine							0xA0EE
#define ScrRectangleRoutine						0xA0EF
#define ScrScreenInfo							0xA0F0
#define ScrDrawNotify							0xA0F1
#define ScrSendUpdateArea						0xA0F2
#define ScrCompressScanLine						0xA0F3
#define ScrDeCompressScanLine					0xA0F4

#define TimGetSeconds							0xA0F5
#define TimSetSeconds							0xA0F6
#define TimGetTicks								0xA0F7
#define TimInit									0xA0F8
#define TimSetAlarm								0xA0F9
#define TimGetAlarm								0xA0FA
#define TimHandleInterrupt						0xA0FB
#define TimSecondsToDateTime						0xA0FC
#define TimDateTimeToSeconds						0xA0FD
#define TimAdjust								0xA0FE
#define TimSleep									0xA0FF
#define TimWake									0xA100

#define CategoryCreateListV10					0xA101
#define CategoryFreeListV10						0xA102
#define CategoryFind								0xA103
#define CategoryGetName							0xA104
#define CategoryEditV10							0xA105
#define CategorySelectV10						0xA106
#define CategoryGetNext							0xA107
#define CategorySetTriggerLabel					0xA108
#define CategoryTruncateName						0xA109

#define ClipboardAddItem							0xA10A
#define ClipboardCheckIfItemExist				0xA10B
#define ClipboardGetItem							0xA10C

#define CtlDrawControl							0xA10D
#define CtlEraseControl							0xA10E
#define CtlHideControl							0xA10F
#define CtlShowControl							0xA110
#define CtlGetValue								0xA111
#define CtlSetValue								0xA112
#define CtlGetLabel								0xA113
#define CtlSetLabel								0xA114
#define CtlHandleEvent							0xA115
#define CtlHitControl							0xA116
#define CtlSetEnabled							0xA117
#define CtlSetUsable								0xA118
#define CtlEnabled								0xA119

#define EvtInitialize							0xA11A
#define EvtAddEventToQueue						0xA11B
#define EvtCopyEvent								0xA11C
#define EvtGetEvent								0xA11D
#define EvtGetPen								0xA11E
#define EvtSysInit								0xA11F
#define EvtGetSysEvent							0xA120
#define EvtProcessSoftKeyStroke					0xA121
#define EvtGetPenBtnList							0xA122
#define EvtSetPenQueuePtr						0xA123
#define EvtPenQueueSize							0xA124
#define EvtFlushPenQueue							0xA125
#define EvtEnqueuePenPoint						0xA126
#define EvtDequeuePenStrokeInfo					0xA127
#define EvtDequeuePenPoint						0xA128
#define EvtFlushNextPenStroke					0xA129
#define EvtSetKeyQueuePtr						0xA12A
#define EvtKeyQueueSize							0xA12B
#define EvtFlushKeyQueue							0xA12C
#define EvtEnqueueKey							0xA12D
#define EvtDequeueKeyEvent						0xA12E
#define EvtWakeup								0xA12F
#define EvtResetAutoOffTimer						0xA130
#define EvtKeyQueueEmpty							0xA131
#define EvtEnableGraffiti						0xA132

#define FldCopy									0xA133
#define FldCut									0xA134
#define FldDrawField								0xA135
#define FldEraseField							0xA136
#define FldFreeMemory							0xA137
#define FldGetBounds								0xA138
#define FldGetTextPtr							0xA139
#define FldGetSelection							0xA13A
#define FldHandleEvent							0xA13B
#define FldPaste									0xA13C
#define FldRecalculateField						0xA13D
#define FldSetBounds								0xA13E
#define FldSetText								0xA13F
#define FldGetFont								0xA140
#define FldSetFont								0xA141
#define FldSetSelection							0xA142
#define FldGrabFocus								0xA143
#define FldReleaseFocus							0xA144
#define FldGetInsPtPosition						0xA145
#define FldSetInsPtPosition						0xA146
#define FldSetScrollPosition						0xA147
#define FldGetScrollPosition						0xA148
#define FldGetTextHeight							0xA149
#define FldGetTextAllocatedSize					0xA14A
#define FldGetTextLength							0xA14B
#define FldScrollField							0xA14C
#define FldScrollable							0xA14D
#define FldGetVisibleLines						0xA14E
#define FldGetAttributes							0xA14F
#define FldSetAttributes							0xA150
#define FldSendChangeNotification				0xA151
#define FldCalcFieldHeight						0xA152
#define FldGetTextHandle							0xA153
#define FldCompactText							0xA154
#define FldDirty									0xA155
#define FldWordWrap								0xA156
#define FldSetTextAllocatedSize					0xA157
#define FldSetTextHandle							0xA158
#define FldSetTextPtr							0xA159
#define FldGetMaxChars							0xA15A
#define FldSetMaxChars							0xA15B
#define FldSetUsable								0xA15C
#define FldInsert								0xA15D
#define FldDelete								0xA15E
#define FldUndo									0xA15F
#define FldSetDirty								0xA160
#define FldSendHeightChangeNotification			0xA161
#define FldMakeFullyVisible						0xA162

#define FntGetFont								0xA163
#define FntSetFont								0xA164
#define FntGetFontPtr							0xA165
#define FntBaseLine								0xA166
#define FntCharHeight							0xA167
#define FntLineHeight							0xA168
#define FntAverageCharWidth						0xA169
#define FntCharWidth								0xA16A
#define FntCharsWidth							0xA16B
#define FntDescenderHeight						0xA16C
#define FntCharsInWidth							0xA16D
#define FntLineWidth								0xA16E

#define FrmInitForm								0xA16F
#define FrmDeleteForm							0xA170
#define FrmDrawForm								0xA171
#define FrmEraseForm								0xA172
#define FrmGetActiveForm							0xA173
#define FrmSetActiveForm							0xA174
#define FrmGetActiveFormID						0xA175
#define FrmGetUserModifiedState					0xA176
#define FrmSetNotUserModified					0xA177
#define FrmGetFocus								0xA178
#define FrmSetFocus								0xA179
#define FrmHandleEvent							0xA17A
#define FrmGetFormBounds							0xA17B
#define FrmGetWindowHandle						0xA17C
#define FrmGetFormId								0xA17D
#define FrmGetFormPtr							0xA17E
#define FrmGetNumberOfObjects					0xA17F
#define FrmGetObjectIndex						0xA180
#define FrmGetObjectId							0xA181
#define FrmGetObjectType							0xA182
#define FrmGetObjectPtr							0xA183
#define FrmHideObject							0xA184
#define FrmShowObject							0xA185
#define FrmGetObjectPosition						0xA186
#define FrmSetObjectPosition						0xA187
#define FrmGetControlValue						0xA188
#define FrmSetControlValue						0xA189
#define FrmGetControlGroupSelection				0xA18A
#define FrmSetControlGroupSelection				0xA18B
#define FrmCopyLabel								0xA18C
#define FrmSetLabel								0xA18D
#define FrmGetLabel								0xA18E
#define FrmSetCategoryLabel						0xA18F
#define FrmGetTitle								0xA190
#define FrmSetTitle								0xA191
#define FrmAlert									0xA192
#define FrmDoDialog								0xA193
#define FrmCustomAlert							0xA194
#define FrmHelp									0xA195
#define FrmUpdateScrollers						0xA196
#define FrmGetFirstForm							0xA197
#define FrmVisible								0xA198
#define FrmGetObjectBounds						0xA199
#define FrmCopyTitle								0xA19A
#define FrmGotoForm								0xA19B
#define FrmPopupForm								0xA19C
#define FrmUpdateForm							0xA19D
#define FrmReturnToForm							0xA19E
#define FrmSetEventHandler						0xA19F
#define FrmDispatchEvent							0xA1A0
#define FrmCloseAllForms							0xA1A1
#define FrmSaveAllForms							0xA1A2
#define FrmGetGadgetData							0xA1A3
#define FrmSetGadgetData							0xA1A4
#define FrmSetCategoryTrigger					0xA1A5

#define UIInitialize								0xA1A6
#define UIReset									0xA1A7

#define InsPtInitialize							0xA1A8
#define InsPtSetLocation							0xA1A9
#define InsPtGetLocation							0xA1AA
#define InsPtEnable								0xA1AB
#define InsPtEnabled								0xA1AC
#define InsPtSetHeight							0xA1AD
#define InsPtGetHeight							0xA1AE
#define InsPtCheckBlink							0xA1AF

#define LstSetDrawFunction						0xA1B0
#define LstDrawList								0xA1B1
#define LstEraseList								0xA1B2
#define LstGetSelection							0xA1B3
#define LstGetSelectionText						0xA1B4
#define LstHandleEvent							0xA1B5
#define LstSetHeight								0xA1B6
#define LstSetSelection							0xA1B7
#define LstSetListChoices						0xA1B8
#define LstMakeItemVisible						0xA1B9
#define LstGetNumberOfItems						0xA1BA
#define LstPopupList								0xA1BB
#define LstSetPosition							0xA1BC

#define MenuInit									0xA1BD
#define MenuDispose								0xA1BE
#define MenuHandleEvent							0xA1BF
#define MenuDrawMenu								0xA1C0
#define MenuEraseStatus							0xA1C1
#define MenuGetActiveMenu						0xA1C2
#define MenuSetActiveMenu						0xA1C3

#define RctSetRectangle							0xA1C4
#define RctCopyRectangle							0xA1C5
#define RctInsetRectangle						0xA1C6
#define RctOffsetRectangle						0xA1C7
#define RctPtInRectangle							0xA1C8
#define RctGetIntersection						0xA1C9

#define TblDrawTable								0xA1CA
#define TblEraseTable							0xA1CB
#define TblHandleEvent							0xA1CC
#define TblGetItemBounds							0xA1CD
#define TblSelectItem							0xA1CE
#define TblGetItemInt							0xA1CF
#define TblSetItemInt							0xA1D0
#define TblSetItemStyle							0xA1D1
#define TblUnhighlightSelection					0xA1D2
#define TblSetRowUsable							0xA1D3
#define TblGetNumberOfRows						0xA1D4
#define TblSetCustomDrawProcedure				0xA1D5
#define TblSetRowSelectable						0xA1D6
#define TblRowSelectable							0xA1D7
#define TblSetLoadDataProcedure					0xA1D8
#define TblSetSaveDataProcedure					0xA1D9
#define TblGetBounds								0xA1DA
#define TblSetRowHeight							0xA1DB
#define TblGetColumnWidth						0xA1DC
#define TblGetRowID								0xA1DD
#define TblSetRowID								0xA1DE
#define TblMarkRowInvalid						0xA1DF
#define TblMarkTableInvalid						0xA1E0
#define TblGetSelection							0xA1E1
#define TblInsertRow								0xA1E2
#define TblRemoveRow								0xA1E3
#define TblRowInvalid							0xA1E4
#define TblRedrawTable							0xA1E5
#define TblRowUsable								0xA1E6
#define TblReleaseFocus							0xA1E7
#define TblEditing								0xA1E8
#define TblGetCurrentField						0xA1E9
#define TblSetColumnUsable						0xA1EA
#define TblGetRowHeight							0xA1EB
#define TblSetColumnWidth						0xA1EC
#define TblGrabFocus								0xA1ED
#define TblSetItemPtr							0xA1EE
#define TblFindRowID								0xA1EF
#define TblGetLastUsableRow						0xA1F0
#define TblGetColumnSpacing						0xA1F1
#define TblFindRowData							0xA1F2
#define TblGetRowData							0xA1F3
#define TblSetRowData							0xA1F4
#define TblSetColumnSpacing						0xA1F5

#define WinCreateWindow							0xA1F6
#define WinCreateOffscreenWindow					0xA1F7
#define WinDeleteWindow							0xA1F8
#define WinInitializeWindow						0xA1F9
#define WinAddWindow								0xA1FA
#define WinRemoveWindow							0xA1FB
#define WinSetActiveWindow						0xA1FC
#define WinSetDrawWindow							0xA1FD
#define WinGetDrawWindow							0xA1FE
#define WinGetActiveWindow						0xA1FF
#define WinGetDisplayWindow						0xA200
#define WinGetFirstWindow						0xA201
#define WinEnableWindow							0xA202
#define WinDisableWindow							0xA203
#define WinGetWindowFrameRect					0xA204
#define WinDrawWindowFrame						0xA205
#define WinEraseWindow							0xA206
#define WinSaveBits								0xA207
#define WinRestoreBits							0xA208
#define WinCopyRectangle							0xA209
#define WinScrollRectangle						0xA20A
#define WinGetDisplayExtent						0xA20B
#define WinGetWindowExtent						0xA20C
#define WinDisplayToWindowPt						0xA20D
#define WinWindowToDisplayPt						0xA20E
#define WinGetClip								0xA20F
#define WinSetClip								0xA210
#define WinResetClip								0xA211
#define WinClipRectangle							0xA212
#define WinDrawLine								0xA213
#define WinDrawGrayLine							0xA214
#define WinEraseLine								0xA215
#define WinInvertLine							0xA216
#define WinFillLine								0xA217
#define WinDrawRectangle							0xA218
#define WinEraseRectangle						0xA219
#define WinInvertRectangle						0xA21A
#define WinDrawRectangleFrame					0xA21B
#define WinDrawGrayRectangleFrame				0xA21C
#define WinEraseRectangleFrame					0xA21D
#define WinInvertRectangleFrame					0xA21E
#define WinGetFramesRectangle					0xA21F
#define WinDrawChars								0xA220
#define WinEraseChars							0xA221
#define WinInvertChars							0xA222
#define WinGetPattern							0xA223
#define WinSetPattern							0xA224
#define WinSetUnderlineMode						0xA225
#define WinDrawBitmap							0xA226
#define WinModal									0xA227
#define WinGetDrawWindowBounds					0xA228
#define WinFillRectangle							0xA229
#define WinDrawInvertedChars						0xA22A

#define PrefOpenPreferenceDBV10					0xA22B
#define PrefGetPreferences						0xA22C
#define PrefSetPreferences						0xA22D
#define PrefGetAppPreferencesV10					0xA22E
#define PrefSetAppPreferencesV10					0xA22F

#define SndInit									0xA230
#define SndSetDefaultVolume						0xA231
#define SndGetDefaultVolume						0xA232
#define SndDoCmd									0xA233
#define SndPlaySystemSound						0xA234

#define AlmInit									0xA235
#define AlmCancelAll								0xA236
#define AlmAlarmCallback							0xA237
#define AlmSetAlarm								0xA238
#define AlmGetAlarm								0xA239
#define AlmDisplayAlarm							0xA23A
#define AlmEnableNotification					0xA23B

#define HwrGetRAMMapping							0xA23C
#define HwrMemWritable							0xA23D
#define HwrMemReadable							0xA23E
#define HwrDoze									0xA23F
#define HwrSleep									0xA240
#define HwrWake									0xA241
#define HwrSetSystemClock						0xA242
#define HwrSetCPUDutyCycle						0xA243
#define HwrDisplayInit							0xA244  /* Before OS 3.5, this trap a.k.a. HwrLCDInit */
#define HwrDisplaySleep							0xA245  /* Before OS 3.5, this trap a.k.a. HwrLCDSleep, */
#define HwrTimerInit								0xA246
#define HwrCursorV33								0xA247  /* This trap obsoleted for OS 3.5 and later */
#define HwrBatteryLevel							0xA248
#define HwrDelay									0xA249
#define HwrEnableDataWrites						0xA24A
#define HwrDisableDataWrites						0xA24B
#define HwrLCDBaseAddrV33						0xA24C  /* This trap obsoleted for OS 3.5 and later */
#define HwrDisplayDrawBootScreen					0xA24D  /* Before OS 3.5, this trap a.k.a. HwrLCDDrawBitmap */
#define HwrTimerSleep							0xA24E
#define HwrTimerWake								0xA24F
#define HwrDisplayWake							0xA250  /* Before OS 3.5, this trap a.k.a. HwrLCDWake */
#define HwrIRQ1Handler							0xA251
#define HwrIRQ2Handler							0xA252
#define HwrIRQ3Handler							0xA253
#define HwrIRQ4Handler							0xA254
#define HwrIRQ5Handler							0xA255
#define HwrIRQ6Handler							0xA256
#define HwrDockSignals							0xA257
#define HwrPluggedIn								0xA258

#define Crc16CalcBlock							0xA259

#define SelectDayV10								0xA25A
#define SelectTimeV33							0xA25B

#define DayDrawDaySelector						0xA25C
#define DayHandleEvent							0xA25D
#define DayDrawDays								0xA25E
#define DayOfWeek								0xA25F
#define DaysInMonth								0xA260
#define DayOfMonth								0xA261

#define DateDaysToDate							0xA262
#define DateToDays								0xA263
#define DateAdjust								0xA264
#define DateSecondsToDate						0xA265
#define DateToAscii								0xA266
#define DateToDOWDMFormat						0xA267
#define TimeToAscii								0xA268

#define Find										0xA269
#define FindStrInStr								0xA26A
#define FindSaveMatch							0xA26B
#define FindGetLineBounds						0xA26C
#define FindDrawHeader							0xA26D

#define PenOpen									0xA26E
#define PenClose									0xA26F
#define PenGetRawPen								0xA270
#define PenCalibrate								0xA271
#define PenRawToScreen							0xA272
#define PenScreenToRaw							0xA273
#define PenResetCalibration						0xA274
#define PenSleep									0xA275
#define PenWake									0xA276

#define ResLoadForm								0xA277
#define ResLoadMenu								0xA278

#define FtrInit									0xA279
#define FtrUnregister							0xA27A
#define FtrGet									0xA27B
#define FtrSet									0xA27C
#define FtrGetByIndex							0xA27D

#define GrfInit									0xA27E
#define GrfFree									0xA27F
#define GrfGetState								0xA280
#define GrfSetState								0xA281
#define GrfFlushPoints							0xA282
#define GrfAddPoint								0xA283
#define GrfInitState								0xA284
#define GrfCleanState							0xA285
#define GrfMatch									0xA286
#define GrfGetMacro								0xA287
#define GrfFilterPoints							0xA288
#define GrfGetNumPoints							0xA289
#define GrfGetPoint								0xA28A
#define GrfFindBranch							0xA28B
#define GrfMatchGlyph							0xA28C
#define GrfGetGlyphMapping						0xA28D
#define GrfGetMacroName							0xA28E
#define GrfDeleteMacro							0xA28F
#define GrfAddMacro								0xA290
#define GrfGetAndExpandMacro						0xA291
#define GrfProcessStroke							0xA292
#define GrfFieldChange							0xA293

#define GetCharSortValue							0xA294
#define GetCharAttr								0xA295
#define GetCharCaselessValue						0xA296

#define PwdExists								0xA297
#define PwdVerify								0xA298
#define PwdSet									0xA299
#define PwdRemove								0xA29A

#define GsiInitialize							0xA29B
#define GsiSetLocation							0xA29C
#define GsiEnable								0xA29D
#define GsiEnabled								0xA29E
#define GsiSetShiftState							0xA29F

#define KeyInit									0xA2A0
#define KeyHandleInterrupt						0xA2A1
#define KeyCurrentState							0xA2A2
#define KeyResetDoubleTap						0xA2A3
#define KeyRates									0xA2A4
#define KeySleep									0xA2A5
#define KeyWake									0xA2A6

#define DlkControl								0xA2A7  /* was CmBroadcast */

#define DlkStartServer							0xA2A8
#define DlkGetSyncInfo							0xA2A9
#define DlkSetLogEntry							0xA2AA

#define IntlDispatch								0xA2AB  /* REUSED IN v3.1 (was PsrInit in 1.0, removed in 2.0) */
#define SysLibLoad								0xA2AC  /* REUSED IN v2.0 (was PsrClose) */
#define SndPlaySmf								0xA2AD  /* REUSED IN v3.0 (was PsrGetCommand in 1.0, removed in 2.0) */
#define SndCreateMidiList						0xA2AE  /* REUSED IN v3.0 (was PsrSendReply in 1.0, removed in 2.0) */

#define AbtShowAbout								0xA2AF

#define MdmDial									0xA2B0
#define MdmHangUp								0xA2B1

#define DmSearchRecord							0xA2B2

#define SysInsertionSort							0xA2B3
#define DmInsertionSort							0xA2B4

#define LstSetTopItem							0xA2B5

// ======================================================================
// Palm OS 2.X traps					Palm Pilot and 2.0 Upgrade Card
// ======================================================================

#define SclSetScrollBar							0xA2B6
#define SclDrawScrollBar							0xA2B7
#define SclHandleEvent							0xA2B8

#define SysMailboxCreate							0xA2B9
#define SysMailboxDelete							0xA2BA
#define SysMailboxFlush							0xA2BB
#define SysMailboxSend							0xA2BC
#define SysMailboxWait							0xA2BD

#define SysTaskWait								0xA2BE
#define SysTaskWake								0xA2BF
#define SysTaskWaitClr							0xA2C0
#define SysTaskSuspend							0xA2C1
#define SysTaskResume							0xA2C2

#define CategoryCreateList						0xA2C3
#define CategoryFreeList							0xA2C4
#define CategoryEditV20							0xA2C5
#define CategorySelect							0xA2C6

#define DmDeleteCategory							0xA2C7

#define SysEvGroupCreate							0xA2C8
#define SysEvGroupSignal							0xA2C9
#define SysEvGroupRead							0xA2CA
#define SysEvGroupWait							0xA2CB

#define EvtEventAvail							0xA2CC
#define EvtSysEventAvail							0xA2CD
#define StrNCopy									0xA2CE

#define KeySetMask								0xA2CF

#define SelectDay								0xA2D0

#define PrefGetPreference						0xA2D1
#define PrefSetPreference						0xA2D2
#define PrefGetAppPreferences					0xA2D3
#define PrefSetAppPreferences					0xA2D4

#define FrmPointInTitle							0xA2D5

#define StrNCat									0xA2D6

#define MemCmp									0xA2D7

#define TblSetColumnEditIndicator				0xA2D8

#define FntWordWrap								0xA2D9

#define FldGetScrollValues						0xA2DA

#define SysCreateDataBaseList					0xA2DB
#define SysCreatePanelList						0xA2DC

#define DlkDispatchRequest						0xA2DD

#define StrPrintF								0xA2DE
#define StrVPrintF								0xA2DF

#define PrefOpenPreferenceDB						0xA2E0

#define SysGraffitiReferenceDialog				0xA2E1

#define SysKeyboardDialog						0xA2E2

#define FntWordWrapReverseNLines					0xA2E3
#define FntGetScrollValues						0xA2E4

#define TblSetRowStaticHeight					0xA2E5
#define TblHasScrollBar							0xA2E6

#define SclGetScrollBar							0xA2E7

#define FldGetNumberOfBlankLines					0xA2E8

#define SysTicksPerSecond						0xA2E9
#define HwrBacklightV33							0xA2EA  /* This trap obsoleted for OS 3.5 and later */
#define DmDatabaseProtect						0xA2EB

#define TblSetBounds								0xA2EC

#define StrNCompare								0xA2ED
#define StrNCaselessCompare						0xA2EE

#define PhoneNumberLookup						0xA2EF

#define FrmSetMenu								0xA2F0

#define EncDigestMD5								0xA2F1

#define DmFindSortPosition						0xA2F2

#define SysBinarySearch							0xA2F3
#define SysErrString								0xA2F4
#define SysStringByIndex							0xA2F5

#define EvtAddUniqueEventToQueue					0xA2F6

#define StrLocalizeNumber						0xA2F7
#define StrDelocalizeNumber						0xA2F8
#define LocGetNumberSeparators					0xA2F9

#define MenuSetActiveMenuRscID					0xA2FA

#define LstScrollList							0xA2FB

#define CategoryInitialize						0xA2FC

#define EncDigestMD4								0xA2FD
#define EncDES									0xA2FE

#define LstGetVisibleItems						0xA2FF

#define WinSetBounds								0xA300

#define CategorySetName							0xA301

#define FldSetInsertionPoint						0xA302

#define FrmSetObjectBounds						0xA303

#define WinSetColors								0xA304

#define FlpDispatch								0xA305
#define FlpEmDispatch							0xA306

// ======================================================================
// Palm OS 3.0 traps					Palm III and 3.0 Upgrade Card
// ======================================================================

#define ExgInit									0xA307
#define ExgConnect								0xA308
#define ExgPut									0xA309
#define ExgGet									0xA30A
#define ExgAccept								0xA30B
#define ExgDisconnect							0xA30C
#define ExgSend									0xA30D
#define ExgReceive								0xA30E
#define ExgRegisterData							0xA30F
#define ExgNotifyReceiveV35						0xA310
#define SysReserved30Trap2 						0xA311	/* "Reserved" trap in Palm OS 3.0 and later (was ExgControl) */

#define PrgStartDialogV31						0xA312  /* Updated in v3.2 */
#define PrgStopDialog							0xA313
#define PrgUpdateDialog							0xA314
#define PrgHandleEvent							0xA315

#define ImcReadFieldNoSemicolon					0xA316
#define ImcReadFieldQuotablePrintable			0xA317
#define ImcReadPropertyParameter					0xA318
#define ImcSkipAllPropertyParameters				0xA319
#define ImcReadWhiteSpace						0xA31A
#define ImcWriteQuotedPrintable					0xA31B
#define ImcWriteNoSemicolon						0xA31C
#define ImcStringIsAscii							0xA31D

#define TblGetItemFont							0xA31E
#define TblSetItemFont							0xA31F

#define FontSelect								0xA320
#define FntDefineFont							0xA321

#define CategoryEdit								0xA322

#define SysGetOSVersionString					0xA323
#define SysBatteryInfo							0xA324
#define SysUIBusy								0xA325

#define WinValidateHandle						0xA326
#define FrmValidatePtr							0xA327
#define CtlValidatePointer						0xA328
#define WinMoveWindowAddr						0xA329
#define FrmAddSpaceForObject						0xA32A
#define FrmNewForm								0xA32B
#define CtlNewControl							0xA32C
#define FldNewField								0xA32D
#define LstNewList								0xA32E
#define FrmNewLabel								0xA32F
#define FrmNewBitmap								0xA330
#define FrmNewGadget								0xA331

#define FileOpen									0xA332
#define FileClose								0xA333
#define FileDelete								0xA334
#define FileReadLow								0xA335
#define FileWrite								0xA336
#define FileSeek									0xA337
#define FileTell									0xA338
#define FileTruncate								0xA339
#define FileControl								0xA33A

#define FrmActiveState							0xA33B

#define SysGetAppInfo							0xA33C
#define SysGetStackInfo							0xA33D

#define WinScreenMode							0xA33E  /* was ScrDisplayMode */
#define HwrLCDGetDepthV33						0xA33F  /* This trap obsoleted for OS 3.5 and later */
#define HwrGetROMToken							0xA340

#define DbgControl								0xA341

#define ExgDBRead								0xA342
#define ExgDBWrite								0xA343

#define HostControl								0xA344  /* Renamed from SysGremlins, functionality generalized */
#define FrmRemoveObject							0xA345

#define SysReserved30Trap1						0xA346  /* "Reserved" trap in Palm OS 3.0 and later (was SysReserved1) */

// NOTE: The following two traps are reserved for future mgrs
// that may or may not be present on any particular device.
// They are NOT present by default; code must check first!
#define ExpansionDispatch						0xA347  /* Reserved for ExpansionMgr (was SysReserved2) */
#define FileSystemDispatch						0xA348  /* Reserved for FileSystemMgr (was SysReserved3) */

#define OEMDispatch								0xA349  /* OEM trap in Palm OS 3.0 and later trap table (formerly SysReserved4) */

// ======================================================================
// Palm OS 3.1 traps					Palm IIIx and Palm V
// ======================================================================

#define HwrLCDContrastV33						0xA34A  /* This trap obsoleted for OS 3.5 and later */
#define SysLCDContrast							0xA34B
#define UIContrastAdjust							0xA34C  /* Renamed from ContrastAdjust */
#define HwrDockStatus							0xA34D

#define FntWidthToOffset							0xA34E
#define SelectOneTime							0xA34F
#define WinDrawChar								0xA350
#define WinDrawTruncChars						0xA351

#define SysNotifyInit							0xA352  /* Notification Manager traps */
#define SysNotifyRegister						0xA353
#define SysNotifyUnregister						0xA354
#define SysNotifyBroadcast						0xA355
#define SysNotifyBroadcastDeferred				0xA356
#define SysNotifyDatabaseAdded					0xA357
#define SysNotifyDatabaseRemoved					0xA358

#define SysWantEvent								0xA359

#define FtrPtrNew								0xA35A
#define FtrPtrFree								0xA35B
#define FtrPtrResize								0xA35C

#define SysReserved31Trap1						0xA35D  /* "Reserved" trap in Palm OS 3.1 and later (was SysReserved5) */

// ======================================================================
// Palm OS 3.2 & 3.3 traps		Palm VII (3.2) and Fall '99 Palm OS Flash Update (3.3)
// ======================================================================

#define HwrNVPrefSet								0xA35E  /* mapped to FlashParmsWrite */
#define HwrNVPrefGet								0xA35F  /* mapped to FlashParmsRead */
#define FlashInit								0xA360
#define FlashCompress							0xA361
#define FlashErase								0xA362
#define FlashProgram								0xA363

#define AlmTimeChange							0xA364
#define ErrAlertCustom							0xA365
#define PrgStartDialog							0xA366  /* New version of PrgStartDialogV31 */

#define SerialDispatch							0xA367
#define HwrBattery								0xA368
#define DmGetDatabaseLockState					0xA369

#define CncGetProfileList						0xA36A
#define CncGetProfileInfo						0xA36B
#define CncAddProfile							0xA36C
#define CncDeleteProfile							0xA36D

#define SndPlaySmfResource						0xA36E

#define MemPtrDataStorage						0xA36F  /* Never actually installed until now. */

#define ClipboardAppendItem						0xA370

#define WiCmdV32									0xA371  /* Code moved to INetLib; trap obsolete */

// ======================================================================
// Palm OS 3.5 traps				Palm IIIc and other products
// ======================================================================

// HAL Display-layer new traps
#define HwrDisplayAttributes						0xA372
#define HwrDisplayDoze							0xA373
#define HwrDisplayPalette						0xA374

// Screen driver new traps
#define BltFindIndexes							0xA375
#define BmpGetBits								0xA376  /* was BltGetBitsAddr */
#define BltCopyRectangle							0xA377
#define BltDrawChars								0xA378
#define BltLineRoutine							0xA379
#define BltRectangleRoutine						0xA37A

// ScrUtils new traps
#define ScrCompress								0xA37B
#define ScrDecompress							0xA37C

// System Manager new traps
#define SysLCDBrightness							0xA37D

// WindowColor new traps
#define WinPaintChar								0xA37E
#define WinPaintChars							0xA37F
#define WinPaintBitmap							0xA380
#define WinGetPixel								0xA381
#define WinPaintPixel							0xA382
#define WinDrawPixel								0xA383
#define WinErasePixel							0xA384
#define WinInvertPixel							0xA385
#define WinPaintPixels							0xA386
#define WinPaintLines							0xA387
#define WinPaintLine								0xA388
#define WinPaintRectangle						0xA389
#define WinPaintRectangleFrame					0xA38A
#define WinPaintPolygon							0xA38B
#define WinDrawPolygon							0xA38C
#define WinErasePolygon							0xA38D
#define WinInvertPolygon							0xA38E
#define WinFillPolygon							0xA38F
#define WinPaintArc								0xA390
#define WinDrawArc								0xA391
#define WinEraseArc								0xA392
#define WinInvertArc								0xA393
#define WinFillArc								0xA394
#define WinPushDrawState							0xA395
#define WinPopDrawState							0xA396
#define WinSetDrawMode							0xA397
#define WinSetForeColor							0xA398
#define WinSetBackColor							0xA399
#define WinSetTextColor							0xA39A
#define WinGetPatternType						0xA39B
#define WinSetPatternType						0xA39C
#define WinPalette								0xA39D
#define WinRGBToIndex							0xA39E
#define WinIndexToRGB							0xA39F
#define WinScreenLock							0xA3A0
#define WinScreenUnlock							0xA3A1
#define WinGetBitmap								0xA3A2

// UIColor new traps
#define UIColorInit								0xA3A3
#define UIColorGetTableEntryIndex				0xA3A4
#define UIColorGetTableEntryRGB					0xA3A5
#define UIColorSetTableEntry						0xA3A6
#define UIColorPushTable							0xA3A7
#define UIColorPopTable							0xA3A8

// misc cleanup and API additions

#define CtlNewGraphicControl						0xA3A9

#define TblGetItemPtr							0xA3AA

#define UIBrightnessAdjust						0xA3AB
#define UIPickColor								0xA3AC

#define EvtSetAutoOffTimer						0xA3AD

// Misc int'l/overlay support.
#define TsmDispatch								0xA3AE
#define OmDispatch								0xA3AF
#define DmOpenDBNoOverlay						0xA3B0
#define DmOpenDBWithLocale						0xA3B1
#define ResLoadConstant							0xA3B2

// new boot-time SmallROM HAL additions
#define HwrPreDebugInit							0xA3B3
#define HwrResetNMI								0xA3B4
#define HwrResetPWM								0xA3B5

#define KeyBootKeys								0xA3B6

#define DbgSerDrvOpen							0xA3B7
#define DbgSerDrvClose							0xA3B8
#define DbgSerDrvControl							0xA3B9
#define DbgSerDrvStatus							0xA3BA
#define DbgSerDrvWriteChar						0xA3BB
#define DbgSerDrvReadChar						0xA3BC

// new boot-time BigROM HAL additions
#define HwrPostDebugInit							0xA3BD
#define HwrIdentifyFeatures						0xA3BE
#define HwrModelSpecificInit						0xA3BF
#define HwrModelInitStage2						0xA3C0
#define HwrInterruptsInit						0xA3C1

#define HwrSoundOn								0xA3C2
#define HwrSoundOff								0xA3C3

// Kernel clock tick routine
#define SysKernelClockTick						0xA3C4

// MenuEraseMenu is exposed as of PalmOS 3.5, but there are
// no public interfaces for it yet.	 Perhaps in a later release.
#define MenuEraseMenu							0xA3C5

#define SelectTime								0xA3C6

// Menu Command Bar traps
#define MenuCmdBarAddButton						0xA3C7
#define MenuCmdBarGetButtonData					0xA3C8
#define MenuCmdBarDisplay						0xA3C9

// Silkscreen info
#define HwrGetSilkscreenID						0xA3CA
#define EvtGetSilkscreenAreaList					0xA3CB

#define SysFatalAlertInit						0xA3CC
#define DateTemplateToAscii						0xA3CD

// New traps dealing with masking private records
#define SecVerifyPW								0xA3CE
#define SecSelectViewStatus						0xA3CF
#define TblSetColumnMasked						0xA3D0
#define TblSetRowMasked							0xA3D1
#define TblRowMasked								0xA3D2

// New form trap for dialogs with text entry field
#define FrmCustomResponseAlert					0xA3D3
#define FrmNewGsi								0xA3D4

// New dynamic menu functions
#define MenuShowItem								0xA3D5
#define MenuHideItem								0xA3D6
#define MenuAddItem								0xA3D7

// New form traps for "smart gadgets"
#define FrmSetGadgetHandler						0xA3D8

// More new control functions
#define CtlSetGraphics							0xA3D9
#define CtlGetSliderValues						0xA3DA
#define CtlSetSliderValues						0xA3DB
#define CtlNewSliderControl						0xA3DC

// Bitmap manager functions
#define BmpCreate								0xA3DD
#define BmpDelete								0xA3DE
#define BmpCompress								0xA3DF
// BmpGetBits defined in Screen driver traps
#define BmpGetColortable							0xA3E0
#define BmpSize									0xA3E1
#define BmpBitsSize								0xA3E2
#define BmpColortableSize						0xA3E3

// extra window namager
#define WinCreateBitmapWindow					0xA3E4

// Ask for a null event sooner (replaces a macro which Poser hated)
#define EvtSetNullEventTick						0xA3E5

// Exchange manager call to allow apps to select destination categories
#define ExgDoDialog								0xA3E6

// this call will remove temporary UI like popup lists
#define SysUICleanup								0xA3E7

// The following 4 traps were "Reserved" traps, present only in SOME post-release builds of Palm OS 3.5
#define WinSetForeColorRGB						0xA3E8
#define WinSetBackColorRGB						0xA3E9
#define WinSetTextColorRGB						0xA3EA
#define WinGetPixelRGB							0xA3EB

// ======================================================================
// Palm OS 4.0 Traps
// ======================================================================

#define HighDensityDispatch						0xA3EC
#define SysReserved40Trap2						0xA3ED
#define SysReserved40Trap3						0xA3EE
#define SysReserved40Trap4						0xA3EF

// New Trap selector added for New Connection Mgr API
#define CncMgrDispatch							0xA3F0

// new trap for notify from interrupt, implemented in SysEvtMgr.c
#define SysNotifyBroadcastFromInterrupt			0xA3F1

// new trap for waking the UI without generating a null event
#define EvtWakeupWithoutNilEvent					0xA3F2

// new trap for doing stable, fast, 7-bit string compare
#define StrCompareAscii							0xA3F3

// New trap for accessors available thru PalmOS glue
#define AccessorDispatch							0xA3F4

#define BltGetPixel								0xA3F5
#define BltPaintPixel							0xA3F6

#define ScrScreenInit							0xA3F7
#define ScrUpdateScreenBitmap					0xA3F8
#define ScrPalette								0xA3F9
#define ScrGetColortable							0xA3FA
#define ScrGetGrayPat							0xA3FB
#define ScrScreenLock							0xA3FC
#define ScrScreenUnlock							0xA3FD

#define FntPrvGetFontList						0xA3FE

// Exchange manager functions
#define ExgRegisterDatatype						0xA3FF
#define ExgNotifyReceive							0xA400
#define ExgNotifyGoto							0xA401
#define ExgRequest								0xA402
#define ExgSetDefaultApplication					0xA403
#define ExgGetDefaultApplication					0xA404
#define ExgGetTargetApplication					0xA405
#define ExgGetRegisteredApplications				0xA406
#define ExgGetRegisteredTypes					0xA407
#define ExgNotifyPreview							0xA408
#define ExgControl								0xA409

// 04/30/00	CS - New Locale Manager handles access to region-specific info like date formats
#define LmDispatch								0xA40A

// 05/10/00 kwk - New Memory Manager trap for retrieving ROM NVParam values (sys use only)
#define MemGetRomNVParams						0xA40B

// 05/12/00 kwk - Safe character width Font Mgr call
#define FntWCharWidth							0xA40C

// 05/17/00 kwk - Faster DmFindDatabase
#define DmFindDatabaseWithTypeCreator			0xA40D

// New Trap selectors added for time zone picker API
#define SelectTimeZone							0xA40E
#define TimeZoneToAscii							0xA40F

// 08/18/00 kwk - trap for doing stable, fast, 7-bit string compare.
// 08/21/00 kwk - moved here in place of SelectDaylightSavingAdjustment.
#define StrNCompareAscii							0xA410

// New Trap selectors added for time zone conversion API
#define TimTimeZoneToUTC							0xA411
#define TimUTCToTimeZone							0xA412

// New trap implemented in PhoneLookup.c
#define PhoneNumberLookupCustom					0xA413

// new trap for selecting debugger path.
#define HwrDebugSelect							0xA414

#define BltRoundedRectangle						0xA415
#define BltRoundedRectangleFill					0xA416
#define WinPrvInitCanvas							0xA417

#define HwrCalcDynamicHeapSize					0xA418
#define HwrDebuggerEnter							0xA419
#define HwrDebuggerExit							0xA41A

#define LstGetTopItem							0xA41B

#define HwrModelInitStage3						0xA41C

// 06/21/00 peter - New Attention Manager
#define AttnIndicatorAllow						0xA41D
#define AttnIndicatorAllowed						0xA41E
#define AttnIndicatorEnable						0xA41F
#define AttnIndicatorEnabled						0xA420
#define AttnIndicatorSetBlinkPattern				0xA421
#define AttnIndicatorGetBlinkPattern				0xA422
#define AttnIndicatorTicksTillNextBlink			0xA423
#define AttnIndicatorCheckBlink					0xA424
#define AttnInitialize							0xA425
#define AttnGetAttention							0xA426
#define AttnUpdate								0xA427
#define AttnForgetIt								0xA428
#define AttnGetCounts							0xA429
#define AttnListOpen								0xA42A
#define AttnHandleEvent							0xA42B
#define AttnEffectOfEvent						0xA42C
#define AttnIterate								0xA42D
#define AttnDoSpecialEffects						0xA42E
#define AttnDoEmergencySpecialEffects			0xA42F
#define AttnAllowClose							0xA430
#define AttnReopen								0xA431
#define AttnEnableNotification					0xA432
#define HwrLEDAttributes							0xA433
#define HwrVibrateAttributes						0xA434

// Trap for getting and setting the device password hint.
#define SecGetPwdHint							0xA435
#define SecSetPwdHint							0xA436

#define HwrFlashWrite							0xA437

#define KeyboardStatusNew						0xA438
#define KeyboardStatusFree						0xA439
#define KbdSetLayout								0xA43A
#define KbdGetLayout								0xA43B
#define KbdSetPosition							0xA43C
#define KbdGetPosition							0xA43D
#define KbdSetShiftState							0xA43E
#define KbdGetShiftState							0xA43F
#define KbdDraw									0xA440
#define KbdErase									0xA441
#define KbdHandleEvent							0xA442

#define OEMDispatch2								0xA443

#define HwrCustom								0xA444

// 08/28/00 kwk - Trap for getting form's active field.
#define FrmGetActiveField						0xA445

// 9/18/00 rkr - Added for playing sounds regardless of interruptible flag
#define SndPlaySmfIrregardless					0xA446
#define SndPlaySmfResourceIrregardless			0xA447
#define SndInterruptSmfIrregardless				0xA448

// 10/14/00 ABa: UDA manager
#define UdaMgrDispatch							0xA449

// WK: private traps for PalmOS
#define PalmPrivate1								0xA44A
#define PalmPrivate2								0xA44B
#define PalmPrivate3								0xA44C
#define PalmPrivate4								0xA44D

// 11/07/00 tlw: Added accessors
#define BmpGetDimensions							0xA44E
#define BmpGetBitDepth							0xA44F
#define BmpGetNextBitmap							0xA450
#define TblGetNumberOfColumns					0xA451
#define TblGetTopRow								0xA452
#define TblSetSelection							0xA453
#define FrmGetObjectIndexFromPtr					0xA454

// 11/10/00 acs
#define BmpGetSizes								0xA455

#define WinGetBounds								0xA456

#define BltPaintPixels							0xA457

// 11/22/00 bob
#define FldSetMaxVisibleLines					0xA458

// 01/09/01 acs
#define ScrDefaultPaletteState					0xA459

// ======================================================================
// Palm OS 5.0 Traps
// No new traps were added for 4.1, though 4.1 SC (see below) added some.
// ======================================================================

// 11/16/01 bob
#define PceNativeCall							0xA45A

// ======================================================================
// Palm OS 5.1 Traps
// ======================================================================

// 12/04/01 lrt
#define SndStreamCreate							0xA45B
#define SndStreamDelete							0xA45C
#define SndStreamStart							0xA45D
#define SndStreamPause							0xA45E
#define SndStreamStop							0xA45F
#define SndStreamSetVolume						0xA460
#define SndStreamGetVolume						0xA461
#define SndPlayResource							0xA462
#define SndStreamSetPan							0xA463
#define SndStreamGetPan							0xA464

// 04/12/02 jed
#define MultimediaDispatch						0xA465

// TRAPS ABOVE THIS POINT CAN NOT CHANGE BECAUSE THEY HAVE
// BEEN RELEASED TO CUSTOMERS IN SHIPPING ROMS AND SDKS.
// (MOVE THIS COMMENT DOWN WHENEVER THE "NEXT" RELEASE OCCURS.)

// ======================================================================
// Palm OS 5.1.1 Traps
// ======================================================================

// 08/02/02 mne
#define SndStreamCreateExtended					0xa466
#define SndStreamDeviceControl					0xa467

// ======================================================================
// Palm OS 4.2SC (Simplified Chinese) Traps
// These were added to an older 68K-based version of Palm OS to support
// QVGA displays.
// ======================================================================

// 09/23/02 acs & bob
#define BmpCreateVersion3						0xA468
#define ECFixedMul								0xA469
#define ECFixedDiv								0xA46A
#define HALDrawGetSupportedDensity				0xA46B
#define HALRedrawInputArea						0xA46C
#define GrfBeginStroke							0xA46D
#define BmpPrvConvertBitmap						0xA46E

// ======================================================================
// Palm OS 5.x Traps
// These were added for new features or extensions for 5.x
// ======================================================================
#define NavSelector			 			        0xA46F


// 12/11/02 grant
#define PinsDispatch								0xA470

// ======================================================================
// Palm OS 5.3 Traps
// These were added for new features or extensions for 5.2. Currently
// they aren't implemented by any version of Palm OS released by
// PalmSource, but are reserved for future implementation.
// ======================================================================
#define SysReservedTrap1		 			0xA471
#define SysReservedTrap2					0xA472
#define SysReservedTrap3					0xA473
#define SysReservedTrap4					0xA474


#define DmSync                           0xA475
#define DmSyncDatabase                   0xA476
