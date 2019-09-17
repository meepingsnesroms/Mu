#ifndef TESTS_H
#define TESTS_H

#include "testSuite.h"

var testButtonInput(void);
var listDataRegisters(void);
var listRegisterFunctions(void);
var listRegisterDirections(void);
var checkSpi2EnableBitDelay(void);
var tstat1GetSemaphoreLockOrder(void);
var ads7846Read(void);
var getClk32Frequency(void);
var getDeviceInfo(void);
var getCpuInfo(void);
var getInterruptInfo(void);
var getIcrInversion(void);
var doesIsrClearChangePinValue(void);
var toggleBacklight(void);
var toggleMotor(void);
var toggleAlarmLed(void);
var watchPenIrq(void);
var getPenPosition(void);
var playConstantTone(void);
var unaligned32bitAccess(void);
var isIrq2AttachedToSdCardChipSelect(void);
var callSysUnimplemented(void);
var testArmAccess(void);
var tsc2101ReadAllAnalogValues(void);

#endif
