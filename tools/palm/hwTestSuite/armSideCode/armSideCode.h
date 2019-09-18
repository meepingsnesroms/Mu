#ifndef ARM_SIDE_CODE_H
#define ARM_SIDE_CODE_H

#include <stdint.h>

enum{
   ARM_TEST_TSC2101_READ_ADC_VALUES = 0,
   TOTAL_ARM_TESTS
}


/*
userData68KP is a uint32_t arg list and return buffer
The number of the test to run is userData68KP[0]
All data in the arg list is in little endian format, 68K must convert to/from when calling
*/
unsigned long runTest(const void* emulStateP, void* userData68KP, /*Call68KFuncType*/void* call68KFuncP);

#endif
