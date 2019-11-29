#ifndef ARM_SIDE_CODE_H
#define ARM_SIDE_CODE_H

enum{
   ARM_TEST_DATA_EXCHANGE = 0,
   ARM_TEST_TSC2101_READ_ADC_VALUES,
   TOTAL_ARM_TESTS
};

#if 0
/*
 userData68KP is a uint32_t arg list and return buffer
 The number of the test to run is userData68KP[0] and userData68KP[>=1] is the arguments to the test and return data from it
 All data in the arg list is in little endian format, 68K must convert to/from when calling
 */
unsigned long runTest(const void* emulStateP, void* userData68KP, /*Call68KFuncType*/void* call68KFuncP);
#endif

#endif
